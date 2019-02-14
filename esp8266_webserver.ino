/*This is a basic web server developed for the ESP8266 12-E NodeMCU chip using the Arduino IDE.*/

/*Acknowedgements:
DIYProjects.io for instructions on how to use Bootstrap and general handling of client-side requests.
buZZ on the /r/Arduino IRC for teaching me port-forwarding and general networking stuff.
RandomNerdTutorials.com for the basic framework for running a web server on the ESP8266.
Ivan Grokhotkov for the core Arduino documentation for the ESP8266.
/r/Arduino and the Arduino Stack Exchange for a lot of random troubleshooting.*/

#include <WiFiClient.h>
#include <ESP8266WebServer.h> // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer
#include <SimpleDHT.h>

/*
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
*/

//CHANGE THESE TO MATCH WIFI CREDENTIALS AND LOGIN CREDENTIALS
#define ssid "" // WiFi SSID
#define password "" // WiFi password
#define authUsername "" // Auth Username
#define authPassword "" // Auth Password
#define jsSource "" // URL for .js

bool authenticated = false;
unsigned long sessionStart = 0;
unsigned long sessionLength = 180000;

bool stateD5 = false;
bool stateD6 = false;
bool stateD7 = false;
bool stateD8 = false;
bool stateLED_BUILTIN = false;

int pinDHT11 = 12;
SimpleDHT11 dht11(pinDHT11);
int lastErrorDHT; // Last error sent by Simple DHT

int brightnessPin = 0;

String dataRequested = "temperature";

float currentTemperature;
float currentPressure;
float currentHumidity;
float currentBrightness;

unsigned long lastTemperature;
unsigned long lastPressure;
unsigned long lastHumidity;
unsigned long lastBrightness;

// Time at last update for weather arrays
unsigned long lastUpdate = 0;

// Arrays for weather data - 0 = now, 5 = 5 hours ago, updates once an hour
float pastTemperature[] = {0,0,0,0,0,0,0,0,0,0};
float pastPressure[] = {0,0,0,0,0,0,0,0,0,0};
float pastHumidity[] = {0,0,0,0,0,0,0,0,0,0};
float pastBrightness[] = {0,0,0,0,0,0,0,0,0,0};

// Arrays (and number of pushes) for average weather data in the last hour (4/min = 240, so 300 and expect a few 0s at the end - to be removed in average())- to be averaged and put into pastTemp, etc
int pushNumber = 0;
float avgTemperature[300];
float avgPressure[300];
float avgHumidity[300];
float avgBrightness[300];

// Populates above arrays with 0s;
void populateAvgArr() {
	for (int i = 0; i < 300; i++) {
		avgTemperature[i] = 0;
		avgPressure[i] = 0;
		avgHumidity[i] = 0;
		avgBrightness[i] = 0;
	}
}

ESP8266WebServer server (303); // Starts web server on port 303

// General HTTP header pre-fixed to all server messages
// Includes: Bootstrap, Plotly.js, Site Title
String bootstrapHeader = "<html lang='en'><head><meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1'/><script src='https://code.jquery.com/jquery-3.3.1.slim.min.js' integrity='sha384-q8i/X+965DzO0rT7abK41JStQIAqVgRVzpbzo5smXKp4YfRvH+8abtTE1Pi6jizo' crossorigin='anonymous'></script><script src='https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.14.3/umd/popper.min.js' integrity='sha384-ZMP7rVo3mIykV+2+9J3UJ46jBk0WLaUAdn689aCwoqbBJiSnjAK/l8WvCWPIPm49' crossorigin='anonymous'></script><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css' integrity='sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO' crossorigin='anonymous'> <script src='https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/js/bootstrap.min.js' integrity='sha384-ChfqqxuZUCnJSK3+MXmPNIyE6ZbWh2IMqE241rYiqJxyMiZ6OW/JmZQ5stwEULTy' crossorigin='anonymous'></script><script src='https://cdn.plot.ly/plotly-latest.min.js'></script><title>ESP8266 Home Server</title></head>";

