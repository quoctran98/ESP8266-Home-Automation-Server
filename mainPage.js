// Gets the div element for the plot
const mainPlot = document.getElementById('mainPlot');

// Arrays to be filled with hidden HTML elements sent by server (index0 = 10hrs ago; index9 = now)
let pastTemperature = [];
let pastPressure = [];
let pastHumidity = [];

let dataRequested = document.getElementById('dataRequested').value;

// Populates pastTemperature
for (i = 0; i < 9; i++){
  let elementID = 'temperature' + String(i);
  let tempTemp = document.getElementById(elementID).value;
  tempTemp = Number(tempTemp);
  pastTemperature.push(tempTemp);
}

// Populates pastPressure
for (i = 0; i < 9; i++){
  let elementID = 'pressure' + String(i);
  let tempPres = document.getElementById(elementID).value;
  tempPres = Number(tempPres);
  pastPressure.push(tempPres);
}

// Populates pastHumidity
for (i = 0; i < 9; i++){
  let elementID = 'humidity' + String(i);
  let tempHum = document.getElementById(elementID).value;
  tempHum = Number(tempHum);
  pastHumidity.push(tempHum);
}

// Still need to populate pastBrightness and add it to the arduino code

// All of this creates the array "hourArray" that fills with labels for time
const hourString = ['12am','1am','2am','3am','4am','5am','6am','7am','8am','9am','10am','11am','12pm','1pm','2pm','3pm','4pm','5pm','6pm','7pm','8pm','9pm','10pm','11pm']
let today = new Date();
let hour = today.getHours(); //0-23
let hourArray = [];
let hourLeftover = 9 - hour;
for (i=hour;i>-1;i--){
  hourArray.push(hourString[i]);
}

for (i=23;i>(23-hourLeftover);i--){
  hourArray.push(hourString[i]);
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
    break;
	case "pressure":
		return "Air Pressure (kPa)";
    break;
  case "humidity":
    return "Humidity (rel. %)";
    break;
  case "brightness":
    return "Brightness (A.U.)";
    break;
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
    break;
	case "pressure":
		return [80, 120];
    break;
  case "humidity":
    return [0, 100];
    break;
  case "brightness":
    return [0, 1023];
    break;
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
  xaxis: {autorange: 'reversed', tickvals: hourArray},
  yaxis: {title: getYAxis(), range: yAxisRange()}
	};

Plotly.newPlot('mainPlot', data, layout, {showSendToCloud: true});