#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

namespace mqtt {

  enum ConnackResult {SUCCESS=0,UNACCEPTABLE_PROTOCOL,CLIENTID_REJECTED,SERVER_UNAVAILABLE,BAD_USERNAME_PASSWORD,NOT_AUTHORIZED};

  enum ErrorCode {NONE=0,ALREADY_CONNECTED=101,NOT_CONNECTED,INSUFFICIENT_DATA,REMAINING_LENGTH_ENCODING,INVALID_PACKET_FLAGS,
                  PACKET_INVALID,PAYLOAD_INVALID,VARHEADER_INVALID,UNACCEPTABLE_PROTOCOL,CLIENTID_REJECTED,SERVER_UNAVAILABLE,
                  BAD_USERNAME_PASSWORD,NOT_AUTHORIZED,NO_CLIENTID,WILLMESSAGE_INVALID,NO_PING_RESPONSE,UNHANDLED_PACKETTYPE,
                  NO_SUBSCRIPTION_LIST,INVALID_SUBSCRIPTION_ENTRIES,INVALID_RETURN_CODES,CONNECT_TIMEOUT,NOT_IMPLEMENTED,
                  PACKET_QUEUE_FULL,PACKETID_NOT_FOUND,SEND_PUBCOMP_FAILED,SEND_PUBREL_FAILED,PACKET_QUEUE_TIMEOUT,UNKNOWN=255};

  /** TODO: Will be used to register event handlers */
  enum Event {CONNECTED = 0, INIT_SESSION, DISCONNECTED};

  /** Quality of Service Levels */
  enum QoS {
    AT_MOST_ONCE = 0, /**< The packet is sent once and may or may not be received by the server */
    AT_LEAST_ONCE,    /**< The packet is acknowledge by the server but may be sent by the client more than once */
    EXACTLY_ONCE,     /**< Delivery of the packet exactly once is guaranteed using multiple acknowledgements */
    MAX_VALUE         
  };

namespace mqtt {

  struct Configuration {
    bool cleanSession   {true};  /**< True if the client is requesting a clean session */
    int pingInterval      {20};  /**< The number of seconds between pings. Must be less than packetTimeout */
    int pingRetryInterval  {6};  /**< Frequency of pings in seconds after a failed ping response */
    int keepalive         {30};  /**< Number of seconds of inactivity before disconnect */
  } defaultConfiguration;


  // Forward Declarations
  class Topic;
  class Filter;
  class Subscription;
  class Message;
  class Client;

  using EventHandlerFunc = void (*)();
  using MessageHandlerFunc = bool (*)(const Subscription& sub, const Message& msg);
  using DefaultMessageHandlerFunc = void (*)(const Message& msg);



  /** @brief    The main class for an MQTT client connection
   *  @details  Create an instance of Client passing a reference to a Stream object as a constructor parameter */ 
  class Client: public Network {
    public:
      Message willMessage;
      Message connectMessage;
      Message disconnectMessage;
      Configuration configuration;

      bool isConnected;
      // Constructor/Destructor
      Client(Stream& stream): Base(stream) {} 
      
      // Outgoing events - Override in descendant classes
      virtual void connected();
      
      /** @brief Called when the MQTT server terminates the connection
       *  @details Override disconnected() to perform additional actions when the the server terminates the MQTT connection */
      virtual void disconnected();
      
      virtual void initSession() {};
      virtual void subscribed(const word packetID, const byte resultCode) {};
      virtual void unsubscribed(const word packetID) {};
      virtual void receiveMessage(const Message& msg) { defaultHandler_(msg); }
      // Main Interface Methods
      bool connect(const String& clientID, const String& username, const String& password, const bool cleanSession = true, const word keepAlive = configuration.keepalive);
      bool connect(const char* clientID, const char* username, const char* password, const bool cleanSession = true, const word keepAlive = configuration.keepAlive);    
      /** @brief Disconnects the MQTT connection */
      void disconnect();
      
      void registerDefaultMessageHandler(DefaultMessageHandlerFunc handler) { defaultHandler_ = handler; }
      bool subscribe(SubscriptionList& subscriptions);
      bool subscribe(const String& filter, const QoS qos = AT_MOST_ONCE, const MessageHandlerFunc handler = nullptr); 
      bool subscribe(const char *filter, const QoS qos = AT_MOST_ONCE, const MessageHandlerFunc handler = nullptr);
      bool unsubscribe(const String& filter);

      // Incoming events - Call from your application 
      byte dataAvailable(); /**< Needs to be called whenever there is data available on the connection */
      byte intervalTimer(); /**< Needs to be called once every second */
    private:
      Pending pending;
      word nextPacketID {0};  
      int  pingIntervalRemaining;
      byte pingCount;
      DefaultMessageHandlerFunc defaultHandler_ {nullptr};
      SubscriptionList subscriptions_;
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
      bool sendPUBLISH();
      bool sendPUBACK(const word packetid);
      bool sendPUBREL(const word packetid);
      bool sendPUBREC(const word packetid);
      bool sendPUBCOMP(const word packetid);

      //
      bool sendPINGREQ();

      bool sendSUBSCRIBE(const SubscriptionList& subscriptions);

      //  
      friend class Packet;
  };

} // Namespace mqtt

#endif