// Creates invisible elements of data for Plotly to append to HTML files - Returns string of HTML (probably could easily make a for loop to do this work)
String getData(){
  String page = "<input id='temperature0' name='temperature0' type='hidden' value='";
  page += pastTemperature[0]; // Temperature 0 hrs ago
  page += "'><input id='pressure0' name='pressure0' type='hidden' value='";
  page += pastPressure[0]; // Pressure 0 hrs ago
  page += "'><input id='humidity0' name='humidity0' type='hidden' value='";
  page += pastHumidity[0]; // Humidity 0 hrs ago
  page += "'><input id='temperature1' name='temperature1' type='hidden' value='";
  page += pastTemperature[1]; // Temperature 1 hr ago
  page += "'><input id='pressure1' name='pressure1' type='hidden' value='";
  page += pastPressure[1]; // Pressure 1 hr ago
  page += "'><input id='humidity1' name='humidity1' type='hidden' value='";
  page += pastHumidity[1]; // Humidity 1 hr ago
  page += "'><input id='temperature2' name='temperature2' type='hidden' value='";
  page += pastTemperature[2]; // Temperature 2 hrs ago
  page += "'><input id='pressure2' name='pressure2' type='hidden' value='";
  page += pastPressure[2]; // Pressure 2 hrs ago
  page += "'><input id='humidity2' name='humidity2' type='hidden' value='";
  page += pastHumidity[2]; // Humidity 2 hrs ago
  page += "'><input id='temperature3' name='temperature3' type='hidden' value='";
  page += pastTemperature[3]; // Temperature 3 hrs ago
  page += "'><input id='pressure3' name='pressure3' type='hidden' value='";
  page += pastPressure[3]; // Pressure 3 hrs ago
  page += "'><input id='humidity3' name='humidity3' type='hidden' value='";
  page += pastHumidity[3]; // Humidity 3 hrs ago
  page += "'><input id='temperature4' name='temperature4' type='hidden' value='";
  page += pastTemperature[4]; // Temperature 4 hrs ago
  page += "'><input id='pressure4' name='pressure4' type='hidden' value='";
  page += pastPressure[4]; // Pressure 4 hrs ago
  page += "'><input id='humidity4' name='humidity4' type='hidden' value='";
  page += pastHumidity[4]; // Humidity 4 hrs ago
  page += "'><input id='temperature5' name='temperature5' type='hidden' value='";
  page += pastTemperature[5]; // Temperature 5 hrs ago
  page += "'><input id='pressure5' name='pressure5' type='hidden' value='";
  page += pastPressure[5]; // Pressure 5 hrs ago
  page += "'><input id='humidity5' name='humidity5' type='hidden' value='";
  page += pastHumidity[5]; // Humidity 5 hrs ago
  page += "'><input id='temperature6' name='temperature6' type='hidden' value='";
  page += pastTemperature[6]; // Temperature 6 hrs ago
  page += "'><input id='pressure6' name='pressure6' type='hidden' value='";
  page += pastPressure[6]; // Pressure 6 hrs ago
  page += "'><input id='humidity6' name='humidity6' type='hidden' value='";
  page += pastHumidity[6]; // Humidity 6 hrs ago
  page += "'><input id='temperature7' name='temperature7' type='hidden' value='";
  page += pastTemperature[7]; // Temperature 7 hrs ago
  page += "'><input id='pressure7' name='pressure7' type='hidden' value='";
  page += pastPressure[7]; // Pressure 7 hrs ago
  page += "'><input id='humidity7' name='humidity7' type='hidden' value='";
  page += pastHumidity[7]; // Humidity 7 hrs ago
  page += "'><input id='temperature8' name='temperature8' type='hidden' value='";
  page += pastTemperature[8]; // Temperature 8 hrs ago
  page += "'><input id='pressure8' name='pressure8' type='hidden' value='";
  page += pastPressure[8]; // Pressure 8 hrs ago
  page += "'><input id='humidity8' name='humidity8' type='hidden' value='";
  page += pastHumidity[8]; // Humidity 8 hrs ago
  page += "'><input id='temperature9' name='temperature9' type='hidden' value='";
  page += pastTemperature[9]; // Temperature 9 hrs ago
  page += "'><input id='pressure9' name='pressure9' type='hidden' value='";
  page += pastPressure[9]; // Pressure 9 hrs ago
  page += "'><input id='humidity9' name='humidity9' type='hidden' value='";
  page += pastHumidity[9]; // Humidity 9 hrs ago
  page += "'><input id='dataRequested' name='dataRequested' type='hidden' value='";
  page += dataRequested; // Requested Graph
  page += "'>";
  return page;
}

// Returns badge identifier - String "primary"
String weatherBadge(String badgeRequest){
	if (badgeRequest == dataRequested) {
		return "btn btn-secondary";
	} else {
		return "btn btn-info";
	}
}

