#include <pebble.h>
#include <time.h>
#include <stdio.h>

#define PERSIST_INVERTED_STATE_KEY 1

#define OPTION_ONLY_VIBRATE_ON_BT_OFF false

Window *window;

TextLayer *text_day_layer;
TextLayer *text_bluetooth_layer;
TextLayer *text_battery_layer;
TextLayer *text_time_layer;
TextLayer *text_monthname_layer;
TextLayer *text_dateday_layer;
// TextLayer *text_dateseperator_layer;
TextLayer *text_datemonth_layer;
TextLayer *text_quarter_layer;
TextLayer *text_week_layer;

GColor backgroundColor;
GColor foregroundColor;

Layer *line_layer_1;
Layer *line_layer_2;
Layer *skew_line_layer;
bool btState = false;
bool invertedState = false;
bool isCharging = false;

static const char *day_names[] = {
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

static const char *month_names[] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


void set_default_colors();

// DRAWING FUNCTIONS

void line_layer_update_callback(Layer *layer, GContext* ctx) {
  GColor color1 = foregroundColor;
  if (invertedState) {
    color1 = backgroundColor;
  }
  graphics_context_set_fill_color(ctx, color1);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void skewline_layer_update_callback(Layer *layer, GContext *ctx) {
  GPoint p0 = GPoint(0, 0);
  GPoint p1 = GPoint(30, 30);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, p0, p1);
}



void update_bt_mark() {
	static char bluetooth_text[] = "×";
  if (btState) {
    text_layer_set_text(text_bluetooth_layer, "");
  }
  else {
    text_layer_set_text(text_bluetooth_layer, bluetooth_text);
  }
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  
// Need to be static because they're used by the system later.
  static char time_text[6];
  static char day_text[4];
  static char week_text[64] ;
  static char quarter_text[64] ;
  static char monthname_text[4];
  static char dateday_text[6];
//  static char dateseperator_text[4];// "/"
  static char datemonth_text[4];
  static int lastDay = 0;

  int day = tick_time->tm_mday;
  int month = tick_time->tm_mon;
  int month_padding = month+1 < 10 ? 3 : 2;
  int seperator_padding = month+1 < 10 ? 3 : 1;
  int day_padding = (month+1 < 10 ? 2 : 0) + (day < 10 ? 3 : 2);
  snprintf(monthname_text, sizeof(monthname_text), "%s", month_names[month]);
  snprintf(dateday_text, sizeof(dateday_text), "%*d", day_padding, day);
//  snprintf(dateseperator_text, sizeof(dateseperator_text), "%*s", seperator_padding, "/");
  snprintf(datemonth_text, sizeof(datemonth_text), "%*d", month_padding, month + 1);
 
	int quarter = (month/3) + 1;

  if (day != lastDay)
  {
	  lastDay = day;

    int dayofweek = tick_time->tm_wday;
	  strcpy(day_text, day_names[dayofweek]);
	  text_layer_set_text(text_day_layer, day_text);
 	  text_layer_set_text(text_monthname_layer, monthname_text);
    text_layer_set_text(text_dateday_layer, dateday_text);
//    text_layer_set_text(text_dateseperator_layer, dateseperator_text);
	  text_layer_set_text(text_datemonth_layer, datemonth_text);
    snprintf(quarter_text, sizeof(quarter_text), "Q%d", quarter); //check for buf. overrun?
	  text_layer_set_text(text_quarter_layer, quarter_text);

	  char temp[64];
   	strftime(temp, sizeof(temp), "%V", tick_time);
    snprintf(week_text, sizeof(week_text), "W%s", temp); //check for buf. overrun?
	  text_layer_set_text(text_week_layer, week_text);
    
    update_bt_mark();
  }

  char *time_format;
  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }
  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string for 12h clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(text_time_layer, time_text);
}

int countdownStart;

void resetCountdown() {
  time_t now = time(NULL);
  struct tm *tm_struct = localtime(&now);
  countdownStart = tm_struct->tm_sec;
}

void bt_handler(bool connected) {
  if (btState != connected) {
    if (!OPTION_ONLY_VIBRATE_ON_BT_OFF || !connected)
    {
      vibes_short_pulse();
    }
  }

  btState = connected;
  update_bt_mark();
}

