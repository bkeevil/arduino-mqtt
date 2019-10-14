#include "subscriptions.h"

using namespace mqtt;

void SubscriptionList::import(const SubscriptionList& subs) noexcept {
  Subscription* ptr;
  Subscription* rhs_ptr;
  Subscription* lhs_ptr;

  rhs_ptr = subs.top;
  while (rhs_ptr != nullptr) {
    lhs_ptr = find(rhs_ptr->filter);
    if (lhs_ptr == nullptr) {
      lhs_ptr = new Subscription(*rhs_ptr);
      push(lhs_ptr);
    } else {
      lhs_ptr->filter.setText(rhs_ptr->filter.getText());
      lhs_ptr->handler = rhs_ptr->handler;
      lhs_ptr->qos = rhs_ptr->qos;
      lhs_ptr->sent = rhs_ptr->sent;
    }
    ptr = rhs_ptr;
    rhs_ptr = rhs_ptr->next;
    delete ptr;
  }
}

Subscription* SubscriptionList::find(Filter& filter) {
  Subscription* ptr = top;
  while (ptr != nullptr) {
    if (ptr->filter.equals(filter)) return ptr;
  ptr = ptr->next;
  }
  return nullptr;
}
