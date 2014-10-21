

var options = JSON.parse(localStorage.getItem('options'));
//console.log('read options: ' + JSON.stringify(options));
if (options === null) options = { "use_gps" : "true",
                                  "location" : "",
                                  "units" : "fahrenheit",
                                  "invert_color" : "false",
                                  "hidebatt" : "false",
                                  "hidedate" : "false",
                                  "hideweather" : "false",
                                  "hidesec" : "false",
					    		  "bluetoothvibe" : "false",
                                  "hourlyvibe" : "false"};

function getWeatherFromLatLong(latitude, longitude) {
  var response;
  var woeid = -1;
  var query = encodeURI("select woeid from geo.placefinder where text=\""+latitude+","+longitude + "\" and gflags=\"R\"");
  var url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json";
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if (req.status == 200) {
        response = JSON.parse(req.responseText);
        if (response) {
          woeid = response.query.results.Result.woeid;
          getWeatherFromWoeid(woeid);
        }
      } else {
        console.log("Error");
      }
    }
  }
  req.send(null);
}

function getWeatherFromLocation(location_name) {
  var response;
  var woeid = -1;

  var query = encodeURI("select woeid from geo.places(1) where text=\"" + location_name + "\"");
  var url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json";
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if (req.status == 200) {
        // console.log(req.responseText);
        response = JSON.parse(req.responseText);
        if (response) {
          woeid = response.query.results.place.woeid;
          getWeatherFromWoeid(woeid);
        }
      } else {
        console.log("Error");
      }
    }
  }
  req.send(null);
}

function getWeatherFromWoeid(woeid) {
  var celsius = options['units'] == 'celsius';
  var query = encodeURI("select item.condition from weather.forecast where woeid = " + woeid +
                        " and u = " + (celsius ? "\"c\"" : "\"f\""));
  var url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json";

  var response;
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if (req.status == 200) {
        response = JSON.parse(req.responseText);
        if (response) {
          var condition = response.query.results.channel.item.condition;
          temperature = condition.temp + (celsius ? "\ C" : "\ F");
//          icon = imageId[condition.code];
          // console.log("temp " + temperature);
          // console.log("icon " + icon);
          // console.log("condition " + condition.text);
          Pebble.sendAppMessage({
//            "icon" : icon,
            "temperature" : temperature,
            "invert_color" : (options["invert_color"] == "true" ? 1 : 0),
            "hidebatt" : (options["hidebatt"] == "true" ? 1 : 0),
			"hidedate" : (options["hidedate"] == "true" ? 1 : 0),
			"hideweather" : (options["hideweather"] == "true" ? 1 : 0),
            "hidesec" : (options["hidesec"] == "true" ? 1 : 0),
			"bluetoothvibe" : (options["bluetoothvibe"] == "true" ? 1 : 0),
            "hourlyvibe" : (options["hourlyvibe"] == "true" ? 1 : 0),
          });
        }
      } else {
        console.log("Error");
      }
    }
  }
  req.send(null);
}

function updateWeather() {
  if (options['use_gps'] == "true") {
    window.navigator.geolocation.getCurrentPosition(locationSuccess,
                                                    locationError,
                                                    locationOptions);
  } else {
    getWeatherFromLocation(options["location"]);
  }
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 };

function locationSuccess(pos) {
  var coordinates = pos.coords;
  getWeatherFromLatLong(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
//    "icon":11,
    "temperature":""
  });
}

Pebble.addEventListener('showConfiguration', function(e) {
  var uri = 'http://www.themapman.com/pebblewatch/bdigital3.html?' +
    'use_gps=' + encodeURIComponent(options['use_gps']) +
    '&location=' + encodeURIComponent(options['location']) +
    '&units=' + encodeURIComponent(options['units']) +
    '&invert_color=' + encodeURIComponent(options['invert_color']) +
    '&hidebatt=' + encodeURIComponent(options['hidebatt']) +
	'&hidedate=' + encodeURIComponent(options['hidedate']) +
	'&hideweather=' + encodeURIComponent(options['hideweather']) +
	'&hidesec=' + encodeURIComponent(options['hidesec']) +
	'&bluetoothvibe=' + encodeURIComponent(options['bluetoothvibe']) +
    '&hourlyvibe=' + encodeURIComponent(options['hourlyvibe']);
  //console.log('showing configuration at uri: ' + uri);

  Pebble.openURL(uri);
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e.response) {
    options = JSON.parse(decodeURIComponent(e.response));
    localStorage.setItem('options', JSON.stringify(options));
    //console.log('storing options: ' + JSON.stringify(options));
    updateWeather();
  } else {
    console.log('no options received');
  }
});

Pebble.addEventListener("ready", function(e) {
  //console.log("connect!" + e.ready);
  updateWeather();
  setInterval(function() {
    //console.log("timer fired");
    updateWeather();
  }, 1800000); // 30 minutes
  console.log(e.type);
});