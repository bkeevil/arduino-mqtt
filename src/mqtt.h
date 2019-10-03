/** @file       mqtt.h
 *  @brief      An MQTT 3.1.1 client library for the Arduino framework
 *  @author     Bond Keevil
 *  @version    2.0
 *  @date       September 1, 2019
 *  @copyright  GNU General Public License Version 3
 */

// TODO: WillMessage should be converted to an MQTTMessage object

#ifndef MQTT_H
#define MQTT_H

#include "Arduino.h"
#include "Printable.h"
#include "Print.h"

#define DEBUG

#define MQTT_DEFAULT_PING_INTERVAL               20 /**< The number of seconds between pings. Must be less than MQTT_PACKET_TIMEOUT */
#define MQTT_DEFAULT_PING_RETRY_INTERVAL          6 /**< Frequency of pings in seconds after a failed ping response */
#define MQTT_DEFAULT_KEEPALIVE                   30 /**< Number of seconds of inactivity before disconnect */
#define MQTT_MIN_PACKETID                       256 /**< The first 256 packet IDs are reserved for subscribe/unsubscribe packet ids */
#define MQTT_MAX_PACKETID                     65535 /**< The maximum packet ID that can be assigned */
#define MQTT_PACKET_TIMEOUT                       3 /**< Number of seconds before a packet is resent */
#define MQTT_PACKET_RETRIES                       2 /**< Number of retry attempts to send a packet before the connection is considered dead */
#define MQTT_MESSAGE_ALLOC_BLOCK_SIZE             8 /**< When writing a message data buffer, this much memory will be allocated at a time */

/** @cond */

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

/** @endcond */

/** @brief Used to identify the type of a received packet */
enum MQTTPacketType {ptBROKERCONNECT = 0, ptCONNECT = 1, ptCONNACK = 2, ptPUBLISH = 3, ptPUBACK = 4,
  ptPUBREC = 5, ptPUBREL = 6, ptPUBCOMP = 7, ptSUBSCRIBE = 8, ptSUBACK = 9, ptUNSUBSCRIBE = 10,
  ptUNSUBACK = 11, ptPINGREQ = 12, ptPINGRESP = 13, ptDISCONNECT = 14};

/** Quality of Service Levels */
enum qos_t {
  qtAT_MOST_ONCE = 0,           /**< The packet is sent once and may or may not be received by the server */
  qtAT_LEAST_ONCE,              /**< The packet is acknowledge by the server but may be sent by the client more than once */
  qtEXACTLY_ONCE,               /**< Delivery of the packet exactly once is guaranteed using multiple acknowledgements */
  qtMAX_VALUE = qtEXACTLY_ONCE
};

class MQTTClient;  // Forward declaration

/** @class    MQTTMessage mqtt.h
 *  @brief    Represents an MQTT message that is sent or received 
 *  @details  Use the methods of the Print and Printable ancestor classes to access the message 
 *            data buffer. If the size of the data buffer is known, call reserve() to reserve 
 *            a specific memory size. Otherwise memory will be allocated in chunks of 
 *            MQTT_MESSAGE_ALLOC_BLOCK_SIZE bytes. Optionally, call pack() when done to free any 
 *            unused bytes.
 * @remark    The Print interface is used to write directly to the data buffer. 
 */
class MQTTMessage: public Printable, public Print {
  public:
    MQTTMessage() = default;
    MQTTMessage(const String& topic_): topic(topic_) {}
    MQTTMessage(const MQTTMessage& m);

    /** @brief The message topic*/
    String topic;         

    /** @brief The message QoS level*/
    qos_t qos = qtAT_LEAST_ONCE;          
    
    /** @brief Set to true if this message is a duplicate copy of a previous message because it has been resent */
    bool duplicate;
    
    /** @brief For incoming messages, whether it is being sent because it is a retained message. For outgoing messages, tells the server to retain the message. */
    bool retain;

    /** @brief When this class specifies a willMessage or connectMessage, whether the message is enabled **/
    bool enabled = true;

    /** @brief The data buffer */
    byte* data;            

    /** @brief The number of valid bytes in the data buffer. Might by < data_size*/
    size_t data_len;
    
    /** @brief Prints the data buffer to any object descended from the Print class
     *  @param p The object to print to
     *  @return size_t The number of bytes printed */
    size_t printTo(Print& p) const override;

    /** @brief Reads a byte from the data buffer and advance to the next character
     *  @return int The byte read, or -1 if there is no more data */
    int read();
    
    /** @brief Read a byte from the data buffer without advancing to the next character
     *  @return int The byte read, or -1 if there is no more data */    
    int peek() const;
    
    /** @brief Writes a byte to the end of the data buffer
     *  @param c The byte to be written
     *  @return size_t The number of bytes written */
    size_t write(byte b) override;
    
