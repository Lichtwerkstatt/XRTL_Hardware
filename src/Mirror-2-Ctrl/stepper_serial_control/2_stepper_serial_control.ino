#include <Stepper.h>

const int stepsPerRevolution = 2048;
Stepper Step1(stepsPerRevolution, 17, 18, 5, 19);
Stepper Step2(stepsPerRevolution, 14, 26, 27, 25);
// switch IN2 and IN3 for Stepper 28BYJ-48
// see https://blog.thesen.eu/es-geht-doch-schrittmotor-28byj-48-mit-der-original-arduino-stepper-library/

void setup() {
  Step1.setSpeed(15);
  Step2.setSpeed(15);
  Serial.begin(115200);
  Serial.println("Running...");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readString();
    Serial.println(input);
    Serial.println(input.substring(2,10));
    int steps = input.substring(2,10).toInt();
    
    if (input.substring(0,1) == "1") {
      Serial.println("Stepper1 : "+String(steps));
      Step1.step(steps);
    } else {
      Serial.println("Stepper2 : "+String(steps));
      Step2.step(steps);
    }

    /*
    String input = Serial.readString();
    int steps = input.toInt();
    Serial.println("Doing Steps: " + String(steps));
    Step2.step(steps);
    */
  }
}
