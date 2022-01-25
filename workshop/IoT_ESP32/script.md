# IoT with ESP-32 Workshop

**Date** : January, 26th 2022  
**Time** : 5pm  
**Place** : Helmholtzweg 5 (Yellow Brick Building), Jena

## Outline
* [Materials](#Materials)
* [Preparation](#Preparation)
* [Tutorials](#Tutorials)
* Content
  * Theory of ESP32
  * Establishing a [Wifi Access Point](#wifiAP)
  * Building a [Webserver](#webserver)
  * [Controlling](#controlLED) an LED
  * [Connecting](#connectWiFi) to Wifi
  * Gather Data from the Internet
  * Connect to MQTT Server
  * Publish via MQTT
  * Subscribe and handle Messages
  * Free Working Time! \o/ (Complete [MQTT Example](#overallMQTT))

## Materials 🛒

| Thing | Description | Link | Price |
|---|---|---|---|
| ESP32 | Microcontroller | [AZ-Delivery](https://www.az-delivery.de/en/products/esp32-developmentboard) | <10€
| μUSB cable | Connector Cable for ESP | | |
| Jumper | Dupont cables for building circuits | | |
| Breadboard | for connecting components | | |

## Preparation
* install [Arduino IDE](https://www.arduino.cc/en/software)
* install ESP32 libraries following this [Guide](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/)
* TroubleShooting [Guide](https://randomnerdtutorials.com/esp32-troubleshooting-guide/)

## <a name="Tutorials"></a> Tutorials and Ressources 
* MQTT [Tutorial](https://www.survivingwithandroid.com/esp32-mqtt-client-publish-and-subscribe/) with ESP32 and HiveMQ
* MQTT [Tutorial](https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/) with ESP32
* ESP-32 [PinOut](https://cdn.shopify.com/s/files/1/1509/1638/files/ESP-32_NodeMCU_Developmentboard_Pinout.pdf?v=1609851295)

## Source Code

### Buildplate
```c#
void setup() {
  Serial.begin(115200); 
}

void loop() {

}
```

### <a name="wifiAP"></a> Wifi Access Point 
```c#
#include <WiFi.h>

const char* ssid     = "YourSSID";
const char* password = "123456789";

//SETUP

Serial.print("Setting AP (Access Point)…");
WiFi.softAP(ssid, password);

IPAddress IP = WiFi.softAPIP();
Serial.print("AP IP address: ");
Serial.println(IP);
```

### <a name="webserver"></a> Webserver
~~~c#
#include <WebServer.h>

WebServer server(80);

//SETUP
server.on("/",handle_root);
server.begin();
Serial.println("HTTP Server started...");


//LOOP
server.handleClient();

void handle_root(){
  String HTML = "<!DOCTYPE html><html>"
  "<body><b>HELLO WORLD!</b></body></html>";
  server.send(200, "text/html", HTML);
}
~~~

### <a name="controlLED"></a> Controlling a LED
~~~c#

bool LED = false;

//SETUP
pinMode(4, OUTPUT);
digitalWrite(4, LED);

server.on("/switch",switch_LED)

String   String HTML = "<!DOCTYPE html><html>"
  "<body><b>HELLO WORLD!</b><br/>"
  "<a href=\"/switch\">SWITCH!</a>"
  "</body></html>";

void handle_root(){

  server.send(200, "text/html",HTML );

void switch_LED(){
  LED = !LED;
  digitalWrite(4, LED);
  server.send(200, "text/html",HTML );
}

}

~~~
#### **Task:**
* Implement a feedback about LED status on the website
* Different Buttons for ON and OFF
* RGB Color LED

### <a name="connectWiFi"></a> Connect to Wifi 
~~~c#

#include <WiFi.h>
const char *SSID = ""; // Enter your Wifi SSID here
const char *PWD = ""; // Enter your WifiPassword Here

void connect2Wifi(){
	  Serial.print ("Connecting to "); Serial.println(SSID);
	  WiFi.begin(SSID,PWD);
	  while(WiFi.status() != WL_CONNECTED) {
	    Serial.print(".");
	    delay(500);
	  }
	  Serial.print("Connected to Wifi AP.");
	}

//SETUP
connect2Wifi();
~~~

### <a name="gatherData"></a> Gather Data from the Internet 
~~~
~~~
*  [Tutorial](https://esp32io.com/tutorials/esp32-http-request) HTTP by hand


### <a name="connectMQQT"></a> Connect to MQTT Server
~~~c#
#include <PubSubClient.h>

PubSubClient mqttClient(wifiClient);

char *mqttServer = "SERVER";
int mqttPort = PORT;
char *mqttUser = LOGIN;
char *mqttPwd = PWD;

//SETUP
mqttClient.setServer(mqttServer, mqttPort);

//LOOP
  if (!mqttClient.connected())
    reconnect();
  mqttClient.loop();

//RECONNECT
void reconnect(){
  Serial.println("Connecting to MQTT Broker...");
  while(!mqttClient.connected()) {
    Serial.println("Reconnecting...");
    String clientId = "    ";
    if (mqttClient.connect(clientId.c_str(), mqttUser, 	mqttPwd)) {
      Serial.println("Connected."); }
  }
}
~~~

### <a name="publishData"></a> Publish Data on MQTT
~~~c#
char payload[10];
long now = millis();
if (now - last_time > 10000){
  int i = random(0,5000);
  sprintf(payload, "%d", i);
  mqttClient.publish("/randomValue", payload);
  Serial.println(payload); 
  last_time = now;    
}
~~~

### <a name="subscribeData"></a> Subscribe to Topic
~~~c#
//SETUP
mqttClient.subscribe("bumsi/debug");
mqttClient.setCallback(callback);

//CALLBACK
void callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Callback - ");
  Serial.print("Message: ");
  for (int i=0; i<length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println(". END of Message!");
}
~~~


### <a name="overallMQTT"></a> MQTT complete example
~~~c#
#include <Arduino.h>
	#include <WiFi.h>
	#include <PubSubClient.h>

	const char *SSID = ""; // Enter your Wifi SSID here
	const char *PWD = ""; // Enter your WifiPassword Here

	WiFiClient wifiClient;
	PubSubClient mqttClient(wifiClient);
	char *mqttServer = ""; // Enter MQTT Server here
	int mqttPort = 0; // Enter MQTT Port here
	char *mqttUser = ""; // Enter MQTT Username here
	char *mqttPwd = ""; // Enter MQTT Password here

	void connect2Wifi(){
	  Serial.print ("Connecting to "); Serial.println(SSID);
	  WiFi.begin(SSID,PWD);
	  while(WiFi.status() != WL_CONNECTED) {
	    Serial.print(".");
	    delay(500);
	  }
	  Serial.print("Connected to Wifi AP.");
	}

	void reconnect(){
	  Serial.println("Connecting to MQTT Broker...");
	  while(!mqttClient.connected()) {
	    Serial.println("Reconnecting...");
			 // Enter Client ID (must be different from anyone else!!!) here
	    String clientId = "";
	    if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPwd)) {
	      Serial.println("Connected to MQTT Broker.");
	    }
	  }
	}

	void callback(char* topic, byte* payload, unsigned int length){
	  Serial.print("Callback - ");
	  Serial.print("Message: ");
	  for (int i=0; i<length; i++) {
	    Serial.print((char)payload[i]);
	  }
	  Serial.println(". END of Message!");
	}

	void setup() {
	  Serial.begin(9600);
	  connect2Wifi();
	  mqttClient.setServer(mqttServer, mqttPort);
	  mqttClient.setCallback(callback);
	}

	void loop() {
	  if (!mqttClient.connected())
	    reconnect();
	  mqttClient.loop();
	}
~~~

---
version 1.0  
Johannes Kretzschmar  
Jan, 2022