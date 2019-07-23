# arduino-mqtt
A lightweight MQTT client library for arduino devices that supports all QoS levels. 

## Project Status

The code is stable and I've been using it extensively in my home automation projects for over a year.

The Example program is for an ESP32 device with an OLED. It probably won't run but it shows how to use the library.

In the development branch:

1. I am converting this project to use the String class rather than C style strings
2. I have switched over to PlatformIO for development and so version 2 of this library will be a proper C style library

## Useage

This is not set up to be a library, I add the code from mqtt.h as a tab in an Arduino IDE project. 

Create a subclass of MQTTClient in your main application and override the virtual methods. See the example code.

## Change Log

Oct, 2017 CONNECT, CONNACK, SUBSCRIBE, SUBACK and PUBLISH are working.

Nov, 2017 PINGREQ/PINGRESP working.  QoS1 and QoS2 PUBLISH messages working in both directions.

June 2018 I'm starting several projects based on mqtt.h so hopefully it can be tested and validated.

October 2018 I have several projects running this code for several months now with no issues.

July 2019 Found a bug in the publish() method. Check that "flags |= 8" rather than "flags != 8"