// Creates log in page HTML file to serve to client - Returns string of HTML
String getLoginPage(){
  String page = bootstrapHeader;
  page += "<div class='container-fluid'><div class='row'><div class='col-md-12'><div class='page-header'><h1>ESP8266 Home Server <small>Log In</small></h1></div><form role='form' form action='/' method='POST'><div class='form-group'><label>Username</label><input type='text' class='form-control' name='username' value='' /></div><div class='form-group'><label>Password</label><input type='password' class='form-control' name='password' value='' /></div><button type='submit' class='btn btn-primary' name='login'>Submit</button></form></div></div></div>";
  return page;
}

// Creates main control page HTML file to serve to client - Returns string of HTML
String getMainPage(){
String page = bootstrapHeader; // Adds header that contains scripts for Bootstrap
page += getData(); // Adds invisible divs that contain weather data.=
page += "<div class='container-fluid'>";
page += 	"<div class='row'>";
page += 		"<div class='col-md-12'>";
page += 			"<div class='page-header'>";
page += 				"<h1>";
page += 					"ESP8266 Home Server <small>Control Panel</small>";
page += 				"</h1>";
page += 			"</div>";
page += 			"<div class='row'>";
page += 				"<div id='tableDiv' class='col-md-5'>";
page += 					"<table class='table'>";
page += 						"<thead>";
page += 							"<tr>";
page += 								"<th>";
page += 									"Weather Parameter";
page += 								"</th>";
page += 								"<th>";
page += 									"Value";
page += 								"</th>";
page += 								"<th>";
page += 									"Last Update";
page += 								"</th>";
page += 							"</tr>";
page += 						"</thead>";
page += 						"<tbody>";
page += 							"<tr class='table-active'>";
page += 								"<td>";
page += 									"Temperature (&#176;C)";
page += 								"</td>";
page += 								"<td>";
page += 									currentTemperature;
page += 								"</td>";
page += 								"<td>";
page += 									lastTemperature;
page += 								"</td>";
page += 							"</tr>";
page += 							"<tr class='table-success'>";
page += 								"<td>";
page += 									"Air Pressure (mBar)";
page += 								"</td>";
page += 								"<td>";
page += 									currentPressure;
page += 								"</td>";
page += 								"<td>";
page += 									lastPressure;
page += 								"</td>";
page += 							"</tr>";
page += 							"<tr class='table-warning'>";
page += 								"<td>";
page += 									"Humidity (%)";
page += 								"</td>";
page += 								"<td>";
page += 									currentHumidity;
page += 								"</td>";
page += 								"<td>";
page += 									lastHumidity;
page += 								"</td>";
page += 							"</tr>";
page += 							"<tr class='table-danger'>";
page += 								"<td>";
page += 									"Brightness (AU)";
page += 								"</td>";
page += 								"<td>";
page += 									currentBrightness;
page += 								"</td>";
page += 								"<td>";
page += 									lastBrightness;
page += 								"</td>";
page += 							"</tr>";
page += 						"</tbody>";
page += 					"</table>";
page += 					"<table class='table'>";
page += 						"<thead>";
page += 							"<tr>";
page += 								"<th>";
page += 									"Product";
page += 								"</th>";
page += 								"<th>";
page += 									"Payment Taken";
page += 								"</th>";
page += 								"<th>";
page += 									"Status";
page += 								"</th>";
page += 							"</tr>";
page += 						"</thead>";
page += 						"<tbody>";
page += 							"<tr class='table-active'>";
page += 								"<td>";
page += 									"TB - Monthly";
page += 								"</td>";
page += 								"<td>";
page += 									"01/04/2012";
page += 								"</td>";
page += 								"<td>";
page += 									"Approved";
page += 								"</td>";
page += 							"</tr>";
page += 							"<tr class='table-success'>";
page += 								"<td>";
page += 									"TB - Monthly";
page += 								"</td>";
page += 								"<td>";
page += 									"02/04/2012";
page += 								"</td>";
page += 								"<td>";
page += 									"Declined";
page += 								"</td>";
page += 							"</tr>";
page += 							"<tr class='table-warning'>";
page += 								"<td>";
page += 									"TB - Monthly";
page += 								"</td>";
page += 								"<td>";
page += 									"03/04/2012";
page += 								"</td>";
page += 								"<td>";
page += 									"Pending";
page += 								"</td>";
page += 							"</tr>";
page += 							"<tr class='table-danger'>";
page += 								"<td>";
page += 									"TB - Monthly";
page += 								"</td>";
page += 								"<td>";
page += 									"04/04/2012";
page += 								"</td>";
page += 								"<td>";
page += 									"Call in to confirm";
page += 								"</td>";
page += 							"</tr>";
page += 						"</tbody>";
page += 					"</table>";
page += 				"</div>";
page += 				"<div class='col-md-2'>";
page += 				"</div>";
page += 				"<div class='col-md-5'>";
page += 					"<div class='row'>";
page += 						"<div id='weatherNavbar' class='col-md-12'>";
page += 							"<ul class='nav nav-pills'>";
page += 								"<li class='nav-item'>";
page += 									"<form action='/' method='POST'><button type='button submit' name='requestData' value='temperature' class='";
page += 									weatherBadge("temperature");
page += 									"'>Temperature <span class='badge badge-secondary'>";
page += 									currentTemperature;
page += 									"</span></button></form>";
page += 								"</li>";
page += 								"<li class='nav-item'>";
page += 									"<form action='/' method='POST'><button type='button submit' name='requestData' value='pressure' class='";
page += 									weatherBadge("pressure");
page += 									"'>Pressure <span class='badge badge-secondary'>";
page += 									currentPressure;
page += 									"</span></button></form>";
page += 								"</li>";
page += 								"<li class='nav-item'>";
page += 									"<form action='/' method='POST'><button type='button submit' name='requestData' value='humidity' class='";
page += 									weatherBadge("humidity");
page += 									"'>Humidity <span class='badge badge-secondary'>";
page += 									currentHumidity;
page += 									"</span></button></form>";
page += 								"</li>";
page += 								"<li class='nav-item'>";
page += 									"<form action='/' method='POST'><button type='button submit' name='requestData' value='brightness' class='";
page += 									weatherBadge("brightness");
page += 									"'>Brightness <span class='badge badge-secondary'>";
page += 									currentBrightness;
page += 									"</span></button></form>";
page += 								"</li>";
page += 							"</ul>";
page += 						"</div>";
page += 					"</div>";
page += 					"<div class='row'>";
page += 						"<div id='mainPlot' class='col-md-5'>";
page += 						"</div>";
page += 					"</div>";
page += 				"</div>";
page += 			"</div>";
page += 			"<div class='row'>";
page += 				"<div class='col-md-6'>";
page += 					"<div class='row'>";
page += 						"<div class='col-md-4'>";
page += 							"<h3>";
page += 								"Onboard LED";
page += 							"</h3>";
page += 						"</div>";
page += 						"<div class='col-md-4'>";

page += 							"<form action='/' method='POST'><button type='button submit' name='LED_BUILTIN' value='0' class='btn btn-info'>";
page += 							"ON";
page += 							"</button></form>";
page += 						"</div>";
page += 						"<div class='col-md-4'>";

page += 							"<form action='/' method='POST'><button type='button submit' name='LED_BUILTIN' value='1' class='btn btn-danger'>";
page += 							"OFF";
page += 							"</button></form>";
page += 						"</div>";
page += 					"</div>";
page += 					"<div class='row'>";
page += 						"<div class='col-md-4'>";
page += 							"<h3>";
page += 								"h3. Lorem ipsum dolor sit amet.";
page += 							"</h3>";
page += 						"</div>";
page += 						"<div class='col-md-4'>";

page += 							"<button type='button' class='btn btn-success'>";
page += 								"Button";
page += 							"</button>";
page += 						"</div>";
page += 						"<div class='col-md-4'>";

page += 							"<button type='button' class='btn btn-success'>";
page += 								"Button";
page += 							"</button>";
page += 						"</div>";
page += 					"</div>";
page += 				"</div>";
page += 				"<div class='col-md-6'>";
page += 					"<div class='row'>";
page += 						"<div class='col-md-4'>";
page += 							"<h3>";
page += 								"h3. Lorem ipsum dolor sit amet.";
page += 							"</h3>";
page += 						"</div>";
page += 						"<div class='col-md-4'>";

page += 							"<button type='button' class='btn btn-success'>";
page += 								"Button";
page += 							"</button>";
page += 						"</div>";
page += 						"<div class='col-md-4'>";

page += 							"<button type='button' class='btn btn-success'>";
page += 								"Button";
page += 							"</button>";
page += 						"</div>";
page += 					"</div>";
page += 					"<div class='row'>";
page += 						"<div class='col-md-4'>";
page += 							"<h3>";
page += 								"h3. Lorem ipsum dolor sit amet.";
page += 							"</h3>";
page += 						"</div>";
page += 						"<div class='col-md-4'>";

page += 							"<button type='button' class='btn btn-success'>";
page += 								"Button";
page += 							"</button>";
page += 						"</div>";
page += 						"<div class='col-md-4'>";

page += 							"<button type='button' class='btn btn-success'>";
page += 								"Button";
page += 							"</button>";
page += 						"</div>";
page += 					"</div>";
page += 				"</div>";
page += 			"</div>";
page += 		"</div>";
page += 	"</div>";
page += "</div>";
page += "<body><script type='text/javascript' src='";
page += jsSource; // Javascript Source
page += "'></script></body>"; 
  return page;
}

