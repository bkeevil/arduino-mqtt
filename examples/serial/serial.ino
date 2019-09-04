#include "mqtt.h"

/* This is the most minimalistic example possible. It reads A0 once per second and publishes the 
 * results as an MQTT packet over the serial port. It will not run unless there happens
 * to be an MQTT server listening to the other end of the serial port.
 */

MQTTClient mqtt(&Serial);
 
void setup() {
  mqtt.connect("Example","","",true);
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
        mqtt.publish("Example/A0",(byte *)itoa(a0,buffer,10),strlen(buffer),qtAT_MOST_ONCE,true);
      }
    }
    delay(100);
  }
}
