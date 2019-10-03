# Bond's MQTT 3.1.1 Client Library for Arduino {#mainpage}

An MQTT 3.1.1 client library for embedded devices that supports all QoS levels.

## Version 1 (Master Branch) Status

The code in the master branch is version 1 code and is stable and I've been using it extensively in my home automation projects for over a year. It was designed with AVR processors in mind and has low memory use and tends away from dynamically allocated memory structures that tend to cause problems on AVR.

The Example program is for an ESP32 device with an OLED. It probably won't run but it shows how to use the library. In my projects I just add mqtt.h as a tab in the Arduino IDE and then creat a subclass of the object for use in my project. Your program must implement five events by implementing virtual methods.

This is not the kind of Arduino library you install from Library Manager.

## Version 2 (Development Branch) Status

Version 2 is under development in the development branch. Its not yet ready for production use.

Goals for Version 2:

1. A proper library that can be installed from the Arduino Library Manager
2. Write a library that better conforms to the Ardunio Style Guide
3. Make better use of the Arduino core streaming library classes Print and Printable
4. Develop better example programs
5. Convert the project to use the String class rather than C style strings for data and topics.
6. Use dynamically allocated memory for queues rather than statically allocated arrays.

Version 2 will have some minor interface changes.

## Useage

Create a subclass of MQTTClient in your main application. Your application can respond to MQTT events by overriding any of the following virtual methods:

``` c++
#include "mqtt.h"

class MyMQTTClient: public MQTTClient {
  public:
    void connected() override;
    void initSession() override;
    void subscribed(const word packetID, const byte resultCode) override;
    void unsubscribed(const word packetID) override;
    void receiveMessage(const MQTTMessage& msg) override;
};

MyMQTTClient mqtt(WiFiClient);
```

Your main application loop needs to call `mqtt.dataAvailable()` in response to incomming network data:

``` c++
void loop() {
  if (WiFiClient.available() > 1) {
    mqtt.dataAvailable();
  } else {
    ...
  }
}
```

It also needs to call `mqtt.interval()` once every second or so. This method keeps track of packet queues and retransmits any packets that timeout:

``` c++
void loop() {
  { Do other stuff }
  c++;
  if (c==10) {
    c = 0;
    mqtt.intervalTimer();
  }
  delay(100);
}
```

Subscriptions are best done by overriding `mqtt.initSession()`. initSession is called after a client connects to the server but no existing session could be re-established:

``` c++
void MyMQTTClient::initSession() {
  Serial.println("Initializing Subscriptions");
  mqtt.subscribe(1,"System/#",qtEXACTLY_ONCE);  
  MQTTClient::initSession();
}
```

Topic strings are String objects that are passed by reference. Topic strings can also be passed as a literal string. Data can be passed as a String object, String literal, as a byte array, or as a character buffer.

``` c++
const String someTopic("Topic/1");
const String someData("someData");
const byte dataBuffer[20];
const char strBuffer[20];

mqtt.publish(someTopic,someData,qtAT_MOST_ONCE);
mqtt.publish("Topic/2","1",qtEXACTLY_ONCE);
mqtt.publish("Topic/3",dataBuffer,20,qtAT_LEAST_ONCE);
mqtt.publish(someTopic,(byte *)strBuffer,strlen(strBuffer));
```

You can also create and send a MQTTMessage object directly. This lets you use the Print interface to write complex data to the data buffer:

``` c++
MQTTMessage msg;
msg.topic = "Some/Topic";
msg.qos = qtAT_MOST_ONCE;
msg.print("{ timestamp: ");
msg.print(year);
msg.print("-");
msg.print(month);
msg.print("-");
msg.print(day);
msg.print(" sensorValue: ");
msg.print(someFloat,3,1);
msg.print(" }");
mqtt.publish(msg);
```

To receive a message, override `MQTTClient::receiveMessage()` and provide an event handler. MQTTMessage supports the Printable interface, which makes it easier to print the contents of the data buffer:

``` c++
void MyMQTTClient::receiveMessage(const MQTTMessage &msg) {
  Serial.print(msg.topic);
  Serial.print("=");
  Serial.println(msg);  // Prints the contents of the data buffer
  if (msg.topic.compareTo(String("Topic/One")) == 0) {
    Serial.println("Topic/One Received");
  }
  MQTTClient::receiveMessage(msg);
}
```
