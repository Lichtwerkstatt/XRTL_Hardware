# IoT with ESP-32 Workshop

**Date** : January, 26th 2022  
**Time** : 5pm  
**Place** : Helmholtzweg 5 (Yellow Brick Building), Jena

## Outline
* Materials
* Preparation
* Content
  * Theory of ESP32
  * Establishing a [Wifi Access Point](#wifiAP)
  * Building a Webserver
  * Controlling an LED
  * Connecting to Wifi
  * Gather Data from the Internet
  * Connect to MQTT Server
  * Publish via MQTT
  * Subscribe and handle Messages
  * Free Working Time! \o/

## Materials 🛒

| Thing | Description | Link | Price |
|---|---|---|---|
| ESP32 | Microcontroller | [AZ-Delivery](https://www.az-delivery.de/en/products/esp32-developmentboard) | <10€
| μUSB cable | Connector Cable for ESP | | |
| Jumper | Dupont cables for building circuits | | |
| Breadboard | for connecting components | | |

## Preparation
Install something (todo.)

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
Serial.println("Hello World!");
```

### <a name="webserver"></a> Webserver

### <a name="controlLED"></a> Controlling a LED

### <a name="connectWiFi"></a> Connect to Wifi 

---
version 1.0  
Johannes Kretzschmar  
Jan, 2022