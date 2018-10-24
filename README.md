# arduino-mqtt
A lightweight MQTT client library for arduino devices that supports all QoS levels. 

## Project Status

The code is stable.

The Example program is for an ESP32 device with an OLED. It probably won't run but it shows how to use the library.

## Useage

This is not set up to be a library, just add mqtt.h as a tab in your Arduino IDE project. 

You create a subclass of MQTTClient in your main application and override the virtual methods.

## Change Log

Oct, 2017 CONNECT, CONNACK, SUBSCRIBE, SUBACK and PUBLISH are working.

Nov, 2017 PINGREQ/PINGRESP working.  QoS1 and QoS2 PUBLISH messages working in both directions.

June 2018 I'm starting several projects based on mqtt.h so hopefully it can be tested and validated.

October 2018 I have several projects running this code for several months now with no issues.
