#include "packet.h"

/** @brief Clears the list of pending Packets */
void PacketList::clear() {
  Packet* ptr = first_;
  Packet* node;

  while (ptr != nullptr) {
    node = ptr;
    ptr = ptr->next_;
    delete node;  
  }

  top_ = nullptr;
}

/** @brief Adds node to the list */
void PacketList::push(Packet* node) {
  if (node != nullptr) {
    if (top_ == nullptr) {
      top_ = node;
    } else {
      node->next_ = top_;
      top_ = node;
    }
  }
}

/** @brief Removes and returns the first item from the stack */
Packet* PacketList::pop() {
  if (top_ == nullptr) {
    return nullptr;
  } else {
    Packet* node = top_;
    top_ = top_->next_;
    return node;
  }
}

/** @brief Decrements the timeout value retransmits packets as necessary */
void PacketList::interval() {
  Packet* prev = nullptr;
  Packet* ptr = top_;

  while (ptr != nullptr) {
    if (--ptr->interval_ == 0) {
      if (++ptr->retrycount_ >= Packet::maxretries) {
        if (prev == nullptr) {
          top_ = ptr->next_;
        } else {
          prev->next_ = ptr->next_;
        }
        delete ptr;
        Serial.println("Packet deleted: Too many packet resends");
      } else {
        ptr->interval_ = Packet::timeout;
        ptr->send(true);
        Serial.println("Packet timeout: Resending packet");
      }
    }
    prev = ptr;
    ptr = ptr->next_;
  }
}

/** @brief Returns a count of the number of items in the stack */
const int PacketList::count() const {
  int count = 0;
  Packet* ptr = top_;
  while (ptr != nullptr) {
    ++count;
    ptr = ptr->next_;
  }
  return count;
}
