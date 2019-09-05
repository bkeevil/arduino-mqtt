/** @file       mqtt.h
 *  @brief      An MQTT 3.1.1 client library for the Arduino framework
 *  @author     Bond Keevil
 *  @version    2.0
 *  @date       September 1, 2019
 *  @copyright  GNU General Public License Version 3
 */

#ifndef MQTT_H
#define MQTT_H

#include "Arduino.h"
#include "Printable.h"
#include "Print.h"

#define MQTT_DEFAULT_PING_INTERVAL               30 /**< Number of seconds between pings */
#define MQTT_DEFAULT_PING_RETRY_INTERVAL          6 /**< Frequency of pings in seconds after a failed ping response */
#define MQTT_DEFAULT_KEEPALIVE                   60 /**< Number of seconds of inactivity before disconnect */
#define MQTT_MIN_PACKETID                       256 /**< The first 256 packet IDs are reserved for subscribe/unsubscribe packet ids */
#define MQTT_MAX_PACKETID                     65535
#define MQTT_PACKET_TIMEOUT                       3 /**< Number of seconds before a packet is resent */
#define MQTT_PACKET_RETRIES                       2 /**< Number of retry attempts to send a packet before the connection is considered dead */
#define MQTT_MESSAGE_ALLOC_BLOCK_SIZE             8 /**< When writing a message data buffer, this much memory will be allocated at a time */

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

/** Quality of Service Levels */
enum qos_t {
  qtAT_MOST_ONCE = 0,           /**< The packet is sent once and may or may not be received by the server */
  qtAT_LEAST_ONCE,              /**< The packet is acknowledge by the server but may be sent by the client more than once */
  qtEXACTLY_ONCE,               /**< Delivery of the packet exactly once is guaranteed using multiple acknowledgements */
  qtMAX_VALUE = qtEXACTLY_ONCE
};

class MQTTClient;

/** @class    MQTTMessage mqtt.h
 *  @brief    Represents an MQTT message that is sent or received 
 *  @details  Use the methods of the Print and Printable ancestor classes to access the message 
 *            data buffer. If the size of the data buffer is known, call reserve() to reserve 
 *            a specific memory size. Otherwise memory will be allocated in chunks of 
 *            MQTT_MESSAGE_ALLOC_BLOCK_SIZE bytes. Optionally, call pack() when done to free any 
 *            unused bytes.
 */
class MQTTMessage: Printable, Print {
  private: 
    size_t data_size;      /**< The number of bytes allocated in the data buffer */
    size_t data_pos;       /**< Index of the next byte to be written */
  public:
    String topic;          /**< The topic of the message */
    qos_t qos;             /**< Message quality of service level */
    bool duplicate;        /**< Set to true if this message is a duplicate copy of a previous message because it has been resent */
    bool retain;           /**< For incoming messages, whether it is being sent because it is a retained 
                                message. For outgoing messages, tells the server to retain the message. */
    byte* data;            /**< The data buffer */
    size_t data_len;       /**< The number of valid bytes in the data buffer. Might by < data_size */
    /** @brief  Initialize the Message data. The only required parameter is a topic String */
    MQTTMessage(): qos(qtAT_LEAST_ONCE),retain(false),data(NULL),data_len(0),data_size(0),data_pos(0) {}
    MQTTMessage(const String& topic, const qos_t qos = qtAT_LEAST_ONCE, const bool retain = false, byte* data = NULL, const size_t data_len = 0) : 
    topic(topic),qos(qos),retain(retain),data(data),data_len(data_len),data_size(data_len),data_pos(data_len) {}
    size_t printTo(Print& p) const;  /**< See the Prinatable class in the Arduino documentation */
    int read();                /**< See the Stream class in the Arduino documentation */
    int peek();                /**< See the Stream class in the Arduino documentation */ 
    size_t write(byte b);   /**< Writes a single byte to the end of the data buffer. See the Print class in the Arduino documnetation */
    size_t write(const byte* buffer, size_t size);         /**< Writes size bytes from buffer to the end of the data buffer. See the Print class in the Arduino documentation */
    int available() { return data_len - data_pos; }           /**< The number of bytes remaining to be read from the buffer */
    int availableForWrite() { return data_size - data_pos; }  /**< The number of bytes allocated but not written to */
    void reserve(size_t size);          /**< Reserve size bytes of RAM for the data buffer (Optional) */
    void pack();                        /**< Free any unused RAM */
    void seek(int pos) { data_pos = pos; } /** Set the index from which the next character will be read/written */
};

struct queuedMessage_t {
  word packetid;
  byte timeout;
  byte retries;
  MQTTMessage* message;
  queuedMessage_t* next;
};

/** @brief   Base class for an MQTT message class.
 *  @details Descendant classes that retransmit packets must implement the resend() methods
 */
