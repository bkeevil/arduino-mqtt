#include "message.h"

using namespace mqtt;

/** @brief    Copy constructor */
mqtt::Message::Message(const Message& m) : qos(m.qos), duplicate(m.duplicate), retain(m.retain), data_len_(m.data_len_), data_size_(m.data_len_), data_pos_(m.data_len_) {
  String s;
  topic.setText(m.topic.getText());  //TODO: MQTTTopic, MQTTFilter, and MQTTTokenizer copy and move constructors
  data_ = (byte*) malloc(data_len_);
  memcpy(data_,m.data_,data_len_);
}

/** @brief    Printing a message object prints its data buffer */
size_t mqtt::Message::printTo(Print& p) const {
  size_t pos;
  for (pos=0;pos<data_len_;pos++) { 
    p.print(data_[pos]);
  }
  return data_len_;
}

/** @brief    Reserve size bytes of memory for the data buffer.
 *            Use to reduce the number of memory allocations when the 
 *            size of the data buffer is known before hand. */
void mqtt::Message::reserve(size_t size) {
  if (data_size_ == 0) {
    data_ = (byte*) malloc(size);
  } else {
    data_ = (byte*) realloc(data_,size);
  }  
  data_size_ = size;
}

/** @brief    Free any reserved data buffer memory that is unused 
 *            You can reserve a larger buffer size than necessary and then
 *            call pack() to discard the unused portion. Useful when the size 
 *            of the data buffer is not easily determined. 
*/
void mqtt::Message::pack() {
  if (data_size_ > data_len_) {
    if (data_len_ == 0) {
      free(data_);
      data_ = NULL;
    } else {
      data_ = (byte *)realloc(data_,data_len_);
    }
    data_size_ = data_len_;
  }
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The C string to compare the data buffer with
 *  @remark   For a case insensitive compare see equalsIgnoreCase()
 */
bool mqtt::Message::dataEquals(const char* str) const {
  const String s(str);
  String d((char*)data_);
  return s.equals(d);
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The String object to compare the data buffer with
 *  @remark   For a case insensitive compare see equalsIgnoreCase()
 */
bool mqtt::Message::dataEquals(const String& str) const {
  String d((char*)data_);
  return str.equals(d);
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The C string to compare the data buffer with
 *  @remark   For a case sensitive compare see equals()
 */
bool mqtt::Message::dataEqualsIgnoreCase(const char* str) const {
  const String s(str);
  String d((char*)data_);
  return s.equalsIgnoreCase(d);
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The String object to compare the data buffer with
 *  @remark   For a case insensitive compare see equals()
 */
bool mqtt::Message::dataEqualsIgnoreCase(const String& str) const {
  String d((char*)data_);
  return str.equalsIgnoreCase(d);
}

/** @brief   Reads a single byte from the data buffer and advances to the next character
 *  @returns -1 if there is no more data to be read
 */
int mqtt::Message::read() {
  if (data_pos_ < data_len_) {
    return data_[data_pos_++];
  } else {
    return -1;
  }
}

/** @brief   Reads a single byte from the data buffer without advancing to the next character
 *  @returns -1 if there is no more data to be read
 */
int mqtt::Message::peek() const {
  if (data_pos_ < data_len_) {
    return data_[data_pos_];
  } else {
    return -1;
  }
}

/** @brief  Writes a byte to the data buffer */
size_t mqtt::Message::write(const byte c) {
  data_len_++;
  if (data_size_ == 0) {
    data_ = (byte *) malloc(msgAllocBlockSize);
  } else if (data_len_ >= data_size_) {
    data_size_ += msgAllocBlockSize;
    data_ = (byte *) realloc(data_,data_size_);
  }
  data_[data_pos_++] = c; 
  return 1;
}
 
/** @brief Writes a block of data to the internal message data buffer */
size_t mqtt::Message::write(const byte* buffer, const size_t size) {
  data_len_ += size;
   if (data_size_ == 0) {
    data_ = (byte *) malloc(data_len_);
    data_size_ = data_len_;
  } else if (data_len_ >= data_size_) {
    data_size_ = data_len_;
    data_ = (byte *) realloc(data_,data_size_);
  }
  return size;
}
