// SESSiON ID STUFF - sends to server
let sessionId = sessionStorage.getItem('sessionId');
sessionId = sessionId.toString();

document.getElementById('sessionIdSubmit0').value = sessionId;
console.log(document.getElementById('sessionIdSubmit0').value);

document.getElementById('sessionIdSubmit1').value = sessionId;
console.log(document.getElementById('sessionIdSubmit1').value);

document.getElementById('sessionIdSubmit2').value = sessionId;
console.log(document.getElementById('sessionIdSubmit2').value);

document.getElementById('sessionIdSubmit3').value = sessionId;
console.log(document.getElementById('sessionIdSubmit3').value);

document.getElementById('sessionIdSubmit4').value = sessionId;
console.log(document.getElementById('sessionIdSubmit4').value);

document.getElementById('sessionIdSubmit5').value = sessionId;
console.log(document.getElementById('sessionIdSubmit5').value);

// PLOTLY STUFF
// Gets the div element for the plot
const mainPlot = document.getElementById('mainPlot');

// Gets time data to plot - fix when new .ino is flashed
const dataKeep = 144 //document.getElement.Id('dataKeep').value;

// Arrays to be filled with hidden HTML elements sent by server (index0 = 10hrs ago; index9 = now)
let pastTemperature = [];
let pastPressure = [];
let pastHumidity = [];

let dataRequested = document.getElementById('dataRequested').value;

if (dataRequested == 'temperature'){
	// Populates pastTemperature
	for (i = 0; i < dataKeep; i++){
		let elementID = 'temperature' + String(i);
		let tempTemp = document.getElementById(elementID).value;
		tempTemp = Number(tempTemp);
		pastTemperature.push(tempTemp);
	}
} else if (dataRequested == 'pressure'){
	// Populates pastPressure
	for (i = 0; i < dataKeep; i++){
		let elementID = 'pressure' + String(i);
		let tempPres = document.getElementById(elementID).value;
		tempPres = Number(tempPres);
		pastPressure.push(tempPres);
	}
} else if (dataRequested == 'humidity'){
	// Populates pastHumidity
	for (i = 0; i < dataKeep; i++){
		let elementID = 'humidity' + String(i);
		let tempHum = document.getElementById(elementID).value;
		tempHum = Number(tempHum);
		pastHumidity.push(tempHum);
	}
} else if (dataRequested == 'brightness'){
	// Populates 	pastBrightness
	for (i = 0; i < dataKeep; i++){
		let elementID = 'brightness' + String(i);
		let tempBri = document.getElementById(elementID).value;
		tempBri = Number(tempBri);
		pastBrightness.push(tempBri);
	}
}

// Still need to populate pastBrightness and add it to the arduino code


// All of this creates the array "hourArrayNames" that fills with labels for time (once an hour) - right now for ((60/weatherBin)*dataKeep) = 144
const hourString = ['12am','1am','2am','3am','4am','5am','6am','7am','8am','9am','10am','11am','12pm','1pm','2pm','3pm','4pm','5pm','6pm','7pm','8pm','9pm','10pm','11pm']
let today = new Date();
let hour = today.getHours(); //0-23
let hourArrayNames = [];
let hourLeftover = 23 - hour; // change the 23 for differnt time periods but literally dont know how it would go past 24 hrs (for loops in for loops?)
for (i=hour;i>-1;i--){
  hourArrayNames.push(hourString[i]);
}
for (i=23;i>(23-hourLeftover);i--){
  hourArrayNames.push(hourString[i]);
}
let hourArray = [];
for (i = 0; i < dataKeep; i++) {
	hourArray[i] = i;
}

// Creates an array with 0 - 144 but one every 6 - have to do math with variables later
// Starts based on how many minutes has passed ^ min (0-59) = ^ subtract % time (1 - 6)
let minute = today.getMinutes();
let hourArrayVal = [];
let roundMin = Math.round(minute/10);
for (i = 0; i < 144; i++) {
  if ((i - roundMin) % 6 === 0) {
    hourArrayVal.push(i)
  }
}


// Creates objects to make plot
let Inside;
if (dataRequested == 'temperature'){
  Inside = {
    x: hourArray,
    y: pastTemperature,
    type: 'scatter',
  };
} else if (dataRequested == 'pressure'){
  Inside = {
    x: hourArray,
    y: pastPressure,
    type: 'scatter',
  };
} else if (dataRequested == 'humidity'){
  Inside = {
    x: hourArray,
    y: pastHumidity,
    type: 'scatter',
  };
} else if (dataRequested == 'brightness'){
  Inside = {
    x: hourArray,
    y: pastBrightness,
    type: 'scatter',
  };
}
let data = [Inside];

// Sets up label for Y Axis
function getYAxis() {
	switch(dataRequested) {
  case "temperature":
    return "Temperature (&#176;C)";
	case "pressure":
		return "Air Pressure (kPa)";
  case "humidity":
    return "Humidity (rel. %)";
  case "brightness":
    return "Brightness (A.U.)";
	}
}

// Sets dimensions of plot
let navbarWidth = document.getElementById("weatherNavbar").offsetWidth;
let tableHeight = document.getElementById("tableDiv").offsetHeight;

// Sets dimensions of Y axis
function yAxisRange() {
	switch(dataRequested) {
  case "temperature":
    return [0, 40];
	case "pressure":
		return [80, 120];
  case "humidity":
    return [0, 100];
  case "brightness":
    return [0, 1023];
	}
}

// Does axis stuff - look it up https://plot.ly/javascript/axes/#toggling-axes-lines-ticks-labels-and-autorange
var layout = {
  autosize: true,
  width: (navbarWidth - 25),
	height: (tableHeight),
  /*margin: {
    r: 50,
    pad: 4
  },*/
  xaxis: {autorange: 'reversed', tickvals: hourArrayVal, ticktext: hourArrayNames},
  yaxis: {title: getYAxis(), range: yAxisRange()}
	};

Plotly.newPlot('mainPlot', data, layout, {showSendToCloud: true});