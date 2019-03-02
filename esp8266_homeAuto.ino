/*This is a basic web server developed for the ESP8266 12-E NodeMCU chip using the Arduino IDE.*/

/*Acknowedgements:
DIYProjects.io for instructions on how to use Bootstrap and general handling of client-side requests.
buZZ on the /r/Arduino IRC for teaching me port-forwarding and general networking stuff.
RandomNerdTutorials.com for the basic framework for running a web server on the ESP8266.
Ivan Grokhotkov for the core Arduino documentation for the ESP8266.
/r/Arduino and the Arduino Stack Exchange for a lot of random troubleshooting.
Adafruit for BME280 documentation.
Robot Zero One for ESP8266-specific BME280 documentation.
TECHTUTORIALSSX for instructions on HTTP GET from the ESP8266
*/

// Libraries for Wifi, Web Server, and HTTP Client
#include <WiFiClient.h>
#include <ESP8266WebServer.h> // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer
#include <ESP8266HTTPClient.h>

// Libraries for SPI and BME280
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

bool useJsURLSource = false;

// CHANGE THESE TO MATCH WIFI CREDENTIALS AND LOGIN CREDENTIALS
#define ssid "ssid" // WiFi SSID
#define password "password" // WiFi password
#define serverPort 80 // Router Port

String authUsername[] = {"admin", "guest"}; // Auth Usernames
String authPassword[] = {"password", "password"}; // Auth Passwords
// Auth Permissions - matches index of above lists - 1 is full permission, 2 is permission to change, 3 is permission to view, 4 is webhook permission, 5 is logged out
int authPermission[] = {1,3};

String webhookKeyList[1] = {"KEY"};

// Notification Parameters (only alertLowPressure is currently implemented)
#define alertLowPressure 100.25
#define alertHighPressure 101.00
#define alertLowTemperature 50.00
#define alertHighTemperature 80.00
#define alertLowHumidity 30.00
#define alertHighHumidity 80.00

// BME280 SPI Bus -> Pins (ESP8266 pins don't line up with the IDE stuff)
#define BME_SCK 5 // D1
#define BME_MISO 2 // D4
#define BME_MOSI 4 // D2
#define BME_CS 0 // D3
#define SEALEVELPRESSURE_HPA (1013.25) // Sea Level Pressure in hPa

// Javascript Sources
#define jsSessionIdURLSource "https://quoctran.com/home_automation/esp8266_webserver/sessionId.js" // URL for sessionId.js
#define jsMainURLSource "https://quoctran.com/home_automation/esp8266_webserver/mainPage.js" // URL for mainPage.js

