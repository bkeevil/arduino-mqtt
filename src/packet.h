#include "Arduino.h"
#include "Printable.h"
#include "Print.h"
#include "network.h"

namespace mqtt {

  class QueuedMessage {
    public:
      QueuedMessage(PacketType packettype) : packettype_(packettype), packetid_(++nextid_) {}
      const word packetid() const { return packetid_; }
      const PacketType packettype() const { return packettype_; }
    protected:
      virtual void send(Network& network) = 0;
    private:
      static const int timeout {3};
      static const byte maxretries {2};
      static word nextid_;
      byte retrycount_;
      word interval_;
      PacketType packettype_;
      word packetid_;
      Packet* next_; 
      friend class PacketList;
  };

  /** @brief A stack of PUBLISH, PUBREL, PUBCOMP, SUBSCRIBE and UNSUBSCRIBE packets that are waiting for acknowledgement */
  class MessageQueue {
    public:
      ~PacketList() { clear(); }
      void clear();
      void push(Packet* node);
      Packet* pop();
      void interval();
      const int count() const;
    private:
      Packet* top_;  
  };

}