void handle_invert_countdown(struct tm *tick_time, TimeUnits units_changed) {
	static char countdown_text[] = "×";
  int second = tick_time->tm_sec;
  
  if (second % 2 == 0) {
    text_layer_set_text(text_bluetooth_layer, "");
  }
  else {
    text_layer_set_text(text_bluetooth_layer, countdown_text);
  }
  
  if ((second + 60 - countdownStart) % 60 >= 4) {
    accel_tap_service_unsubscribe();
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
    btState =  bluetooth_connection_service_peek();
    bt_handler(btState);
    bluetooth_connection_service_subscribe(bt_handler);
  }
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  invertedState = !invertedState;
  persist_write_bool(PERSIST_INVERTED_STATE_KEY, invertedState);
  set_default_colors();
  vibes_short_pulse();

  // Reset countdown
  resetCountdown();
}



// BATTERY

static void battery_handler(BatteryChargeState charge_state) {
  isCharging = charge_state.is_charging;
  set_default_colors();

  static char s_battery_buffer[16];
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), " %d%%", charge_state.charge_percent);
  
  if (strncmp(s_battery_buffer, " 100%", strlen( s_battery_buffer)) ==0) {
    text_layer_set_text(text_battery_layer, " 100");
  
    } else {
      text_layer_set_text(text_battery_layer, s_battery_buffer);
    }
}

void set_default_colors() {
  GColor color1 = foregroundColor;
  GColor color2 = backgroundColor;
  if (invertedState) {
    color1 = backgroundColor;
    color2 = foregroundColor;
  }
  
  window_set_background_color(window, color2);

  text_layer_set_text_color(text_day_layer, color1);
  text_layer_set_background_color(text_day_layer, color2);
  text_layer_set_text_color(text_bluetooth_layer, color1);
  text_layer_set_background_color(text_bluetooth_layer, color2);
  text_layer_set_text_color(text_battery_layer, color1);
  text_layer_set_background_color(text_battery_layer, color2);
  text_layer_set_text_color(text_time_layer, color1);
  text_layer_set_background_color(text_time_layer, color2);
  text_layer_set_text_color(text_monthname_layer, color1);
  text_layer_set_background_color(text_monthname_layer, color2);
  text_layer_set_text_color(text_dateday_layer, color1);
  text_layer_set_background_color(text_dateday_layer, color2);
  // text_layer_set_text_color(text_dateseperator_layer, color1);
  // text_layer_set_background_color(text_dateseperator_layer, color2);
  text_layer_set_text_color(text_datemonth_layer, color1);
  text_layer_set_background_color(text_datemonth_layer, color2);
  text_layer_set_text_color(text_quarter_layer, color1);
  text_layer_set_background_color(text_quarter_layer, color2);
  text_layer_set_text_color(text_week_layer, color1);
  text_layer_set_background_color(text_week_layer, color2);

  if (isCharging) {
    text_layer_set_text_color(text_battery_layer, color2);
    text_layer_set_background_color(text_battery_layer, color1);
  }
  else {
    text_layer_set_text_color(text_battery_layer, color1);
    text_layer_set_background_color(text_battery_layer, color2);
  }

  layer_mark_dirty(line_layer_1);
  layer_mark_dirty(line_layer_2);
  layer_mark_dirty(skew_line_layer);
}



// INIT ------------