// Minified mainPage.js
#define jsMainStringSource "let sessionId=sessionStorage.getItem('sessionId');sessionId=sessionId.toString(),document.getElementById('sessionIdSubmit0').value=sessionId,console.log(document.getElementById('sessionIdSubmit0').value),document.getElementById('sessionIdSubmit1').value=sessionId,console.log(document.getElementById('sessionIdSubmit1').value),document.getElementById('sessionIdSubmit2').value=sessionId,console.log(document.getElementById('sessionIdSubmit2').value),document.getElementById('sessionIdSubmit3').value=sessionId,console.log(document.getElementById('sessionIdSubmit3').value),document.getElementById('sessionIdSubmit4').value=sessionId,console.log(document.getElementById('sessionIdSubmit4').value),document.getElementById('sessionIdSubmit5').value=sessionId,console.log(document.getElementById('sessionIdSubmit5').value);const mainPlot=document.getElementById('mainPlot'),dataKeep=144;let pastTemperature=[],pastPressure=[],pastHumidity=[],dataRequested=document.getElementById('dataRequested').value;if('temperature'==dataRequested)for(i=0;i<144;i++){let e='temperature'+String(i),t=document.getElementById(e).value;t=Number(t),pastTemperature.push(t)}else if('pressure'==dataRequested)for(i=0;i<144;i++){let e='pressure'+String(i),t=document.getElementById(e).value;t=Number(t),pastPressure.push(t)}else if('humidity'==dataRequested)for(i=0;i<144;i++){let e='humidity'+String(i),t=document.getElementById(e).value;t=Number(t),pastHumidity.push(t)}else if('brightness'==dataRequested)for(i=0;i<144;i++){let e='brightness'+String(i),t=document.getElementById(e).value;t=Number(t),pastBrightness.push(t)}const hourString=['12am','1am','2am','3am','4am','5am','6am','7am','8am','9am','10am','11am','12pm','1pm','2pm','3pm','4pm','5pm','6pm','7pm','8pm','9pm','10pm','11pm'];let today=new Date,hour=today.getHours(),hourArrayNames=[],hourLeftover=23-hour;for(i=hour;i>-1;i--)hourArrayNames.push(hourString[i]);for(i=23;i>23-hourLeftover;i--)hourArrayNames.push(hourString[i]);let hourArray=[];for(i=0;i<144;i++)hourArray[i]=i;let Inside,minute=today.getMinutes(),hourArrayVal=[],roundMin=Math.round(minute/10);for(i=0;i<144;i++)(i-roundMin)%6==0&&hourArrayVal.push(i);'temperature'==dataRequested?Inside={x:hourArray,y:pastTemperature,type:'scatter'}:'pressure'==dataRequested?Inside={x:hourArray,y:pastPressure,type:'scatter'}:'humidity'==dataRequested?Inside={x:hourArray,y:pastHumidity,type:'scatter'}:'brightness'==dataRequested&&(Inside={x:hourArray,y:pastBrightness,type:'scatter'});let data=[Inside];function getYAxis(){switch(dataRequested){case'temperature':return'Temperature (&#176;F)';case'pressure':return'Air Pressure (kPa)';case'humidity':return'Humidity (rel. %)';case'brightness':return'Brightness (A.U.)'}}let navbarWidth=document.getElementById('weatherNavbar').offsetWidth,tableHeight=document.getElementById('tableDiv').offsetHeight;function yAxisRange(){switch(dataRequested){case'temperature':return[30,100];case'pressure':return[80,120];case'humidity':return[0,100];case'brightness':return[0,1023]}}var layout={autosize:!0,width:navbarWidth-25,height:tableHeight,xaxis:{autorange:'reversed',tickvals:hourArrayVal,ticktext:hourArrayNames},yaxis:{title:getYAxis(),range:yAxisRange()}};Plotly.newPlot('mainPlot',data,layout,{showSendToCloud:!0});" 

// Minified sessionId.js
#define jsSessionIdStringSource "var sessionId=document.getElementById('sessId').value;sessionStorage.setItem('sessionId',sessionId),document.getElementById('sessionIdSubmit').value=sessionId,document.getElementById('sessionIdSubmit').click();" 

ESP8266WebServer server (serverPort); // Creates web server object on port 303
HTTPClient http; // Creates http client object
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // Creates Adafruit_BME280 object

int sessionIdList[10]; // Stores 10 active Session IDs
int sessionIdPermission[10];
int pushNumberSessionId = 0; // Counts current index of sessionIdList and sessionIdPermission

// Whether or not thse pins (if used as outputs) are currently on/HIGH (true) or off/LOW (false) - default (if used as input is false)
bool stateD5 = false;
bool stateD6 = false;
bool stateD7 = false;
bool stateD8 = false;
bool stateLED_BUILTIN = true; // Some NodeMCU boards have the HIGH and LOW of LED_BUILTIN switched

