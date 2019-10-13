#ifndef MQTT_SUBSCRIPTIONS_H
#define MQTT_SUBSCRIPTIONS_H

#include "Arduino.h"
#include "tokenizer.h"
#include "message.h"

namespace mqtt {

  /** @brief Represents a client susbscriptions */
  class Subscription {
    public:
      Subscription(const String &filter) : filter(filter), qos(QoS::AT_MOST_ONCE), handler(nullptr), sent(false) {}
      Subscription(const String &filter, const QoS qos, const MessageHandlerFunc f) : filter(filter), qos(qos), handler(f), sent(false) {}
      Subscription(const Subscription& rhs) : qos(rhs.qos), filter(rhs.filter), next(nullptr), sent(rhs.sent), handler(rhs.handler) {}
      QoS qos;
      Filter filter;
      bool sent;
    protected:
      virtual bool handle(Message& msg) { return handler(*this,msg); }
    private:
      MessageHandlerFunc handler = nullptr;
      Subscription* next;
      friend class SubscriptionList;
  };

  /** @brief A linked list of Subscription objects */
  class SubscriptionList {
    public:
      ~SubscriptionList() { clear(); }
      void clear();
      void push(Subscription* node) { if (node != nullptr) { node->next = top; top = node; } }
      void import(const SubscriptionList& subs);
      Subscription* find(Filter& filter);
    private:
      Subscription* top;
  };

};

#endif