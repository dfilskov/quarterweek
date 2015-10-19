//var configData = {};

Pebble.addEventListener('showConfiguration', function(e) {
  // Get config options

  // Prepare AppMessage payload
  var dict = {
    'KEY_REQUEST_CONFIG': 1
  };

  // Send settings to Pebble app
  Pebble.sendAppMessage(dict, function(){
    console.log('Sent config request to Pebble');  
  }, function() {
    console.log('Failed to send config request!');
  });
});

Pebble.addEventListener('webviewclosed', function(e) {
  // Decode and parse config data as JSON
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Config window returned: ' + JSON.stringify(configData));
    
  // Prepare AppMessage payload
  var dict = {
    'KEY_CHANGE_INVERT_ON_STARTUP': configData.allowChangeInvertOnStartup ? 1 : 0
  };

  // Send settings to Pebble app
  Pebble.sendAppMessage(dict, function(){
    console.log('Sent config data to Pebble');  
  }, function() {
    console.log('Failed to send config data!');
  });
});

Pebble.addEventListener('appmessage', function(e) {
  //console.log('AppMessage received: ' + JSON.stringify(e));
  console.log('AppMessage received: ' + JSON.stringify(e));

  // Read config settings sent from the Pebble app on startup
  var configData = {
    allowChangeInvertOnStartup: e.payload.KEY_CHANGE_INVERT_ON_STARTUP == 1
  };
  
  console.log('Opening config window');
  Pebble.openURL('https://dfilskov.github.io/quarterweek#' + encodeURIComponent(JSON.stringify(configData)));

});