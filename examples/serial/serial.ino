#include "mqtt.h"

/** @brief   This is the most minimalistic example possible. 
 *  @details It reads A0 once per second and publishes the results as an MQTT packet over 
 *           the serial port. It will not run unless there happens to be an MQTT server 
 *           listening to the other end of the serial port.
 */

MQTTClient mqtt(&Serial);

char data[10];
const String topic("Example/A0");
const String clientid("Example");
const String username("");
const String password("");

void setup() {
  Serial.begin(115200);
  mqtt.connect(clientid,username,password,true);
}

void loop() {
  static int c = 0;

  if (Serial.available() > 1) {
    mqtt.dataAvailable();     // This needs to be called when a useful amount of serial 
                              // data is available in the serial buffer
  } else {
    c++;
    if (c==10) {
      c = 0;
      mqtt.intervalTimer();   // This needs to be called approximately once per second
                              // irregardless of whether the client is connected
      if (mqtt.isConnected) {        
        itoa(analogRead(A0),data,9);
        mqtt.publish(topic,(const byte *)data,strlen(data),qtAT_MOST_ONCE);
      }
    }  
    delay(95);
  }
}