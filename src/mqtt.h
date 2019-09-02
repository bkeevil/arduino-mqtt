#ifndef MQTT_H
#define MQTT_H

#include "Arduino.h"
#include "Printable.h"
#include "Print.h"

#define MQTT_DEFAULT_PING_INTERVAL               30 // Number of seconds between pings
#define MQTT_DEFAULT_PING_RETRY_INTERVAL          6 // Frequency of pings in seconds after a failed ping response.
#define MQTT_DEFAULT_KEEPALIVE                   60 // Number of seconds of inactivity before disconnect
//#define MQTT_MAX_TOPIC_LEN                       64 // Bytes
//#define MQTT_MAX_DATA_LEN                        64 // Bytes
//#define MQTT_PACKET_QUEUE_SIZE                    8
#define MQTT_MIN_PACKETID                       256 // The first 256 packet IDs are reserved for subscribe/unsubscribe packet ids
#define MQTT_MAX_PACKETID                     65535
#define MQTT_PACKET_TIMEOUT                       3 // Number of seconds before a packet is resent
#define MQTT_PACKET_RETRIES                       2 // Number of retry attempts to send a packet before the connection is considered dead

#define ptBROKERCONNECT                           0
#define ptCONNECT                                 1
#define ptCONNACK                                 2
#define ptPUBLISH                                 3
#define ptPUBACK                                  4
#define ptPUBREC                                  5
#define ptPUBREL                                  6
#define ptPUBCOMP                                 7
#define ptSUBSCRIBE                               8
#define ptSUBACK                                  9
#define ptUNSUBSCRIBE                            10
#define ptUNSUBACK                               11
#define ptPINGREQ                                12
#define ptPINGRESP                               13
#define ptDISCONNECT                             14

#define MQTT_CONNACK_SUCCESS                      0
#define MQTT_CONNACK_UNACCEPTABLE_PROTOCOL        1
#define MQTT_CONNACK_CLIENTID_REJECTED            2
#define MQTT_CONNACK_SERVER_UNAVAILABLE           3
#define MQTT_CONNACK_BAD_USERNAME_PASSWORD        4
#define MQTT_CONNACK_NOT_AUTHORIZED               5

#define MQTT_ERROR_NONE                           0

#define MQTT_ERROR_ALREADY_CONNECTED            101
#define MQTT_ERROR_NOT_CONNECTED                102
#define MQTT_ERROR_INSUFFICIENT_DATA            103
#define MQTT_ERROR_REMAINING_LENGTH_ENCODING    104
#define MQTT_ERROR_INVALID_PACKET_FLAGS         105
#define MQTT_ERROR_PACKET_INVALID               106
#define MQTT_ERROR_PAYLOAD_INVALID              107
#define MQTT_ERROR_VARHEADER_INVALID            108
#define MQTT_ERROR_UNACCEPTABLE_PROTOCOL        109
#define MQTT_ERROR_CLIENTID_REJECTED            110
#define MQTT_ERROR_SERVER_UNAVAILABLE           111
#define MQTT_ERROR_BAD_USERNAME_PASSWORD        112
#define MQTT_ERROR_NOT_AUTHORIZED               113
#define MQTT_ERROR_NO_CLIENTID                  114
#define MQTT_ERROR_WILLMESSAGE_INVALID          115
#define MQTT_ERROR_NO_PING_RESPONSE             116
#define MQTT_ERROR_UNHANDLED_PACKETTYPE         117
#define MQTT_ERROR_NO_SUBSCRIPTION_LIST         118
#define MQTT_ERROR_INVALID_SUBSCRIPTION_ENTRIES 119
#define MQTT_ERROR_INVALID_RETURN_CODES         120
#define MQTT_ERROR_CONNECT_TIMEOUT              121
#define MQTT_ERROR_NOT_IMPLEMENTED              122
#define MQTT_ERROR_PACKET_QUEUE_FULL            123
#define MQTT_ERROR_PACKETID_NOT_FOUND           124
#define MQTT_ERROR_SEND_PUBCOMP_FAILED          125
#define MQTT_ERROR_SEND_PUBREL_FAILED           126
#define MQTT_ERROR_PACKET_QUEUE_TIMEOUT         127

#define MQTT_ERROR_UNKNOWN                      255

enum qos_t {
  qtAT_MOST_ONCE = 0,
  qtAT_LEAST_ONCE,
  qtEXACTLY_ONCE,
  qtMAX_VALUE = qtEXACTLY_ONCE
};

