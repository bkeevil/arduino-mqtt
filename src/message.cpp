#include "message.h"

using namespace mqtt;

/** @brief    Copy constructor */
Message::Message(const Message& m): qos(m.qos), duplicate(m.duplicate), retain(m.retain), data_len(m.data_len), data_size(m.data_len), data_pos(m.data_len) {
  String s;
  topic.setText(m.topic.getText());  //TODO: MQTTTopic, MQTTFilter, and MQTTTokenizer copy and move constructors
  data = (byte*) malloc(data_len);
  memcpy(data,m.data,data_len);
}

/** @brief    Printing a message object prints its data buffer */
size_t Message::printTo(Print& p) const {
  size_t pos;
  for (pos=0;pos<data_len;pos++) { 
    p.print(data[pos]);
  }
  return data_len;
}

/** @brief    Reserve size bytes of memory for the data buffer.
 *            Use to reduce the number of memory allocations when the 
 *            size of the data buffer is known before hand. */
void Message::reserve(size_t size) {
  if (data_size == 0) {
    data = (byte*) malloc(size);
  } else {
    data = (byte*) realloc(data,size);
  }  
  data_size = size;
}

/** @brief    Free any reserved data buffer memory that is unused 
 *            You can reserve a larger buffer size than necessary and then
 *            call pack() to discard the unused portion. Useful when the size 
 *            of the data buffer is not easily determined. 
*/
void Message::pack() {
  if (data_size > data_len) {
    if (data_len == 0) {
      free(data);
      data = NULL;
    } else {
      data = (byte *) realloc(data,data_len);
    }
    data_size = data_len;
  }
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The C string to compare the data buffer with
 *  @remark   For a case insensitive compare see equalsIgnoreCase()
 */
bool Message::equals(const char* str) const {
  const String s(str);
  String d((char*)data);
  return s.equals(d);
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The String object to compare the data buffer with
 *  @remark   For a case insensitive compare see equalsIgnoreCase()
 */
bool Message::equals(const String& str) const {
  String d((char*)data);
  return str.equals(d);
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The C string to compare the data buffer with
 *  @remark   For a case sensitive compare see equals()
 */
bool Message::equalsIgnoreCase(const char* str) const {
  const String s(str);
  String d((char*)data);
  return s.equalsIgnoreCase(d);
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The String object to compare the data buffer with
 *  @remark   For a case insensitive compare see equals()
 */
bool Message::equalsIgnoreCase(const String& str) const {
  String d((char*)data);
  return str.equalsIgnoreCase(d);
}

/** @brief   Reads a single byte from the data buffer and advances to the next character
 *  @returns -1 if there is no more data to be read
 */
int Message::read() {
  if (data_pos < data_len) {
    return data[data_pos++];
  } else {
    return -1;
  }
}

/** @brief   Reads a single byte from the data buffer without advancing to the next character
 *  @returns -1 if there is no more data to be read
 */
int Message::peek() const {
  if (data_pos < data_len) {
    return data[data_pos];
  } else {
    return -1;
  }
}

/** @brief  Writes a byte to the data buffer */
size_t Message::write(const byte c) {
  data_len++;
  if (data_size == 0) {
    data = (byte *) malloc(clientConfiguration.msgAllocBlockSize);
  } else if (data_len >= data_size) {
    data_size += clientConfiguration.msgAllocBlockSize;
    data = (byte *) realloc(data,data_size);
  }
  data[data_pos++] = c; 
  return 1;
}
 
/** @brief Writes a block of data to the internal message data buffer */
size_t Message::write(const byte* buffer, const size_t size) {
  data_len += size;
   if (data_size == 0) {
    data = (byte *) malloc(data_len);
    data_size = data_len;
  } else if (data_len >= data_size) {
    data_size = data_len;
    data = (byte *) realloc(data,data_size);
  }
  return size;
}