// Recieves and dispatches based on kind of argument - Calls other functions and passes on argument
void handleRoot(){
  if (server.hasArg("login")){
    handleLogin();
  }
  if (authenticated) {
    if (server.hasArg("D5")) {
      updatePin(D5,server.arg("D5"));
    } else if (server.hasArg("D6")) {
      updatePin(D6,server.arg("D6"));
    } else if (server.hasArg("D7")) {
      updatePin(D7,server.arg("D7"));
    } else if (server.hasArg("D8")) {
      updatePin(D8,server.arg("D8"));
    } else if (server.hasArg("LED_BUILTIN")){
      updatePin(LED_BUILTIN, server.arg("LED_BUILTIN"));
    } else if (server.hasArg("restart")) {
      ESP.restart();
		} else if (server.hasArg("requestData")) {
			dataRequested = server.arg("requestData");
			server.send (200,"text/html",getMainPage());
    } else {
    server.send (404,"text/html",getMainPage());
    }
  } else {
    server.send (301,"text/html",getLoginPage());
  }
}

// Recieves "login" argument handleRoot() - Modifies bool authenticated
void handleLogin(){
  String inputUsername = server.arg("username"); // Argument value from username field
  String inputPassword = server.arg("password"); // Argument value from password field
  if (inputUsername == authUsername && inputPassword == authPassword){ // Starts "session" if credentials are valid
    sessionStart = millis();
    authenticated = true;
    Serial.println("Successfully Authenticated!"); Serial.print("Session started at:"); Serial.println(sessionStart);
    server.send (200,"text/html",getMainPage());
  } else { // Redirects back to log in page with error 301 if credentials are invalid
    server.send (301,"text/html",getLoginPage());
    Serial.println("Invalid Credentials!");
  }
}

