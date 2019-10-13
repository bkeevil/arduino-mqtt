#include "Arduino.h"
#include "Printable.h"
#include "Print.h"
#include "network.h"

namespace mqtt {

  /** Used to identify the type of a received packet */
  enum PacketType {BROKERCONNECT = 0, CONNECT = 1, CONNACK = 2, PUBLISH = 3, PUBACK = 4,
    PUBREC = 5, PUBREL = 6, PUBCOMP = 7, SUBSCRIBE = 8, SUBACK = 9, UNSUBSCRIBE = 10,
    UNSUBACK = 11, PINGREQ = 12, PINGRESP = 13, DISCONNECT = 14};

  class Packet {
    public:
      Packet(PacketType packettype) : packettype_(packettype), packetid_(++nextid) {}
      const word packetid() const { return packetid_; }
      const PacketType packettype() const { return packettype_; }
    protected:
      virtual void send(Network& network) = 0;
    private:
      static const int timeout {3};
      static const byte maxretries {2};
      static const word nextid {0};
      byte retrycount;
      word interval;
      PacketType packettype_;
      word packetid_;
      Packet* next; 
      friend class Pending;
  };

  /** @brief A stack of PUBLISH, PUBREL, PUBCOMP, SUBSCRIBE and UNSUBSCRIBE packets that are waiting for acknowledgement */
  class Pending {
    public:
      ~Pending() { clear(); }
      void clear();
      void push(Packet* node);
      Packet* pop();
      void interval();
      const int count() const;
    private:
      Packet* top;  
  };

}
