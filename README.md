# ESP8266-Home-Automation-Server
A basic web server on the ESP8266 chip with some command sending and weather tracking functionality.

## Hardware
#### Materials
1. ESP8266 12E NodeMCU board
2. BME280 temperature, humidity, and pressure sensor
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
3. `serverPort` is the router port the server will be hosted on.
4. `authUsername` is the username used to log into the server.
5. `authPassword` is the password used to log into the server. This is very insecure, so don't reuse any passwords.
6. `BME_SCK`
   `BME_MISO`
   `BME_MOSI`
   `BME_CS` are pins for the BME280
7. `jsMainSource` `jsSessionSource` are explained further below

#### Pointing to the JavaScript File
The webserver sends a couple of JavaScript files to the client in order to plot charts and do a variety of other functions. The file can be served in two ways.
1. **Pointing to the GitHub** - change `useJsURLSource` to `true` to use scripts hosted by Github.
2. **Minifying the scripts** - change `useJsURLSource` to `false` in order to used the minified scripts included in the .ino

#### Setting up IFTTT Integration
This webserver uses the Maker Webhooks provided by IFTTT.
1. Add this applet: https://ifttt.com/applets/95730778d-home-server-weather-notification to your IFTTT account
2. Get your Webhook key at: https://ifttt.com/services/maker_webhooks/settings. It's the string of alphanumeric following `https://maker.ifttt.com/use/` under Account Info and URL
3. Set the Webhook key as an entry under `webhookKeyList[1]`. Notifications can be sent to multiple users if the array is extended with keys from multiple users.
4. Change `alertLowPressure`, `alertLowPressure`, `alertLowTemperature`, `alertHighTemperature`, `alertLowHumidity`, and `alertHighHumidity` to match the parameters that match your needs. (units are in kPa, Fahrenheit, and % relative humidity, respectively)

#### Installing Boards and Libraries
To use the ESP8266 board with the Arduino IDE open up the IDE and go to `File > Preferences > Additional Boards Manager URLs` and paste in this URL: http://arduino.esp8266.com/versions/2.5.0/package_esp8266com_index.json kindly provided by the ESP8266 Community Forum. Restart the Arduino IDE and go to `Tools > Board > Boards Manager` search for "ESP8266 and click install. Now, in `Tools > Board` select the `NodeMCU 1.0 (ESP-12E Module)` board. Set the upload speed and CPU frequency to your board's (it's usually 115200 and 80 MHz, respectively).

The web server needs the following SimpleDHT library not pre-installed in the Arduino IDE or the ESP8266 package. The library by WinLin can be found here: https://github.com/winlinvip/SimpleDHT It can be installed similarly to other Arduino libraries by downloading the ZIP and going to `Sketch > Include Library > Add .ZIP Library...` and opening the ZIP file. Make sure it's installed in `Sketch > Include Library > Manage Libraries`

#### Flashing the Board
The ESP8266 board can now be plugged in and the sketch can be uploaded to the board as normal. The board's activity can be monitored in the Serial Window at 9600 baud.

#### Port Fowarding
The exact protocol will vary based on the model of the router, but the basic procedure remains the same. Accessing the router through a web browser will show the internal IP of the web server. From there, the internal IP should be reserved, so the same IP always belongs to the device. In addition, the port specified in `serverPort` should be forwarded with the internal IP, allowing the server to be accessed from your external IP. The external IPv4 address can be found from sites such as `https://www.whatismyip.com/`. The ESP8266 web server can then be accessed from `[external IP]:[port]`

## TODO
1. Add photoresistor functionality
2. Implement a more secure authenication system 
3. Include instrucions for serving custom Boostrap HTML/CSS pages
4. Increase page loading speeds