class MQTTMessageQueue {
  private:
    queuedMessage_t* first = NULL;
    queuedMessage_t* last  = NULL;
    int count = 0;
  protected:
    MQTTClient* client;
    virtual void resend(queuedMessage_t* qm) = 0;
  public:
    MQTTMessageQueue(MQTTClient* client) : client(client) {}
    ~MQTTMessageQueue() { clear(); }
    int getCount() { return count; }
    void clear();
    bool interval();
    void push(queuedMessage_t* qm);
    queuedMessage_t* pop();
};

class MQTTPUBLISHQueue: public MQTTMessageQueue {
  protected:
    virtual void resend(queuedMessage_t* qm);
  public:
    MQTTPUBLISHQueue(MQTTClient* client) : MQTTMessageQueue(client) {}
};

class MQTTPUBRECQueue: public MQTTMessageQueue {
  protected:
    virtual void resend(queuedMessage_t* qm);
  public:
    MQTTPUBRECQueue(MQTTClient* client) : MQTTMessageQueue(client) {}    
};

class MQTTPUBRELQueue: public MQTTMessageQueue {
  protected:
    virtual void resend(queuedMessage_t* qm);
  public:
    MQTTPUBRELQueue(MQTTClient* client) : MQTTMessageQueue(client) {}    
};

struct willMessage_t {
  String topic;
  byte* data;
  size_t data_len;
  bool enabled;
  bool retain;
  byte qos;
};

typedef willMessage_t connectMessage_t;

class MQTTBase {
  protected:
    Stream* stream;
    bool readWord(word* value);
    bool writeWord(const word value);
    bool readRemainingLength(long* value);
    bool writeRemainingLength(const long value);
    bool writeStr(const String& str);
    bool readStr(String& str);
  public:
    MQTTBase(Stream* stream): stream(stream) {}
};

class MQTTClient: public MQTTBase {
  private:
    MQTTPUBLISHQueue* PUBLISHQueue;         /**< Outgoing QOS1 or QOS2 Publish Messages that have not been acknowledged */
    MQTTPUBRECQueue*  PUBRECQueue;          /**< Incoming QOS2 messages that have not been acknowledged */
    MQTTPUBRELQueue*  PUBRELQueue;          /**< Outgoing QOS2 messages that have not been released */
    word nextPacketID = MQTT_MIN_PACKETID;  /**< Packet IDs 0..255 are used for subscriptions */
    int  pingIntervalRemaining;
    byte pingCount;
    //
    void reset();
    byte pingInterval();
    bool queueInterval();
    //
    byte recvCONNACK();
    byte recvSUBACK(const long remainingLength);
    byte recvUNSUBACK();
    byte recvPUBLISH(const byte flags, const long remainingLength);
    byte recvPUBACK();
    byte recvPUBREC();
    byte recvPUBREL();
    byte recvPUBCOMP();
    //
    bool sendPINGREQ();
    bool sendPUBLISH(MQTTMessage* msg);
    bool sendPUBACK(const word packetid);
    bool sendPUBREL(const word packetid);
    bool sendPUBREC(const word packetid);
    bool sendPUBCOMP(const word packetid);
    friend class MQTTPUBLISHQueue;
    friend class MQTTPUBRECQueue;
    friend class MQTTPUBRELQueue;
  public:
    willMessage_t willMessage;
    connectMessage_t connectMessage;
    bool isConnected;
    // Constructor/Destructor
    MQTTClient(Stream* stream);
    ~MQTTClient();
    // Outgoing events - Override in descendant classes
    virtual void connected() {};
    virtual void disconnected();
    virtual void initSession() {};
    virtual void subscribed(const word packetID, const byte resultCode) {};
    virtual void unsubscribed(const word packetID) {};
    virtual void receiveMessage(const String& topic, const byte* data, const size_t data_len, const bool retain, const bool duplicate) {};
    // Main Interface Methods
    bool connect(const String& clientID, const String& username, const String& password, const bool cleanSession = false, const word keepAlive = MQTT_DEFAULT_KEEPALIVE);
    void disconnect();
    bool subscribe(const word packetid, const String& filter, const qos_t qos = qtAT_MOST_ONCE);
    bool unsubscribe(const word packetid, const String& filter);
    bool publish(const String& topic, byte* data = NULL, const size_t data_len = 0, const qos_t qos = qtAT_MOST_ONCE, const bool retain=false);
    bool publish(const String& topic, const String& data, const qos_t qos = qtAT_MOST_ONCE, const bool retain=false);
    // Incoming events - Call from your application 
    byte dataAvailable(); /**< Needs to be called whenever there is data available on the connection */
    byte intervalTimer(); /**< Needs to be called once every second */
};

#endif