// Checks to see if "session" is still valid - Modifies bool authenticated
void checkSession(){
  unsigned long sessionCurrent = millis();
  if (authenticated){
    if (sessionStart + sessionLength < sessionCurrent){
      Serial.println("Session Expired");
      authenticated = false;
      sessionStart = 0;
    }
  }
}

// Recieves specific pin arguments from handleRoot() - Writes to hardware
// Pin Argument Patterns:
// 0:H(inf); 1:L(inf); 2:L(500)H(500)L(inf)
void updatePin(uint8_t pin, String pinValue) {
  Serial.print("Command #"); Serial.print(pinValue); Serial.print(" sent to "); Serial.println(pin);
  server.send (200,"text/html",getMainPage());
  if (pinValue == "1") {
    digitalWrite(pin, HIGH);
  } else if (pinValue == "0") {
    digitalWrite(pin, LOW);
  } else if (pinValue == "2") {
    digitalWrite(pin, LOW);
    delay(500);
    digitalWrite(pin, HIGH);
    delay(500);
    digitalWrite(pin, LOW);
  } else {
    Serial.println("Error: Invalid Pin Value Argument.");
  }
}

// Updates current weather and traks when to update weather arrays, once an hour - Calls other functions
void updateWeather() {	
  // Adapted SimpleDHT example
  float tempCurrentTemperature = 0;
  float tempCurrentHumidity = 0;
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    // Passes error codes to lastErrorDHT
		Serial.print("SimpleDHT Error: "); Serial.println(err);
    lastErrorDHT = err;
    delay(1000);
    return;
  } else {
    // If no error, save temporary values
    tempCurrentTemperature = (int)temperature;
    tempCurrentHumidity = (int)humidity;
  }
		
  // If values arent 0, push to global variables
  if (tempCurrentTemperature != 0) {
    currentTemperature = tempCurrentTemperature;
  }
  if (tempCurrentHumidity != 0) {
    currentHumidity = tempCurrentHumidity;
  }
	
	//Pushes current data into average arrays
	avgTemperature[pushNumber] = currentTemperature;
	avgPressure[pushNumber] = currentPressure;
	avgHumidity[pushNumber] = currentHumidity;
	avgBrightness[pushNumber] = currentBrightness;
  pushNumber++;
	
  // If one hour has elapsed, move values in past weather arrays
  if (lastUpdate == 0) { // On start up
    pastTemperature[0] = currentTemperature;
    pastPressure[0] = currentPressure;
    pastHumidity[0] = currentHumidity;
		lastUpdate = millis();
  } else if (millis() - lastUpdate > 3600000) {
    Serial.print("Updating Weather Data at "); Serial.println(millis());
    lastUpdate = millis();
    pastTemperature[9] = pastTemperature[8];
    pastPressure[9] = pastPressure[8];
    pastHumidity[9] = pastHumidity[8];
    pastTemperature[8] = pastTemperature[7];
    pastPressure[8] = pastPressure[7];
    pastHumidity[8] = pastHumidity[7];
    pastTemperature[7] = pastTemperature[6];
    pastPressure[7] = pastPressure[6];
    pastHumidity[7] = pastHumidity[6];
    pastTemperature[6] = pastTemperature[5];
    pastPressure[6] = pastPressure[5];
    pastHumidity[6] = pastHumidity[5];
    pastTemperature[5] = pastTemperature[4];
    pastPressure[5] = pastPressure[4];
    pastHumidity[5] = pastHumidity[4];
    pastTemperature[4] = pastTemperature[3];
    pastPressure[4] = pastPressure[3];
    pastHumidity[4] = pastHumidity[3];
    pastTemperature[3] = pastTemperature[2];
    pastPressure[3] = pastPressure[2];
    pastHumidity[3] = pastHumidity[2];
    pastTemperature[2] = pastTemperature[1];
    pastPressure[2] = pastPressure[1];
    pastHumidity[2] = pastHumidity[1];
    pastTemperature[1] = pastTemperature[0];
    pastPressure[1] = pastPressure[0];
    pastHumidity[1] = pastHumidity[0];
		
		// averages avgArrays
    pastTemperature[0] = average(avgTemperature);
    pastPressure[0] = average(avgPressure);
    pastHumidity[0] = average(avgHumidity);
		pastBrightness[0] = average(avgBrightness);
		
		// resets avg arrays
		populateAvgArr();
		pushNumber = 0;
  }
}

