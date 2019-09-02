#include "mqtt.h"

/* This is the most basic example possible. It reads A0 once per second and publishes the 
 * results as MQTT packets over the serial port. It will not run unless there happens
 * to be an MQTT server at the other end of the serial port.
 */

MQTTClient mqtt;
 
void setup() {
  Serial.begin(115200);
  mqtt.stream = &Serial;
  mqtt.connect("Example",NULL,NULL,true);
}

void loop() {
  static int c = 0;
  char buffer[11];
  
  if (Serial.available()) {
    mqtt.dataAvailable();
  } else {
    if (mqtt.isConnected) {
      c++;
      if (c==10) {
        mqtt.intervalTimer();
        c = 0;
        int a0 = analogRead(A0);
        mqtt.publish("Example/A0",itoa(a0,buffer,10),qtAT_MOST_ONCE,true,false));
      }
    }
    delay(100);
  }
}