    /** @brief Writes a block of data to the data buffer
     *  @param buffer The data to be written
     *  @param size The size of the data to be written
     *  @return size_t The number of bytes actually written to the buffer */
    size_t write(const byte* buffer, size_t size) override;
    
    /** @brief The number of bytes remaining to be read from the buffer */
    int available() const { return data_len - data_pos; }
    
    /** @brief The number of bytes allocated but not written to */
    int availableForWrite() const /* override */ { return data_size - data_pos; }  
    
    /** @brief Pre-allocate bytes in the data buffer to prevent reallocation and fragmentation
     *  @param size Number of bytes to reserve */
    void reserve(size_t size);
    
    /** @brief Call after writing the data buffer is done to free any extra reserved memory */
    void pack();
    
    /** @brief Set the index from which the next character will be read/written */
    void seek(int pos) { data_pos = pos; }

    /** @brief Returns true if data matches str **/
    bool equals(const char str[]) const;
    /** @brief Returns true if data matches str **/
    bool equals(const String& str) const;
    /** @brief Returns true if data matches str **/
    bool equalsIgnoreCase(const char str[]) const;
    /** @brief Returns true if data matches str **/
    bool equalsIgnoreCase(const String& str) const;
  private: 
    size_t data_size;      /**< The number of bytes allocated in the data buffer */
    size_t data_pos;       /**< Index of the next byte to be written */
};

/** @brief Structure for a linked list of subscriptions */
struct subscription_t {
  char filter[]; 
  qos_t qos_;
  subscription_t* next;
};

/** @struct  queuedMessage_t mqtt.h
 *  @details Structure for message queue linked list 
 */
struct queuedMessage_t {
  /** @brief A unique packetID is assigned when a packet is placed in the queue */
  word packetid;          
  /** @brief Time the packet has been in the queue (in seconds) */
  byte timeout;
  /** @brief Number of times the packet has been retreansmitted */
  byte retries;
  /** @brief The MQTTMessage object that was sent */
  MQTTMessage* message;
  /** @brief Pointer to the next structure in the linked list */
  queuedMessage_t* next;  
};

/** @class   MQTTMessaageQueue mqtt.h
 *  @brief   Base abstract class for managing a linked list of messages
 *  @details Descendant classes that retransmit packets must implement the resend() methods
 */
class MQTTMessageQueue {
  public:
    MQTTMessageQueue(MQTTClient* client) : client(client) {}
    ~MQTTMessageQueue() { clear(); }
    int getCount() const { return count; }
    void clear();
    bool interval();
    void push(queuedMessage_t* qm);
    queuedMessage_t* pop();
  protected:
    MQTTClient* client;
    virtual void resend(queuedMessage_t* qm) = 0;
  private:
    queuedMessage_t* first = NULL;
    queuedMessage_t* last  = NULL;
    int count = 0;
};

/** @class   MQTTPUBLISHQueue mqtt.h
 *  @brief   Message queue for QOS1 & QOS2 Messages that have been sent but have not been acknowledged
 *  @details When a new PUBLISH packet is sent with QOS1 or 2 it is placed in the PUBLISH Queue. 
 *           In the case of a QOS1 message, it is removed from this queue on receipt of a PUBACK message with a matching packetID.
 *           In the case of a QOS2 message, it is moved to the PUBREL queue in response to a PUBREC message. 
 */ 
class MQTTPUBLISHQueue: public MQTTMessageQueue {
  protected:
    /** @brief Resends the PUBLISH message with the duplicate flag set to true */
    virtual void resend(queuedMessage_t* qm);
  public:
    MQTTPUBLISHQueue(MQTTClient* client) : MQTTMessageQueue(client) {}
};

/** @class   MQTTPUBRECQueue mqtt.h
 *  @brief   Message queue for QOS2 messages that have been received but have not been dispatched.
 *  @details When a QOS2 message is received it is stored in the PUBREC queue until a PUBREL message is received,
 *           At which point receiveMessage() is called and the message is removed from the queue.
 *           If no PUBREL message is recieved, the PUBREC packet is retransmitted. 
 */
class MQTTPUBRECQueue: public MQTTMessageQueue {
  protected:
    /** @brief Resends the PUBREC message */
    virtual void resend(queuedMessage_t* qm);
  public:
    MQTTPUBRECQueue(MQTTClient* client) : MQTTMessageQueue(client) {}    
};

/** @class   MQTTPUBRELQueue mqtt.h
 *  @brief   Message queue for QOS2 messages where a PUBREC has been recieved and a PUBREL has been sent
 *  @details The message object is deleted from the queue when a PUBCOMP message with a matching packetID is received
 *  @warning The message attribute is not used and is always NULL in this queue.          
 */
class MQTTPUBRELQueue: public MQTTMessageQueue {
  protected:
    /** @brief Resends the PUBREL message */
    virtual void resend(queuedMessage_t* qm);
  public:
    MQTTPUBRELQueue(MQTTClient* client) : MQTTMessageQueue(client) {}    
};