// Gets average of a float array with a certain number of elements after removing 0s- Returns float
float average(float avgArray[300]) {
  float sum;
  int elementCount = 300;
  int finalElementCount = elementCount;
  for (int i = 0; i < elementCount; i++) {
    if (avgArray[i] != 0) {
      sum += avgArray[i];
    } else {
      finalElementCount--;
    }
  }
  return (sum/finalElementCount);
}

// Writes server status to Serial - Serial.print()
void serialUpdate() {
  Serial.println(""); Serial.println("=====");
  Serial.print("ESP8266 Status Update @ "); Serial.print(millis()/60000); Serial.println(" minutes");
  if (authenticated) {
    Serial.print("Currently authenticated @ "); Serial.println(sessionStart);
  } else {
    Serial.println("Currently unauthenticated");
  }
  if (lastErrorDHT != 0) {
    Serial.print("Temperature: "); Serial.print(currentTemperature); Serial.println("*C");
    Serial.print("Humidity: "); Serial.print(currentHumidity); Serial.println("%");
  } else {
    Serial.print("SimpleDHT Error: "); Serial.println(lastErrorDHT);
  }
  Serial.println("====="); Serial.println("");

}

// Initializes pins, serial, and server
void setup() {
	populateAvgArr(); // fills avg arrs with 0s
	
  pinMode(D5,OUTPUT);
  pinMode(D6,OUTPUT);
	//D7 is for DHT11
  pinMode(D8,OUTPUT);
  pinMode(LED_BUILTIN,OUTPUT);
  
  Serial.begin (9600);
  
  WiFi.begin (ssid,password);
  Serial.print ("Connecting to ");
  Serial.print (ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay (250); Serial.print (".");
  }
  Serial.println ("");
  Serial.print ("Connected to "); Serial.println (ssid);
  Serial.print ("Local IP address: "); Serial.println (WiFi.localIP());

  server.on ("/",handleRoot);

  server.begin();
  Serial.println ("HTTP server started.");
  Serial.println("=====");
}

int weatherSkips = 30;
void loop() {
  checkSession();
  if (weatherSkips == 30){ //updates weather only 4 times a minute
    updateWeather();
    weatherSkips = 0;
    serialUpdate();
  } else {
    weatherSkips++;
  }
  server.handleClient();
  delay(500);
}