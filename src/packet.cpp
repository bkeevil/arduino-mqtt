#include "packet.h"

using namespace std;
using namespace mqtt;

/* MQTTPacket */

/** @brief Clears and destroys the stack */
void Pending::clear() {
  Packet* ptr = first;
  Packet* node;

  while (ptr != nullptr) {
    node = ptr;
    ptr = ptr->next;
    delete node;  
  }

  top = nullptr;
}

/** @brief Adds node to the top of the stack */
void Pending::push(Packet* node) {
  if (node != nullptr) {
    if (top == nullptr) {
      top = node;
    } else {
      node->next_ = top;
      top = node;
    }
  }
}

/** @brief Removes and returns the first item from the stack */
Packet* Pending::pop() {
  if (top == nullptr) {
    return nullptr;
  } else {
    Packet* node = top;
    top = top->next;
    return node;
  }
}

/** @brief Decrements the timeout value retransmits packets as necessary */
void Pending::interval() {
  Packet* prev = nullptr;
  Packet* ptr = top;

  while (ptr != nullptr) {
    if (--ptr->interval == 0) {
      if (++ptr->retrycount >= Packet::maxretries) {
        if (prev == nullptr) {
          top = ptr->next;
        } else {
          prev->next = ptr->next;
        }
        delete ptr;
        Serial.println("Packet deleted: Too many packet resends");
      } else {
        ptr->interval = Packet::timeout;
        ptr->send(true);
        Serial.println("Packet timeout: Resending packet");
      }
    }
    prev = ptr;
    ptr = ptr->next;
  }
}

/** @brief Returns a count of the number of items in the stack */
const int Pending::count() const {
  int count = 0;
  Packet* ptr = top;
  while (ptr != nullptr) {
    ++count;
    ptr = ptr->next;
  }
  return count;
}
