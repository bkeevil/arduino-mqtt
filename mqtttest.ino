#include <WiFi.h>
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "mqtt.h"

class MyMQTTClient: public MQTTClient {
  public:
    // Events
    void connected() override;
    void initSession() override;
    void subscribed(word packetID, byte resultCode) override;
    void unsubscribed(word packetID) override;
    void receiveMessage(char *topic, char *data, bool retain, bool duplicate) override; 
};

SSD1306  display(0x3c, 5, 4);
WiFiClient client;
MyMQTTClient mqtt;

const char wifi_ssid[]     = "Prognosti";
const char wifi_password[] = "SunnyEva1255";
const char mqtt_server[]   = "192.168.1.23";
const int  mqtt_port      = 1883;
char* mqtt_clientid  = "ESP32";
char* mqtt_username  = NULL;
char* mqtt_password  = NULL;
 
void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setContrast(255);
  WiFi.onEvent(WiFiEvent);
    
  connectToWifi();
}

void loop() {
  byte errorCode;
  
  if (client.available() > 1) {
    Serial.print(client.available()); Serial.println(" bytes available");
    errorCode = mqtt.dataAvailable();
    if (errorCode != MQTT_ERROR_NONE) {
      Serial.print("Error code "); Serial.println(errorCode);
    }
  } else {
    delay(100);
  }
}

void connectToMQTT() {
  display.drawString(0,40,"Establishing MQTT Session");
  mqtt.stream = &client;
  display.display();
  mqtt.connect(mqtt_clientid,mqtt_username,mqtt_password,true);
}

void connectToWifi() {
  display.drawString(0,0,"Connecting to WiFi...");
  display.display();  
  WiFi.begin(wifi_ssid,wifi_password);
}

void connectToServer() {
  Serial.println("Establishing TCP Connection");
  display.drawString(0,20,"Establishing TCP Connection");
  display.display();
  if (!client.connect(mqtt_server, mqtt_port)) {
    Serial.println("TCP Connection Failed");
    display.drawString(0,30,"TCP Connection Failed");
    display.display();
    //reboot
    return;
  } else {
    display.drawString(0,30,"TCP Connected");    
    connectToMQTT();
  }
}

void doWifiConnected() {
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  display.drawString(0,10,"WiFi Connected");
  connectToServer();
}

void doWifiDisconnected() {
  Serial.println("WiFi lost connection");
  mqtt.disconnected();
  //reboot  
}

void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);
    
    // https://github.com/espressif/esp-idf/blob/master/components/esp32/include/esp_event.h
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        doWifiConnected();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        doWifiDisconnected();
        break;
    }
}

void MyMQTTClient::initSession() {
  Serial.println("Initializing Subscriptions");
  mqtt.subscribe(1,"System/Time/Minute");  
  MQTTClient::initSession();
}

void MyMQTTClient::connected() {
  Serial.println("Connected to MQTT");
  display.drawString(0,50,"Connected to MQTT");
  display.display();
  MQTTClient::connected();
}

void MyMQTTClient::subscribed(word packetID, byte resultCode) {
  Serial.print("Subscribed "); Serial.print(packetID); Serial.print(" "); Serial.println(resultCode);
  MQTTClient::subscribed(packetID,resultCode);
}

void MyMQTTClient::unsubscribed(word packetID) {
  Serial.print("Unsubscribed "); Serial.println(packetID);
  MQTTClient::unsubscribed(packetID);
}

void MyMQTTClient::receiveMessage(char *topic, char *data, bool retain, bool duplicate) {
  Serial.print("recieveMessage topic="); Serial.print(topic); Serial.print(" data="); Serial.println(data);
  MQTTClient::receiveMessage(topic,data,retain,duplicate);
}
    
