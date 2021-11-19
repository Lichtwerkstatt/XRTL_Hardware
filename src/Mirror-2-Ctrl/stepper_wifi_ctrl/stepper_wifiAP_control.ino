/*
  Wifi Access Point for Mirror Control (XRTwinLab)

  - connect to "Mirror_Stepper-AP" Wifi Network (no password required)
  - goto http://192.168.4.1 and use HTML form or Debug links
  - use HTTP request http://192.168.4.1/turn?motor=1&steps=1000 for 1000 steps on Motor1 
  - run raw TCP "GET turn?motor=1&steps=1000" on Terminal with 192.168.4.1 Port 80

  Written by J. Kretzschmar (Lichtwerkstatt)
  based on
  Created for arduino-esp32 on 04 July, 2018
  by Elochukwu Ifediora (fedy0)
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <Stepper.h>

// Set these to your desired credentials.
const char *ssid = "Mirror_Stepper_AP";

WiFiServer server(80);

const int stepsPerRevolution = 2048;
Stepper Step1(stepsPerRevolution, 17,18,5,19);
Stepper Step2(stepsPerRevolution, 14,26,27,25);


void setup() {

  Serial.begin(115200);
  Serial.println();

  Step1.setSpeed(15);
  Step2.setSpeed(15);
  
  Serial.println("Configuring access point...");

  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  
  Serial.println("Server started");
}

void loop() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                            
    Serial.println("New Client.");         
    String currentLine = "";                
    while (client.connected()) {            
      if (client.available()) {             
        char c = client.read();            
        Serial.write(c);                    
        if (c == '\n') {                    
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            client.print("<html><body><style>* {margin:10px;}</style><h1>XRTwinLab</h1><h3>HTTP Mirror Control Test</h3><p>");
            client.print("<a href=\"/\">Home</a>");

            client.print("<form action=\"turn\" method=\"get\" style=\"width:400px;\"><fieldset>");
            client.print("<p>Select a Motor to control:</p><input type=\"radio\" id=\"m1\" name=\"motor\" value=\"1\"  checked=\"checked\" /><label for=\"m1\">Motor 1</label>");
            client.print("<input type=\"radio\" id=\"m2\" name=\"motor\" value=\"2\" /><label for=\"m2\">Motor 2</label></fieldset>");
            client.print("<fieldset><input type=\"range\" id=\"value\" name=\"steps\" min=\"-1000\" max=\"1000\"><label for=\"value\">Steps to Turn</label></fieldset>");
            client.print("<input type=\"submit\" value=\"Go Brrrrrm!\"/></form>");

            client.print("<h4>Debug</h4>");
            client.print("<a href=\"/turn?motor=1&steps=1000\">M1 1000</a><br/>");
            client.print("<a href=\"/turn?motor=1&steps=-1000\">M1 -1000</a><br/>");
            client.print("<a href=\"/turn?motor=2&steps=1000\">M2 1000</a><br/>");
            client.print("<a href=\"/turn?motor=2&steps=-1000\">M2 -1000</a><br/>");
            client.print("</p><i>Lichtwerkstatt Jena, 2020</i></body></html>");
            client.println();
            break;
          } else {    
            if (currentLine.startsWith("GET")){
              Serial.println(">> GET REQUEST FOUND: "+currentLine);  
              String getCmnd = currentLine.substring(currentLine.indexOf(" ")+1); // Cut "GET"
              getCmnd = getCmnd.substring(0,getCmnd.indexOf(" ")); // Cut HTTP Version
              if (getCmnd.startsWith("/turn?")) {
                int motor = getCmnd.substring(getCmnd.indexOf("motor=")+6,getCmnd.indexOf("&")).toInt();
                int steps = getCmnd.substring(getCmnd.indexOf("steps=")+6).toInt();
                Serial.println(">> Motor: "+String(motor)+" ,Steps:"+String(steps));
                if (motor == 1){
                  Serial.println(">> Running Motor1");
                  Step1.step(steps);
                } else if (motor == 2){
                  Serial.println(">> Running Motor2");
                  Step2.step(steps);
                } else {
                  Serial.println(">> ERROR: undefined motor");
                }
              }
            }
            currentLine = "";   
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }      
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
