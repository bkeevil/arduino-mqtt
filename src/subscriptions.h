#ifndef MQTT_SUBSCRIPTIONS_H
#define MQTT_SUBSCRIPTIONS_H

#include "Arduino.h"
#include "types.h"
#include "tokenizer.h"
#include "message.h"

namespace mqtt {

  class SubscriptionList;

  /** @brief Represents a client subscription */
  class Subscription {
    public:
      Subscription(const String &filter, const QoS qos = QoS::AT_MOST_ONCE, const MessageHandlerFunc f = nullptr) : 
        filter_(filter), qos_(qos), handler_(f) {}
      ~Subscription() { if (next_ != nullptr) { delete next_; next_ = nullptr; }
    protected:
      virtual bool handle(Message& msg) { return handler_(*this,msg); }
    private:
      QoS qos_ {QoS::AT_MOST_ONCE};
      Filter filter_;
      bool sent_ {false};
      MessageHandlerFunc handler_ {nullptr};
      Subscription* next_ {nullptr};
      friend class SubscriptionList;
  };

  /** @brief A linked list of Subscription objects */
  class SubscriptionList {
    public:
      ~SubscriptionList() { clear(); }
      void clear() { if (top_ != nullptr) { delete top_; top_ = nullptr; }}
      void push(Subscription* node) { if (node != nullptr) { node->next_ = top_; top_ = node; } }
      void import(const SubscriptionList& subs);
      Subscription* find(Filter& filter);
    private:
      Subscription* top_ {nullptr};
      friend class Subscription;
  };

};

#endif