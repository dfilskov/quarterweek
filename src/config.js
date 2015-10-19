

Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  console.log('Opening config browser');
  Pebble.openURL('https://dfilskov.github.io/quarterweek/');
});

Pebble.addEventListener('webviewclosed', function(e) {
  // Decode and parse config data as JSON
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Config window returned: ' + JSON.stringify(configData));

  // Prepare AppMessage payload
  var dict = {
    'KEY_CHANGE_INVERT_ON_STARTUP': configData.allowChangeInvertOnStartup ? 1 : 0,
  };

  // Send settings to Pebble watchapp
  Pebble.sendAppMessage(dict, function(){
    console.log('Sent config data to Pebble');  
  }, function() {
    console.log('Failed to send config data!');
  });
});