class MQTTMessage: Printable, Print {
  private: 
    size_t data_size;
    size_t data_pos;
  public:
    String topic;
    qos_t qos;
    bool retain;
    uint8_t data[];
    size_t data_len;
    // 
    MQTTMessage(String topic, qos_t qos = qtAT_LEAST_ONCE, bool retain = false, uint8_t data[] = NULL, uint8_t data_len = 0) : topic(topic),qos(qos),retain(retain),data(data),data_len(data_len),data_size(data_len),data_pos(data_len);
    virtual size_t printTo(Print& p);
    virtual int read();
    virtual int peek();
    virtual size_t write(uint8_t b);
    virtual size_t write(const uint8_t *buffer, size_t size);
    virtual int available() { return data_len - data_pos; }
    virtual int availableForWrite() { return data_size - data_pos; } 
    void reserve(size_t size);
    void pack();
   
}

struct queuedMessage_t {
  word packetid;
  byte timeout;
  byte retries;
  bool duplicate;
  mqttMessage_t *message;
};

struct willMessage_t {
  String topic;
  String data;
  bool enabled;
  bool retain;
  byte qos;
};

typedef willMessage_t connectMessage_t;

struct packetMessage_t {
  word packetid;
  byte timeout;
  byte retries;
};

class MQTT
class MQTTClient {
  private:
    publishMessage_t outgoingPUBLISHQueue[MQTT_PACKET_QUEUE_SIZE];
    publishMessage_t incomingPUBLISHQueue[MQTT_PACKET_QUEUE_SIZE];
    packetMessage_t  PUBRELQueue[MQTT_PACKET_QUEUE_SIZE];
    byte incomingPUBLISHQueueCount;
    byte outgoingPUBLISHQueueCount;
    byte PUBRELQueueCount;
    word nextPacketID = MQTT_MIN_PACKETID;
    int  pingIntervalRemaining;
    byte pingCount;
    //
    bool readByte(byte* b);
    bool writeByte(const byte b);
    bool readWord(word *value);
    bool writeWord(const word value);
    bool readRemainingLength(long *value);
    bool writeRemainingLength(const long value);
    bool readData(char *data, const word len);
    bool writeData(char *data, const word len);
    bool writeData(String data);
    bool writeStr(char* str);
    bool writeStr(String str);
    bool readStr(char* str, const word len);
    //
    void reset();
    byte pingInterval();
    bool queueInterval();
    bool addToOutgoingQueue(word packetid, byte qos, bool retain, bool duplicate, char* topic, char* data);
    bool addToOutgoingQueue(word packetid, byte qos, bool retain, bool duplicate, String topic, String data);
    bool addToIncomingQueue(word packetid, byte qos, bool retain, bool duplicate, char* topic, char* data);
    bool addToPUBRELQueue(word packetid);
    void deleteFromOutgoingQueue(byte i);
    void deleteFromIncomingQueue(byte i);
    void deleteFromPUBRELQueue(byte i);
    //
    byte recvCONNACK();
    byte recvPINGRESP();
    byte recvSUBACK(long remainingLength);
    byte recvUNSUBACK();
    byte recvPUBLISH(byte flags, long remainingLength);
    byte recvPUBACK();
    byte recvPUBREC();
    byte recvPUBREL();
    byte recvPUBCOMP();
    //
    bool sendPINGREQ();
    bool sendPUBACK(word packetid);
    bool sendPUBREL(word packetid);
    bool sendPUBREC(word packetid);
    bool sendPUBCOMP(word packetid);
  public:
    Stream* stream;
    willMessage_t willMessage;
    connectMessage_t connectMessage;
    bool isConnected;
    // Events
    virtual void connected() {};
    virtual void initSession() {};
    virtual void subscribed(word packetID, byte resultCode) {};
    virtual void unsubscribed(word packetID) {};
    virtual void receiveMessage(String topic, String data, bool retain, bool duplicate) {};
    // Methods
    bool connect(String clientID, String username, String password, bool cleanSession = false, word keepAlive = MQTT_DEFAULT_KEEPALIVE);
    bool disconnect();
    void disconnected();
    bool subscribe(word packetid, char *filter, qos_t qos = qtAT_MOST_ONCE);
    bool subscribe(word packetid, String filter, qos_t qos = qtAT_MOST_ONCE);
    bool unsubscribe(word packetid, char *filter);
    bool publish(char *topic, char *data, qos_t qos = qtAT_MOST_ONCE, bool retain=false, bool duplicate=false);
    bool publish(char *topic, uint8_t *data, uint8_t data_len, qos_t qos = qtAT_MOST_ONCE, bool retain=false, bool duplicate = false); 
    bool publish(String topic, String data, qos_t qos = qtAT_MOST_ONCE, bool retain=false, bool duplicate=false);
    
    byte dataAvailable(); // Needs to be called whenever there is data available
    byte intervalTimer(); // Needs to be called by program every second
};

#endif
