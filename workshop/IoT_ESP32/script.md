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
  * Connecting to Wifi
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

## Tutorials
* MQTT [Tutorial](https://www.survivingwithandroid.com/esp32-mqtt-client-publish-and-subscribe/) with ESP32 and HiveMQ
* MQTT [Tutorial](https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/) with ESP32

## Source Code

### Buildplate
```
void setup() {
  Serial.begin(115200); 
}

void loop() {

}
```

### <a name="wifiAP"></a> Wifi Access Point 
```
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
~~~
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
~~~

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

### <a name="overallMQTT"></a> MQTT complete example
~~~
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