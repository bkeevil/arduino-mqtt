#ifndef MQTT_CONNECTION_H
#define MQTT_CONNECTION_H

#include "types.h"
#include "network.h"
#include "packet.h"
#include "tokenizer.h"
#include "message.h"
#include "subscriptions.h"

namespace mqtt {

  enum ConnectionState {DISCONNECTED=0,CONNECTING,CONNECTED};

  enum ConnackResult {SUCCESS=0,UNACCEPTABLE_PROTOCOL,CLIENTID_REJECTED,SERVER_UNAVAILABLE,BAD_USERNAME_PASSWORD,NOT_AUTHORIZED};

  /** TODO: Will be used to register event handlers */
  enum Event {CONNECTED = 0, INIT_SESSION, DISCONNECTED, MAX};

namespace mqtt {

  /** @brief    The main class for an MQTT client connection
   *  @details  Create an instance of Client passing a reference to a Stream object as a constructor parameter */ 
  class Connection : public Network {
    public:
      using EventHandlerFunc = void (*)();

      Connection(Client& client);

      bool connect(const char* clientid, const char* username, const char* password);
      bool reconnect() { connect(clientid_,username_,password_); }
      void disconnect();
      
      void registerEvent(Event ev, EventHandlerFunc fn) { if (ev < Event::MAX) events_[ev] = fn; }      

      void poll(); /**< Call poll() regularly from your application loop */

      SystemMessage willMessage;
      SystemMessage connectMessage;
      SystemMessage disconnectMessage;
      unsigned long int connectTimeout {3000};
      unsigned long int reconnectTimeout {10000}; 
      bool autoReconnect   {true};  /**< If true, automatically attempt to reconnect a lost connection */
      bool cleanSession    {true};  /**< True if the client is requesting a clean session */
      byte pingInterval      {20};  /**< The number of seconds between pings. Must be less than packetTimeout */
      byte pingRetryInterval  {6};  /**< Frequency of pings in seconds after a failed ping response */
      word keepalive         {30};  /**< Number of seconds of inactivity before disconnect */
      ConnectionState state {ConnectionState::DISCONNECTED};
    protected:
      //void reset();
      virtual byte handlePacket(PacketType packetType, byte flags, long remainingLength);
      // Outgoing events - Override in descendant classes
      virtual void initSession()  { if (events_[Event::INIT_SESSION] != nullptr) events_[INIT_SESSION](); }
      virtual void connected()    { state = ConnectionState::CONNECTED; if (events_[Event::CONNECTED] != nullptr) events_[CONNECTED](); }
      virtual void disconnected();
      // Packet queue
      Pending pending;
      word nextPacketID;
    private:
      byte pingInterval();
      byte dataAvailable();
      byte recvCONNACK();
      byte recvSUBACK(const long remainingLength);
      byte recvUNSUBACK();
      byte recvPUBLISH(const byte flags, const long remainingLength);
      byte recvPUBACK();
      byte recvPUBREC();
      byte recvPUBREL();
      byte recvPUBCOMP();
      bool sendSUBSCRIBE(const SubscriptionList& subscriptions);
      bool sendPUBLISH();
      bool sendPUBACK(const word packetid);
      bool sendPUBREL(const word packetid);
      bool sendPUBREC(const word packetid);
      bool sendPUBCOMP(const word packetid);      
      bool sendPINGREQ();
      EventHandlerFunc events_[Event::MAX];
      unsigned long int lastMillis;
      byte pingIntervalRemaining;
      byte pingCount;
      const char* clientid_ {nullptr};
      const char* username_ {nullptr};
      const char* password_ {nullptr};
  };

}; // Namespace mqtt

#endif