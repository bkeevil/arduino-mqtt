#include "subscriptions.h"

using namespace mqtt;

void MQTTSubscriptionList::clear() {
  MQTTSubscription* ptr = first_;
  while (ptr != nullptr) {
    first_ = ptr->next;
    delete ptr;
    ptr = first_;
  }
}

void MQTTSubscriptionList::import(const MQTTSubscriptionList& subs) noexcept {
  MQTTSubscription* ptr;
  MQTTSubscription* rhs_ptr;
  MQTTSubscription* lhs_ptr;

  rhs_ptr = subs.top_;
  while (rhs_ptr != nullptr) {
    lhs_ptr = find(rhs_ptr->filter);
    if (lhs_ptr == nullptr) {
      lhs_ptr = new MQTTSubscription(*rhs_ptr);
      push(lhs_ptr);
    } else {
      lhs_ptr->filter.setText(rhs_ptr->filter.getText());
      lhs_ptr->handler_ = rhs_ptr->handler_;
      lhs_ptr->qos = rhs_ptr->qos;
      lhs_ptr->sent = rhs_ptr->sent;
    }
    ptr = rhs_ptr;
    rhs_ptr = rhs_ptr->next_;
    delete ptr;
  }
}

MQTTSubscription* MQTTSubscriptionList::find(MQTTFilter& filter) {
  MQTTSubscription* ptr = top_;
  while (ptr != nullptr) {
    if (ptr->filter.equals(filter)) return ptr;
    ptr = ptr->next_;
  }
  return nullptr;
}