void handle_init(void) {

#ifdef GColorFromRGB
  backgroundColor = GColorFromHEX(0x000000);
  foregroundColor = GColorFromHEX(0xFFFFFF);
#else
  backgroundColor = GColorWhite;
  foregroundColor = GColorBlack;
#endif

  window = window_create();
  window_stack_push(window, true /* Animated */);

  Layer *window_layer = window_get_root_layer(window);
  
  // Read settings
   if (persist_exists(PERSIST_INVERTED_STATE_KEY)) {
     invertedState = persist_read_bool(PERSIST_INVERTED_STATE_KEY);
   }
 

  // weekday
  text_day_layer = text_layer_create(GRect(4, -3, 70, 40));
  text_layer_set_font(text_day_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONTNORMAL_33)));
  layer_add_child(window_layer, text_layer_get_layer(text_day_layer));

	// bluetooth
  text_bluetooth_layer = text_layer_create(GRect(64, -1, 18, 30));
  text_layer_set_font(text_bluetooth_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONTNORMAL_31)));
  layer_add_child(window_layer, text_layer_get_layer(text_bluetooth_layer));

	// battery
  text_battery_layer = text_layer_create(GRect(81, -1, 80, 40));
  text_layer_set_font(text_battery_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONTNORMAL_31)));
  layer_add_child(window_layer, text_layer_get_layer(text_battery_layer));
	
  // time
  text_time_layer = text_layer_create(GRect(2, 32, 144, 60));
  text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONTBOLD_54)));
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

  // month-name
  text_monthname_layer = text_layer_create(GRect(4, 95, 75, 44));
  text_layer_set_font(text_monthname_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONTNORMAL_33)));
  layer_add_child(window_layer, text_layer_get_layer(text_monthname_layer));

  // date day
  text_dateday_layer = text_layer_create(GRect(56, 95, 60, 42));
  text_layer_set_font(text_dateday_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONTBOLD_33)));
  layer_add_child(window_layer, text_layer_get_layer(text_dateday_layer));

  // date seperator
//  text_dateseperator_layer = text_layer_create(GRect(96, 95, 35, 40));
//  text_layer_set_font(text_dateseperator_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONTNORMAL_31)));
//  layer_add_child(window_layer, text_layer_get_layer(text_dateseperator_layer));
  
  // date month
  text_datemonth_layer = text_layer_create(GRect(106, 95, 90, 42));
  text_layer_set_font(text_datemonth_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONTNORMAL_33)));
  layer_add_child(window_layer, text_layer_get_layer(text_datemonth_layer));

	// quarter
  text_quarter_layer = text_layer_create(GRect(4, 128, 40, 40));
  text_layer_set_font(text_quarter_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONTNORMAL_33)));
  layer_add_child(window_layer, text_layer_get_layer(text_quarter_layer));

  // week no
  text_week_layer = text_layer_create(GRect(81, 128, 60, 40));
  text_layer_set_font(text_week_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONTNORMAL_33)));
  layer_add_child(window_layer, text_layer_get_layer(text_week_layer));

	// line one
  GRect line_frame_one = GRect(0, 38, 144, 1);
  line_layer_1 = layer_create(line_frame_one);
  layer_set_update_proc(line_layer_1, line_layer_update_callback);
  layer_add_child(window_layer, line_layer_1);
  
  // line two
  GRect line_frame_two = GRect(0, 95, 144, 1);
  line_layer_2 = layer_create(line_frame_two);
  layer_set_update_proc(line_layer_2, line_layer_update_callback);
  layer_add_child(window_layer, line_layer_2);	

  // line three (date seperator)
  GRect line_frame_three = GRect(96, 95, 35, 40);
  skew_line_layer = layer_create(line_frame_three);
  layer_set_update_proc(skew_line_layer, skewline_layer_update_callback);
  layer_add_child(window_layer, skew_line_layer); 

  set_default_colors();
	
	// Avoid blank display on launch
  time_t now = time(NULL);
  struct tm *tm_struct = localtime(&now);
  handle_minute_tick(tm_struct, (TimeUnits) NULL);
  resetCountdown();

  tick_timer_service_subscribe(SECOND_UNIT, handle_invert_countdown);

  battery_state_service_subscribe(battery_handler);
  battery_handler(battery_state_service_peek());
  accel_tap_service_subscribe(tap_handler);
}


void handle_deinit(void) {
  bluetooth_connection_service_unsubscribe();
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();

  layer_destroy(line_layer_1);
  layer_destroy(line_layer_2);
  layer_destroy(skew_line_layer);
  text_layer_destroy(text_day_layer);
  text_layer_destroy(text_bluetooth_layer);
  text_layer_destroy(text_battery_layer);
  text_layer_destroy(text_time_layer);
  text_layer_destroy(text_monthname_layer);
  text_layer_destroy(text_dateday_layer);
//  text_layer_destroy(text_dateseperator_layer);
  text_layer_destroy(text_datemonth_layer);
  text_layer_destroy(text_quarter_layer);
  text_layer_destroy(text_week_layer);

  window_destroy(window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
