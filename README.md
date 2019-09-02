# arduino-mqtt

An MQTT 3.1.1 client library for embedded devices that supports all QoS levels. 

## Version 1 (Master Branch) Status

The code in the master branch is version 1 code and is stable and I've been using it extensively in my home automation projects for over a year. It was designed with AVR processors in mind and has low memory use and tends away from dynamically allocated memory structures that tend to cause problems on AVR.

The Example program is for an ESP32 device with an OLED. It probably won't run but it shows how to use the library. In my projects I just add mqtt.h as a tab in the Arduino IDE and then creat a subclass of the object for use in my project. Your program must implement five events by implementing virtual methods. 

This is not the kind of Arduino library you install from Library Manager. 

## Version 2 (Development Branch) Status

I wound up using this code mostly on ESP8266 and ESP32 with higher memory requirements and better handling of dynamically allocated memory so version 2 is being redesigned. The code may be broken.

Goals for Version 2:

1. A proper library that can be installed from the Arduino Library Manager and conforms to the Ardunio Style Guide
2. Make better use of the Arduino core byte streaming library
3. Develop some working example programs
4. Convert the project to use the String class rather than C style strings for data and topics (to reduce memory use).
5. Use dynamically allocated memory for queues rather than statically allocated arrays (to reduce memory use).

## Useage

This is not set up to be a library, I add the code from mqtt.h as a tab in an Arduino IDE project. 

Create a subclass of MQTTClient in your main application and override the virtual methods. See the example code.

## Change Log

Oct, 2017 CONNECT, CONNACK, SUBSCRIBE, SUBACK and PUBLISH are working.

Nov, 2017 PINGREQ/PINGRESP working.  QoS1 and QoS2 PUBLISH messages working in both directions.

June 2018 I'm starting several projects based on mqtt.h so hopefully it can be tested and validated.

October 2018 I have several projects running this code for several months now with no issues.

July 2019 Found a bug in the publish() method. Check that "flags |= 8" rather than "flags != 8"

August 2019 Starting version 2