// Various weather logging data
String dataRequested = "humidity"; // Data type to be sent to Plotly.js
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
	for (int i = 0; i < (sizeof(pastTemperature)/sizeof(*pastTemperature)); i++) {
		pastTemperature[i] = 0;
		pastPressure[i] = 0;
		pastHumidity[i] = 0;
		pastBrightness[i] = 0;
	}
	
}
// Populates above avgArr with 0s
void populateAvgArr() {
	for (int i = 0; i < (sizeof(avgTemperature)/sizeof(*avgTemperature)); i++) {
		avgTemperature[i] = 0;
		avgPressure[i] = 0;
		avgHumidity[i] = 0;
		avgBrightness[i] = 0;
	}
}
// Populates sessionIdList with random numbers and sessionIdPermission with 5 (logged out)
void populateSessId() {
	for (int i = 0; i < (sizeof(sessionIdList)/sizeof(*sessionIdList)); i++) {
		sessionIdList[i] = randNum(1,99999);
		sessionIdPermission[i] = 5;
	}
}

// General HTTP header pre-fixed to all server messages
// Includes: HTTP Header, Bootstrap, Plotly.js, Site Title
String bootstrapHeader = "<html lang='en'><head><meta http-equiv='refresh' name='viewport' content='width=device-width, initial-scale=1'/><script src='https://code.jquery.com/jquery-3.3.1.slim.min.js' integrity='sha384-q8i/X+965DzO0rT7abK41JStQIAqVgRVzpbzo5smXKp4YfRvH+8abtTE1Pi6jizo' crossorigin='anonymous'></script><script src='https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.14.3/umd/popper.min.js' integrity='sha384-ZMP7rVo3mIykV+2+9J3UJ46jBk0WLaUAdn689aCwoqbBJiSnjAK/l8WvCWPIPm49' crossorigin='anonymous'></script><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css' integrity='sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO' crossorigin='anonymous'> <script src='https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/js/bootstrap.min.js' integrity='sha384-ChfqqxuZUCnJSK3+MXmPNIyE6ZbWh2IMqE241rYiqJxyMiZ6OW/JmZQ5stwEULTy' crossorigin='anonymous'></script><script src='https://cdn.plot.ly/plotly-latest.min.js'></script><title>ESP8266 Home Server</title></head>";

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
  for (int i = 0; i < (sizeof(pastTemperature)/sizeof(*pastTemperature)); i++) {
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
  for (int i = 0; i < (sizeof(pastPressure)/sizeof(*pastPressure)); i++) {
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
  for (int i = 0; i < (sizeof(pastHumidity)/sizeof(*pastHumidity)); i++) {
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
  for (int i = 0; i < (sizeof(pastBrightness)/sizeof(*pastBrightness)); i++) {
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
	page += "<body>";
	if (useJsURLSource == true) { // Javascript Source
		page += "<script type='text/javascript' src='";
		page += jsSessionIdURLSource; 
		page += "'></script>";
	} else {
		page += "<script type='text/javascript'>";
		page += jsSessionIdStringSource; 
		page += "</script>";
	} 
	page += "</body>";
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
page += 									"Air Pressure (kPa)";
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
page += "<body>";
if (useJsURLSource == true) { // Javascript Source
	page += "<script type='text/javascript' src='";
	page += jsMainURLSource; 
	page += "'></script>";
} else {
	page += "<script type='text/javascript'>";
	page += jsMainStringSource; 
	page += "</script>";
}
page += "</body>";
  return page;
}

// Recieves and dispatches client requests based on kind of argument - Calls other handler functions and passes on the argument
// Basic POST format: name=server.hasArg(name) | value=server.arg(name)
// Updating Pins: name = pin number (D0,D1,etc.) | value = what to write (HIGH = 1, LOW = 0, other numbers can be programmed as patterns)
// Other arguments include "login", "username", "password" (for logging in) and "requestData" (for plotly data)
void handleRoot(){
	// Var to handle invalid requests
	bool validRequest = false;
	
	//Print sent session IDs
	for (int i = -1; i < 6; i++) {
		if (i == -1) {
			if (server.arg("sessionIdSubmit") != 0) {
				Serial.print("Session ID: "); Serial.print(server.arg("sessionIdSubmit")); Serial.println(" recieved.");
			}
		} else {
			String currSessId = "sessionIdSubmit" + String(i);
			if (server.arg(currSessId) != 0) {
				Serial.print("Session ID: "); Serial.print(server.arg(currSessId)); Serial.println(" recieved.");
			}
		}
	}
	
	// To log in
  if (server.hasArg("login")){
		validRequest = true;
		Serial.println("Login is requested");
    handleLogin();
  }
  
	// From session ID page
	if (server.hasArg("sessionIdSubmit") && validSessionId(99)) {
		validRequest = true;
		Serial.println("Session ID Page sent a request");
		 server.send (200,"text/html",getMainPage());
	}
	
  // From IFTTT Webhoook widget
  // <input name='webhookRequest' id='weatherUpdate'>
  if (server.hasArg("webhookRequest")) {
		validRequest = true;
		Serial.println("Webhook sent a request");
    if (server.arg("weatherUpdate")) {
      sendNotification("weather_update", String(currentTemperature), String(currentPressure), String(currentHumidity));
    }
  }
  
  if (server.hasArg("changeSettings") && validSessionId(1)) { // Level 1 auth to change settings
		validRequest = true;
		Serial.println("Settings sent a request");
    if (server.hasArg("requestSettings")) {
      String page;
      server.send (200,"text/html",page);
      
    }
  }
	
	if ((server.hasArg("requestData")) && (validSessionId(3))) { // Level 3 auth to request data
		validRequest = true;
		dataRequested = server.arg("requestData");
		Serial.print(dataRequested); Serial.println(" data is requested.");
		server.send (200,"text/html",getMainPage());
	}
	
	if ((server.hasArg("D5") || server.hasArg("D6") || server.hasArg("D7") || server.hasArg("D8") || server.hasArg("LED_BUILTIN")) && (validSessionId(2))) { // Level 2 auth to write to pins
		validRequest = true;
		Serial.println ("updatePin() request recieved with valid Session ID");
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
    }
	}
	
	if ((server.hasArg("restart")) && (validSessionId(1))) { // Level 1 auth to access web server settings
		validRequest = true;
		ESP.restart();
	}
	
	if ((server.hasArg("login") == false) && (validSessionId(99) == false)) { // Search for all auth codes
		validRequest = true; // For logged in users doing stuff out of range
		Serial.println ("No valid Session ID recieved");
		server.send (403,"text/html",getLoginPage());
	}
	
	if (validRequest == false) {
		Serial.println("Invalid request for permission level!");
		server.send (403,"text/html",getMainPage());
	}
	
	Serial.println("End handleRoot()");
}

// Returns true if a sessionId sent is in sessionIdList matches auth level
bool validSessionId(int authLevel) {
	bool returnVal = false;
	for (int i = 0; i < (sizeof(sessionIdList)/sizeof(*sessionIdList)); i++) {
		if ((String(sessionIdList[i]) == server.arg("sessionIdSubmit")) || (String(sessionIdList[i]) == server.arg("sessionIdSubmit0")) || (String(sessionIdList[i]) == server.arg("sessionIdSubmit1")) || (String(sessionIdList[i]) == server.arg("sessionIdSubmit2")) || (String(sessionIdList[i]) == server.arg("sessionIdSubmit3")) || (String(sessionIdList[i]) == server.arg("sessionIdSubmit4")) || (String(sessionIdList[i]) == server.arg("sessionIdSubmit5"))) {
			if (sessionIdPermission[i] <= authLevel) {
				returnVal = true;
			}
		}
	}
	return returnVal;
}

// Recieves "login" argument handleRoot() - hands client a session id
void handleLogin(){
  String inputUsername = server.arg("username"); // Argument value from username field
  String inputPassword = server.arg("password"); // Argument value from password field
	bool loggedIn = false;
	// Searches user/pass arrays for username password combo
	for (int i = 0; i < ((sizeof(authUsername)/sizeof(*authUsername))); i++) {
		if (inputUsername == authUsername[i] && inputPassword == authPassword[i]) { // Starts session and sends sessionId if credentials are valid
			loggedIn = true;
			int sessionId = randNum(1,99999); // Session ID is a random number
			sessionIdList[pushNumberSessionId] = sessionId;
			sessionIdPermission[pushNumberSessionId] = authPermission[i]; // Assigns auth permission to another array in index with session ID's
			Serial.print(inputUsername); Serial.println(" is successfully authenticated!");
			Serial.print("Session started with sessionId "); Serial.print(sessionId); Serial.print(" with permission level: "); Serial.println(authPermission[pushNumberSessionId]);
			if (pushNumberSessionId == 9) { // Max sessionId's stored is 10
				pushNumberSessionId = 0;
			} else {
				pushNumberSessionId++;
			}
			server.send (200,"text/html",getSessionIdPage(sessionId));
		}
	}
	// Redirects back to log in page with error 301 if no credentials are invalid
	if (loggedIn == false) {
		server.send (301,"text/html",getLoginPage());
		Serial.println("Invalid Credentials!");
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
  // Adapted BME280 example
	currentTemperature = ((bme.readTemperature() * (1.8)) + 32);
	currentPressure = bme.readPressure() / 1000.0F;
	currentHumidity = bme.readHumidity();
	
	//Pushes current data into average arrays
	avgTemperature[pushNumber] = currentTemperature;
	avgPressure[pushNumber] = currentPressure;
	avgHumidity[pushNumber] = currentHumidity;
	avgBrightness[pushNumber] = currentBrightness;
  pushNumber++;
	
  // If 10 minutes has elapsed, average values from avgArr to pastArr and move values in past weather arrays. Also does check notifications
  if (lastUpdate == 0) { // On start up
    pastTemperature[0] = currentTemperature;
    pastPressure[0] = currentPressure;
    pastHumidity[0] = currentHumidity;
		lastUpdate = millis();
		checkWeatherNotification();
		
  } else if (millis() - lastUpdate > 600000 ) { // every 10 minutes
		
    Serial.print("Updating Weather Data at "); Serial.println(millis());
    lastUpdate = millis();
		
		for (int i = ((sizeof(pastTemperature)/sizeof(*pastTemperature)) - 1); i > 0; i--) {
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
		checkWeatherNotification();
		
		// resets avg arrays
		populateAvgArr();
		pushNumber = 0;
  }
}

// Will only send alerts a maximum of once threshold crossing
String lastPressureAlert = "null";
String lastTemperatureAlert = "null";
String lastHumidityAlert = "null";
// Checks to see if notifications need to be called - calls sendNotification
void checkWeatherNotification() {
  // Low and high pressure
  if ((pastPressure[0] < alertLowPressure) &&(lastPressureAlert != "low")) {
		lastPressureAlert = "low";
    sendNotification("weather_update", "barometric+pressure+is+low", String(pastPressure[0])+"+kPa", "🌧️");
  } else if ((pastPressure[0] > alertHighPressure) &&(lastPressureAlert != "high")) {
		lastPressureAlert = "high";
    sendNotification("weather_update", "barometric+pressure+is+high", String(pastPressure[0])+"+kPa", "😎");
  } else if (pastPressure[0] > alertLowPressure && pastPressure[0] < alertHighPressure) { // resets lastPressureAlert if the value is normal
		lastPressureAlert = "null";
	}
	
	// Low and high temperature
	if ((pastTemperature[0] < alertLowTemperature) &&(lastTemperatureAlert != "low")) {
		lastTemperatureAlert = "low";
    sendNotification("weather_update", "temperature+is+low", String(pastTemperature[0])+"&deg;F", "☀️");
  } else if ((pastTemperature[0] > alertHighTemperature) &&(lastTemperatureAlert != "high")) {
		lastTemperatureAlert = "high";
    sendNotification("weather_update", "temperature+is+high", String(pastTemperature[0])+"&deg;F", "❄️");
  } else if (pastTemperature[0] > alertLowTemperature && pastTemperature[0] < alertHighTemperature) { // resets lastTemperatureAlert if the value is normal
		lastTemperatureAlert = "null";
	}
	
	// Low and high humidity
	if ((pastHumidity[0] < alertLowHumidity) &&(lastHumidityAlert != "low")) {
		lastHumidityAlert = "low";
    sendNotification("weather_update", "humidity+is+low", String(pastHumidity[0])+"%+rel.+humidity", "🌵");
  } else if ((pastHumidity[0] > alertHighHumidity) &&(lastHumidityAlert != "high")) {
		lastHumidityAlert = "high";
    sendNotification("weather_update", "humidity+is+high", String(currentHumidity)+"%+rel.+humidity", "💦");
  } else if (pastHumidity[0] > alertLowHumidity && pastHumidity[0] < alertHighHumidity) { // resets lastHumidityAlert if the value is normal
		lastHumidityAlert = "null";
	}
}

// Sends a GET request to IFTTT Maker Webhook based on notificationEvent, with 5 parameters maximum
void sendNotification(String notificationEvent, String value1, String value2, String value3) {
	for (int i = 0; i < (sizeof(webhookKeyList)/sizeof(*webhookKeyList)); i++) { // Does HTTP GET for each key in webhookKeyList[]
		// Creates specific webhook URL
		String webhookURL = "http://maker.ifttt.com/trigger/";
		webhookURL += notificationEvent;
		webhookURL += "/with/key/";
		webhookURL += webhookKeyList[i];
		webhookURL += "/?value1=";
		webhookURL += value1;
		webhookURL += "&value2=";
		webhookURL += value2;
		webhookURL += "&value3=";
		webhookURL += value3;
		// Starts http Client - thanks to https://techtutorialsx.com/2016/07/17/esp8266-http-get-requests/
		http.begin(webhookURL); // Specify request destination
    int httpCode = http.GET(); // Send the request
    if (httpCode > 0) { // Check the returning code
      String payload = http.getString(); // Get the request response payload
      Serial.println(payload); // Print the response payload
    } else {
      Serial.println("Calling IFTTT webhook failed.");
    }
    http.end(); // Close connection
	}
}

// Writes server status to Serial - Serial.print()
void serialUpdate() {
  Serial.println(""); Serial.println("=====");
  Serial.print("ESP8266 Status Update @ "); Serial.print(millis()/60000); Serial.println(" minutes");
  Serial.print("Temperature: "); Serial.print(currentTemperature); Serial.println("*F");
  Serial.print("Humidity: "); Serial.print(currentHumidity); Serial.println("%");
	Serial.print("Pressure: "); Serial.print(currentPressure); Serial.println("kPa");
	Serial.print("Current Data Requested: "); Serial.println(dataRequested);
  Serial.println("====="); Serial.println("");
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

// Initializes pins, serial, and server
void setup() {
	populateAvgArr(); // fills avg arrs with 0s
	populatePastArr(); // fills past arrs with 0s
	populateSessId(); // fills session id list with 1s

  // Initializing pins and serial
  pinMode(LED_BUILTIN,OUTPUT);
  Serial.begin (9600);
  
	// Initialzing BME280
	bool status;
  status = bme.begin();
		if (!status) {
			Serial.println("Could not find a valid BME280 sensor, check wiring!");
			while (1);
    }

	// Initializing WiFi
  WiFi.begin (ssid,password);
  Serial.print ("Connecting to ");
  Serial.print (ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay (250); Serial.print (".");
  }
  Serial.println ("");
  Serial.print ("Connected to "); Serial.println (ssid);
  Serial.print ("Local IP address: "); Serial.println (WiFi.localIP());

  // Initializing Web Server
  server.on ("/",handleRoot);
  server.begin();
  Serial.println ("HTTP server started.");
  Serial.println("=====");
}

// Main loop - updates weather and seiral periodically and handles clients
int weatherSkips = 30;
void loop() {
  if (weatherSkips == 30){ //updates weather only 4 times a minute
    updateWeather();
    weatherSkips = 0;
    serialUpdate();
		// checkWeatherNotification() moved INSIDE updateWeather()
  } else {
    weatherSkips++;
  }
  server.handleClient(); // Web server handles client
  delay(500);
}