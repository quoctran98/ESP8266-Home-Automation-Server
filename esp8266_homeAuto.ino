/*This is a basic web server developed for the ESP8266 12-E NodeMCU chip using the Arduino IDE.*/

/*Acknowedgements:
DIYProjects.io for instructions on how to use Bootstrap and general handling of client-side requests.
buZZ on the /r/Arduino IRC for teaching me port-forwarding and general networking stuff.
RandomNerdTutorials.com for the basic framework for running a web server on the ESP8266.
Ivan Grokhotkov for the core Arduino documentation for the ESP8266.
/r/Arduino and the Arduino Stack Exchange for a lot of random troubleshooting.*/

// dht lines: 14 32 33 241 242 243

#include <WiFiClient.h>
#include <ESP8266WebServer.h> // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer
#include <SimpleDHT.h> // https://github.com/winlinvip/SimpleDHT

/*
This matches pins on the NodeMCU board to Arduino IDE pins
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

// CHANGE THESE TO MATCH WIFI CREDENTIALS AND LOGIN CREDENTIALS
#define ssid "" // WiFi SSID
#define password "" // WiFi password
#define serverPort 80 // Router Port
#define authUsername "" // Auth Username
#define authPassword "" // Auth Password
#define jsSessionSource "" // URL for sessionId.js
#define jsMainSource "" // URL for mainPage.js
#define pinDHT11 12 // DATA pin DHT11

ESP8266WebServer server (serverPort); // Creates web server object on port 303

int sessionIdList[10]; // Stores 10 active Session IDs
int pushNumberSessionId = 0; // Counts current index of sessionIdList

// Whether or not thse pins (if used as outputs) are currently on/HIGH (true) or off/LOW (false) - default (if used as input is false)
bool stateD5 = false;
bool stateD6 = false;
bool stateD7 = false;
bool stateD8 = false;
bool stateLED_BUILTIN = true; // Some NodeMCU boards have the HIGH and LOW of LED_BUILTIN switched

// Varibales for SimpleDHT
SimpleDHT11 dht11(pinDHT11); // Creates SimpleDHT11 object
int lastErrorDHT; // Last error sent by SimpleDHT

// Various weather logging data
String dataRequested = "temperature"; // Data type to be sent to Plotly.js
// currVal - Last recorded value for each weather parameter
float currentTemperature;
float currentPressure;
float currentHumidity;
float currentBrightness;
// millis() at last update to weather parameter
unsigned long lastTemperature;
unsigned long lastPressure;
unsigned long lastHumidity;
unsigned long lastBrightness;
// Time at last update for pastArr weather arrays
unsigned long lastUpdate = 0;
//pastArr - Arrays for weather data - index0 is most recent
float pastTemperature[144];
float pastPressure[144];
float pastHumidity[144];
float pastBrightness[144];
// avgArr - Arrays for binning recent currVal before averaging and sending to pastArr + some buffer space for extra values
float avgTemperature[50];
float avgPressure[50];
float avgHumidity[50];
float avgBrightness[50];
// Keeps track of the index in use of avgArr
int pushNumber = 0;

// Populates above pastArr with 0s
void populatePastArr() {
	for (int i = 0; i < 144; i++) {
		pastTemperature[i] = 0;
		pastPressure[i] = 0;
		pastHumidity[i] = 0;
		pastBrightness[i] = 0;
	}
	
}
// Populates above avgArr with 0s
void populateAvgArr() {
	for (int i = 0; i < 50; i++) {
		avgTemperature[i] = 0;
		avgPressure[i] = 0;
		avgHumidity[i] = 0;
		avgBrightness[i] = 0;
	}
}
// Populates sessionIdList with random numbers, to make session ID's harder to spoof, but still possible
void populateSessId() {
	for (int i = 0; i < 10; i++) {
		sessionIdList[i] = randNum(0,99999);
	}
}

// General HTTP header pre-fixed to all server messages
// Includes: HTTP Header, Bootstrap, Plotly.js, Site Title
String bootstrapHeader = "<html lang='en'><head><meta http-equiv='refresh' content='60' name='viewport' content='width=device-width, initial-scale=1'/><script src='https://code.jquery.com/jquery-3.3.1.slim.min.js' integrity='sha384-q8i/X+965DzO0rT7abK41JStQIAqVgRVzpbzo5smXKp4YfRvH+8abtTE1Pi6jizo' crossorigin='anonymous'></script><script src='https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.14.3/umd/popper.min.js' integrity='sha384-ZMP7rVo3mIykV+2+9J3UJ46jBk0WLaUAdn689aCwoqbBJiSnjAK/l8WvCWPIPm49' crossorigin='anonymous'></script><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css' integrity='sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO' crossorigin='anonymous'> <script src='https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/js/bootstrap.min.js' integrity='sha384-ChfqqxuZUCnJSK3+MXmPNIyE6ZbWh2IMqE241rYiqJxyMiZ6OW/JmZQ5stwEULTy' crossorigin='anonymous'></script><script src='https://cdn.plot.ly/plotly-latest.min.js'></script><title>ESP8266 Home Server</title></head>";

// Creates invisible elements of general data for client - Returns string of HTML to be appended to some server sends
String getDataGeneral(){
	String page;
  page += "<input id='dataRequested' name='dataRequested' type='hidden' value='";
  page += dataRequested; // Requested Graph
  page += "'>";
  return page;
}

// Creates invisible elements of past temperature data for client - Returns string of HTML to be appended to some server sends
String getDataTemperature(){
  String page;
  for (int i = 0; i < 144; i++) {
    // Makes "<input id='temperature0' name='temperature0' type='hidden' value=temperature[0]>"
    page += "<input id='temperature";
    page += i;
    page += "' name='temperature";
    page += i;
    page += "' type='hidden' value='";
    page += pastTemperature[i];
    page += "'>";
  }
  return page;
}

// Creates invisible elements of past pressure data for client - Returns string of HTML to be appended to some server sends
String getDataPressure(){
  String page;
  for (int i = 0; i < 144; i++) {
    // Makes "<input id='pressure0' name='pressure0' type='hidden' value=pressure[0]>"
    page += "<input id='pressure";
    page += i;
    page += "' name='pressure";
    page += i;
    page += "' type='hidden' value='";
    page += pastPressure[i];
    page += "'>";
  }
  return page;
}

// Creates invisible elements of past humidity data for client - Returns string of HTML to be appended to some server sends
String getDataHumidity(){
  String page;
  for (int i = 0; i < 144; i++) {
    // Makes "<input id='humidity0' name='humidity'0 type='hidden' value=humidity[0]>"
    page += "<input id='humidity";
    page += i;
    page += "' name='humidity";
    page += i;
    page += "' type='hidden' value='";
    page += pastHumidity[i];
    page += "'>";
  }
  return page;
}

// Creates invisible elements of past brightness data for client - Returns string of HTML to be appended to some server sends
String getDataBrightness(){
  String page;
  for (int i = 0; i < 144; i++) {
    // Makes "<input id='brightness0' name='brightness'0 type='hidden' value=brightness[0]>"
    page += "<input id='brightness";
    page += i;
    page += "' name='brightness";
    page += i;
    page += "' type='hidden' value='";
    page += pastBrightness[i];
    page += "'>";
  }
  return page;
}

// Returns plot button identifier - Returns string for getMainPage()
String weatherBadge(String badgeRequest){
	if (badgeRequest == dataRequested) {
		return "btn btn-secondary";
	} else {
		return "btn btn-info";
	}
}

// Creates log in page HTML file to serve to client - Returns string of HTML
String getLoginPage() {
  String page = bootstrapHeader;
  page += "<div class='container-fluid'><div class='row'><div class='col-md-12'><div class='page-header'><h1>ESP8266 Home Server <small>Log In</small></h1></div><form role='form' form action='/' method='POST'><div class='form-group'><label>Username</label><input type='text' class='form-control' name='username' value='' /></div><div class='form-group'><label>Password</label><input type='password' class='form-control' name='password' value='' /></div><button type='submit' class='btn btn-primary' name='login'>Submit</button></form></div></div></div>";
  return page;
}

String getSessionIdPage(int sessionID) {
	String page = "<input id='sessId' name='sessId' type='hidden' value='";
	page += sessionID;
	page += "'>";
	page += "<form action='/' method='POST'><button type='button submit' id='sessionIdSubmit' name='sessionIdSubmit' value='0'>Redirect</button></form>";
	page += "<body><script type='text/javascript' src='";
	page += jsSessionSource;
	page += "'></script></body>"; // Javascript Source
	return page;
}

// Creates main control page HTML file to serve to client - Returns string of HTML with all necessary elements attached
String getMainPage(){
String page = bootstrapHeader; // Adds header that contains scripts for Bootstrap
page += getDataGeneral(); // Adds invisible divs that contain weather data.
if (dataRequested == "temperature") {
  page += getDataTemperature();
} else if (dataRequested == "pressure") {
  page += getDataPressure();
} else if (dataRequested == "humidity") {
  page += getDataHumidity();
} else if (dataRequested == "brightness") {
  page += getDataBrightness();
}
// I'll try to create a better way to easily edit what HTML is served
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
page += 									"<form action='/' method='POST'><input id='sessionIdSubmit0' name='sessionIdSubmit0' type='hidden' value='0' class='form-group'><button type='button submit' name='requestData' value='temperature' class='";
page += 									weatherBadge("temperature");
page += 									"'>Temperature <span class='badge badge-secondary'>";
page += 									currentTemperature;
page += 									"</span></button></form>";
page += 								"</li>";
page += 								"<li class='nav-item'>";
page += 									"<form action='/' method='POST'><input id='sessionIdSubmit1' name='sessionIdSubmit1' type='hidden' value='0' class='form-group'><button type='button submit' name='requestData' value='pressure' class='";
page += 									weatherBadge("pressure");
page += 									"'>Pressure <span class='badge badge-secondary'>";
page += 									currentPressure;
page += 									"</span></button></form>";
page += 								"</li>";
page += 								"<li class='nav-item'>";
page += 									"<form action='/' method='POST'><input id='sessionIdSubmit2' name='sessionIdSubmit2' type='hidden' value='0' ><button type='button submit' name='requestData' value='humidity' class='";
page += 									weatherBadge("humidity");
page += 									"'>Humidity <span class='badge badge-secondary'>";
page += 									currentHumidity;
page += 									"</span></button></form>";
page += 								"</li>";
page += 								"<li class='nav-item'>";
page += 									"<form action='/' method='POST'><input id='sessionIdSubmit3' name='sessionIdSubmit3' type='hidden' value='0' class='form-group'><button type='button submit' name='requestData' value='brightness' class='";
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

page += 							"<form action='/' method='POST'><input id='sessionIdSubmit4' name='sessionIdSubmit4' type='hidden' value='0' class='form-group'><button type='button submit' name='LED_BUILTIN' value='0' class='btn btn-info'>";
page += 							"ON";
page += 							"</button></form>";
page += 						"</div>";
page += 						"<div class='col-md-4'>";

page += 							"<form action='/' method='POST'><input id='sessionIdSubmit5' name='sessionIdSubmit5' type='hidden' value='0' class='form-group'><button type='button submit' name='LED_BUILTIN' value='1' class='btn btn-danger'>";
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
page += jsMainSource;
page += "'></script></body>"; // Javascript Source
  return page;
}

// Recieves and dispatches client requests based on kind of argument - Calls other handler functions and passes on the argument
// Basic POST format: name=server.hasArg(name) | value=server.arg(name)
// Updating Pins: name = pin number (D0,D1,etc.) | value = what to write (HIGH = 1, LOW = 0, other numbers can be programmed as patterns)
// Other arguments include "login", "username", "password" (for logging in) and "requestData" (for plotly data)
void handleRoot(){
  if (server.hasArg("login")){
    handleLogin();
  } else if (validSessionId(server.arg("sessionIdSubmit")) || validSessionId(server.arg("sessionIdSubmit0")) || validSessionId(server.arg("sessionIdSubmit1")) || validSessionId(server.arg("sessionIdSubmit2")) || validSessionId(server.arg("sessionIdSubmit3")) || validSessionId(server.arg("sessionIdSubmit4")) || validSessionId(server.arg("sessionIdSubmit5"))) { // Will only handle other arguments if currently a valid session id is sent
		Serial.println ("Request recieved with valid Session ID");
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
		Serial.println ("No valid Session ID recieved");
    server.send (403,"text/html",getLoginPage());
  }
}

// Recieves "login" argument handleRoot() - hands client a session id and
void handleLogin(){
  String inputUsername = server.arg("username"); // Argument value from username field
  String inputPassword = server.arg("password"); // Argument value from password field
  if (inputUsername == authUsername && inputPassword == authPassword){ // Starts session and sends sessionId if credentials are valid
		int sessionId = randNum(0, 99999); // Session ID is a random number
		sessionIdList[pushNumberSessionId] = sessionId;
		if (pushNumberSessionId == 99) { // Max sessionId's stored is 100
			pushNumberSessionId = 0;
		} else {
			pushNumberSessionId++;
		}
    Serial.println("Successfully Authenticated!"); Serial.print("Session started with sessionId "); Serial.println(sessionId);
    server.send (200,"text/html",getSessionIdPage(sessionId));
  } else { // Redirects back to log in page with error 301 if credentials are invalid
    server.send (301,"text/html",getLoginPage());
    Serial.println("Invalid Credentials!");
  }
}

// Returns true if sessionId sent is
bool validSessionId(String sessionId) {
	for (int i = 0; i < 10; i++) {
		if (sessionIdList[i] == sessionId.toInt()) {
			return true;
		}
	}
	return false;
}

// Recieves specific pin arguments from handleRoot() - Writes to hardware
// Pin Argument Patterns:
// 0:H(inf); 1:L(inf); 2:L(500)H(500)L(inf)
void updatePin(uint8_t pin, String pinValue) {
  Serial.print("Command #"); Serial.print(pinValue); Serial.print(" sent to "); Serial.println(pin);
  server.send (200,"text/html",getMainPage());
  if (pinValue == "1") {
    digitalWrite(pin, HIGH);
    switch (pin) {
      case D5:
        stateD5 = true;
        break;
      case D6:
        stateD6 = true;
        break;
      case D7:
        stateD7 = true;
        break;
      case D8:
        stateD8 = true;
        break;
      case LED_BUILTIN:
        stateLED_BUILTIN = false;
        break;
    }
  } else if (pinValue == "0") {
    digitalWrite(pin, LOW);
    switch (pin) {
      case D5:
        stateD5 = false;
        break;
      case D6:
        stateD6 = false;
        break;
      case D7:
        stateD7 = false;
        break;
      case D8:
        stateD8 = false;
        break;
      case LED_BUILTIN:
        stateLED_BUILTIN = true;
        break;
    }
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

// Updates current weather and tracks when to update weather arrays - Calls other functions
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
	
  // If 10 minutes has elapsed, average values from avgArr to pastArr and move values in past weather arrays
  if (lastUpdate == 0) { // On start up
    pastTemperature[0] = currentTemperature;
    pastPressure[0] = currentPressure;
    pastHumidity[0] = currentHumidity;
		lastUpdate = millis();
		
  } else if (millis() - lastUpdate > 600000 ) { // every 10 minutes
		
    Serial.print("Updating Weather Data at "); Serial.println(millis());
    lastUpdate = millis();
		
		for (int i = 143; i > 0; i--) {
			pastTemperature[i] = pastTemperature[i-1];
			pastPressure[i] = pastPressure[i-1];
			pastHumidity[i] = pastHumidity[i-1];
			pastBrightness[i] = pastBrightness[i-1];
		}
		
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

// Gets average of a float array with a certain number of elements after removing 0s - Returns float
float average(float avgArray[50]) {
  float sum;
  int elementCount = 50;
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

// Gets a random integer between min and max
int randNum(int min, int max) {
  return rand() % max + min;
}

// Writes server status to Serial - Serial.print()
void serialUpdate() {
  Serial.println(""); Serial.println("=====");
  Serial.print("ESP8266 Status Update @ "); Serial.print(millis()/60000); Serial.println(" minutes");
  if (lastErrorDHT != 0) {
    Serial.print("Temperature: "); Serial.print(currentTemperature); Serial.println("*C");
    Serial.print("Humidity: "); Serial.print(currentHumidity); Serial.println("%");
  } else {
    Serial.print("SimpleDHT Error: "); Serial.println(lastErrorDHT);
  }
	Serial.print("Current Data Requested: "); Serial.println(dataRequested);
  Serial.println("====="); Serial.println("");

}

// Initializes pins, serial, and server
void setup() {
	// Populates some arrays
	populateAvgArr(); // fills avg arrs with 0s
	populatePastArr(); // fills past arrs with 0s
	populateSessId(); // fills session id list with random numbers
	
	// Sets pin modes
	pinMode(D5,OUTPUT);
	pinMode(D6,INPUT);
	pinMode(D7,OUTPUT);
	pinMode(D8,OUTPUT);
  pinMode(LED_BUILTIN,OUTPUT);
  
  Serial.begin (9600);
  
	// Connects to WiFi
  WiFi.begin (ssid,password);
  Serial.print ("Connecting to ");
  Serial.print (ssid);
  while (WiFi.status() != WL_CONNECTED) {delay (250); Serial.print (".");}
  Serial.println ("");
  Serial.print ("Connected to "); Serial.println (ssid);
  Serial.print ("Local IP address: "); Serial.println (WiFi.localIP());

	// Starts Server
  server.on ("/",handleRoot);
  server.begin();
  Serial.println ("HTTP server started.");
  Serial.println("=====");
}

// Main loop - updates weather and seiral periodically and handles clients
int weatherSkips = 30;
void loop() {
  if (weatherSkips == 30){ //Updates weather only 4 times a minute
    updateWeather();
    weatherSkips = 0;
    serialUpdate();
  } else {
    weatherSkips++;
  }
  server.handleClient();
  delay(500);
}