/** @brief   The abstract base class for the MQTTClient class
 *  @details Provides several protected utility methods for reading/writing data to/from a Stream object as 
 *           per the 3.1.1 protocol specs  */
class MQTTBase {
  public:
    /** @brief The user of the component will supply a reference to an object of the Stream class */
    MQTTBase(Stream& stream): stream(stream) {} 
    protected:
      /** @brief The network stream to read/write from */
    Stream& stream; 

    /** @brief  Reads a word from the stream in big endian order 
     *  @param  A pointer to a variable that will receive the outgoing word
     *  @return True iff two bytes were read from the stream */
    bool readWord(word* value);

    /** @brief  Writes a word to the stream in big endian order 
     *  @param  word  The word to write to the Stream
     *  @return bool  Returns true iff both bytes were successfully written to the stream */
    bool writeWord(const word value);

    /** @brief  Reads the remaining length field of an MQTT packet 
     *  @param  value Receives the remaining length
     *  @return True if successful, false otherwise
     *  @remark See the MQTT 3.1.1 specifications for the format of the remaining length field */
    bool readRemainingLength(long* value);

    /** @brief  Writes the remaining length field of an MQTT packet
     *  @param  value The remaining length to write
     *  @return True if successful, false otherwise 
     *  @remark See the MQTT 3.1.1 specification for the format of the remaining length field */
    bool writeRemainingLength(const long value);

    /** @brief    Writes a UTF8 string to the stream in the format required by the MQTT protocol */
    bool writeStr(const String& str);

    /** @brief    Reads a UTF8 string from the stream in the format required by the MQTT protocol */
    bool readStr(String& str);    
};

/** @brief    The main class for an MQTT client connection
 *  @details  Create an instance of MQTTClient passing a reference to a Stream object as a constructor parameter */ 
class MQTTClient: public MQTTBase {
  public:
    MQTTMessage willMessage;
    MQTTMessage connectMessage;
    MQTTMessage disconnectMessage;

    bool isConnected;
    // Constructor/Destructor
    MQTTClient(Stream& stream): MQTTBase(stream), PUBLISHQueue(this), PUBRECQueue(this), PUBRELQueue(this) {} 
    // Outgoing events - Override in descendant classes
    virtual void connected();
    
    /** @brief Called when the MQTT server terminates the connection
     *  @details Override disconnected() to perform additional actions when the the server terminates the MQTT connection */
    virtual void disconnected();
    
    virtual void initSession() {};
    virtual void subscribed(const word packetID, const byte resultCode) {};
    virtual void unsubscribed(const word packetID) {};
    virtual void receiveMessage(const MQTTMessage& msg) {};
    // Main Interface Methods
    bool connect(const String& clientID, const String& username, const String& password, const bool cleanSession = true, const word keepAlive = MQTT_DEFAULT_KEEPALIVE);
    
    /** @brief Disconnects the MQTT connection */
    void disconnect();
    bool subscribe(const word packetid, const String& filter, const qos_t qos = qtAT_MOST_ONCE);
    bool unsubscribe(const word packetid, const String& filter);

    /** @brief   Publish a message to the server.
     *  @remark  In this version of the function, data is provided as a pointer to a buffer.
     *  @warning The data sent does not include the trailing NULL character */
    bool publish(const String& topic, byte* data = NULL, const size_t data_len = 0, const qos_t qos = qtAT_MOST_ONCE, const bool retain=false);
    
    /** @brief   Publish a message to the server.
     *  @remark  In this version of the function, data is provided as a String object.
     *  @warning If the data sent might include NULL characters, use the alternate version of this function.
     *  @warning The data sent by this function does not include a trailing NULL character */
    bool publish(const String& topic, const String& data, const qos_t qos = qtAT_MOST_ONCE, const bool retain=false);

    /** @brief   Publish an MQTTMessage object to the server.
     *  This method will create a copy of msg and manage its lifecycle. You are free to destroy the original message 
     *  object after calling this function. */
    bool publish(MQTTMessage& msg) { MQTTMessage* m = new MQTTMessage(msg); return sendPUBLISH(m); }

    // Incoming events - Call from your application 
    byte dataAvailable(); /**< Needs to be called whenever there is data available on the connection */
    byte intervalTimer(); /**< Needs to be called once every second */
  private:
    MQTTPUBLISHQueue  PUBLISHQueue;         /**< Outgoing QOS1 or QOS2 Publish Messages that have not been acknowledged */
    MQTTPUBRECQueue   PUBRECQueue;          /**< Incoming QOS2 messages that have not been acknowledged */
    MQTTPUBRELQueue   PUBRELQueue;          /**< Outgoing QOS2 messages that have not been released */
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
    
    /** @brief    Sends an MQTT publish packet. Do not call directly. */  
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
    //  
    friend class MQTTPUBLISHQueue;
    friend class MQTTPUBRECQueue;
    friend class MQTTPUBRELQueue;
};

#endif