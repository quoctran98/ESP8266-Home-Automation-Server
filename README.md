# ESP8266_WebServer
A basic web server on the ESP8266 chip with some command sending and weather tracking functionality.

## Hardware
#### Materials
1. ESP8266 12E NodeMCU board
2. DHT11 (3 pins is preferred - no instructions are provided to wire up a 4 pin DHT11)
3. Various duPont wires and a breadboard (or some alternate way of wiring up the circuit)

#### Wiring
1. Connect the board's `GND` to the DHT11's `GND`
2. Connect the board's `3v3` to the DHT11's `VCC`
3. Connect the board's pin `D6` (or another pin) to the DHT11's `DATA`

## Installation
#### Defining Variables
Before doing anything, we'll need to go into the espe8266_webserver.ino and define some variables. 

1. `ssid` is the name of your WiFi network. Keep in mind that most ESP8266 12E NodeMCU chips can only access the 2.4GHz band.
2. `password` is the WiFi password. I don't know exactly what WiFi security types this library works with but I know that WPA2 is okay.
3. `authUsername` is the username used to log into the server.
4. `authPassword` is the password used to log into the server. This is very insecure, so don't reuse any passwords.
5. `int pinDHT11` is the pin connected to the DATA pin of the DHT11. The default pin is 12 or D7 (the pins may not match up).

#### Pointing to the JavaScript File
The webserver sends a JavaScript file to the client in order to plot charts and do a variety of other functions. The file can be served in a number of ways.
1. **Hosting it on your own sever** - change `jsSource` to the URL of the file. This is preferred if a webserver you control is available because it allows for editing the JavaScript file on the fly
2. **Pointing to the GitHub** - change `jsSource` to https://raw.githubusercontent.com/quoctran98/ESP8266_WebServer/master/mainPage.js
3. **Minifying the script** - changing all `"` to `'` and minifying the script will allow it to be served as a string directly from the webserver. This requires some poking around the .ino file itself and changing the `getMainPage()` function. Future releases may include an alternate .ino file with this option.

#### Installing Boards and Libraries
To use the ESP8266 board with the Arduino IDE open up the IDE and go to `File > Preferences > Additional Boards Manager URLs` and paste in this URL: http://arduino.esp8266.com/versions/2.5.0/package_esp8266com_index.json kindly provided by the ESP8266 Community Forum. Restart the Arduino IDE and go to `Tools > Board > Boards Manager` search for "ESP8266 and click install. Now, in `Tools > Board` select the `NodeMCU 1.0 (ESP-12E Module)` board. Set the upload speed and CPU frequency to your board's (it's usually 115200 and 80 MHz, respectively).

The web server needs the following SimpleDHT library not pre-installed in the Arduino IDE or the ESP8266 package. The library by WinLin can be found here: https://github.com/winlinvip/SimpleDHT It can be installed similarly to other Arduino libraries by downloading the ZIP and going to `Sketch > Include Library > Add .ZIP Library...` and opening the ZIP file. Make sure it's installed in `Sketch > Include Library > Manage Libraries`

#### Flashing the Board
The ESP8266 board can now be plugged in and the sketch can be uploaded to the board as normal. The board's activity can be monitored in the Serial Window at 9600 baud.

## TODO
1. Add barometer and photoresistor functionality
2. Actually implement real log in and authentication system
3. Include instrucions for serving custom Boostrap HTML/CSS pages
