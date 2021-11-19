#include <Stepper.h>
#include "credentials.h"

#include <WiFi.h> //Wifi library
#include "esp_wpa2.h"
#include <BlynkSimpleEsp32.h>

// Encapsulated Function for hiding Wifi Credentials
const char ssid[] = WIFI_SSID;
const char usr[] = WIFI_USERNAME;
const char pass[] = WIFI_PASSWD;
char token[] = BLYNK_TOKEN;

int M1steps, M2steps;

const int stepsPerRevolution = 2048;
Stepper Step1(stepsPerRevolution, 17, 18, 5, 19);
Stepper Step2(stepsPerRevolution, 14, 26, 27, 25);
// switch IN2 and IN3 for Stepper 28BYJ-48
// see https://blog.thesen.eu/es-geht-doch-schrittmotor-28byj-48-mit-der-original-arduino-stepper-library/

WidgetTerminal terminal(V0);

BLYNK_WRITE(V0) {
  String input = param.asStr();
  int steps = input.substring(2,10).toInt();
  if (input.substring(0,1) == "1") {
      terminal.println("Moving Stepper 1 : "+String(steps)+" steps");
      Step1.step(steps);
    } else {
      terminal.println("Moving Stepper 2 : "+String(steps)+" steps");
      Step2.step(steps);
    }
  terminal.flush();
}

BLYNK_WRITE(V4) {
 M1steps = param.asInt();
 terminal.println("Set M1 Steps to "+String(M1steps));
 terminal.flush();
}

BLYNK_WRITE(V5) {
  terminal.print("Turning M1...");
  Step1.step(M1steps);
  terminal.println("done!");
}

BLYNK_WRITE(V6) {
  M2steps = param.asInt();
  terminal.println("Set M2 Steps to "+String(M2steps));
  terminal.flush();
}

BLYNK_WRITE(V7) {
  terminal.print("Turning M2...");
  Step2.step(M2steps);
  terminal.println("done!");
}

void setup() {
  // --- WIFI ---
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.print("Connecting to network: ");
  Serial.println(ssid);
  WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
  WiFi.mode(WIFI_STA); //init wifi mode
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)usr, strlen(usr)); //provide identity
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)usr, strlen(usr)); //provide username --> identity and username is same
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)pass, strlen(pass)); //provide password
  esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT(); //set config settings to default
  esp_wifi_sta_wpa2_ent_enable(&config); //set config settings to enable function
  WiFi.begin(ssid); //connect to wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address set: "); 
  Serial.println(WiFi.localIP()); //print LAN IP

  //--- BLYNK ---

   
  Blynk.config(token);
  Blynk.connect();  

  // --- HARDWARE ---
  Step1.setSpeed(15);
  Step2.setSpeed(15);
  Serial.begin(115200);
  Serial.println("Running...");

  terminal.clear();
  terminal.println("Lichtwerkstatt Mirror Motor Mount Test");
  terminal.println("--------------------------------------");
  terminal.flush();
}

void loop() {
  Blynk.run();
}
