#include "mqtt.h"

/* MQTTPacket */

/** @brief Copy constructor */
MQTTPacket::MQTTPacket(const MQTTPacket& rhs) {
  
}

/** @brief Move constructor */
MQTTPacket::MQTTPacket(MQTTPacket&& rhs) noexcept {

}

/** @brief Copy assignment operator */
MQTTPacket& MQTTPacket::operator=(const MQTTPacket& rhs) {
  if (this == &rhs) return *this;
} 

/** @brief Move assignment operator */
MQTTPacket& MQTTPacket::operator=(MQTTPacket&& rhs) noexcept {
  if (this == &rhs) return *this;
}

/** @brief Clears and destroys the stack */
void MQTTPacket::stack_clear(MQTTPacket* top) {
  MQTTPacket* ptr = top;
  MQTTPacket* node;

  while (ptr != nullptr) {
    node = ptr;
    ptr = ptr->next;
    delete node;  
  }

  top = nullptr;
}

/** @brief Adds node to the top of the stack */
void MQTTPacket::stack_push(MQTTPacket* top, MQTTPacket* node) {
  if (node != nullptr) {
    if (top == nullptr) {
      top = node;
    } else {
      node->next = top;
      top = node;
    }
  }
}

/** @brief Removes and returns the first item from the stack */
MQTTPacket* MQTTPacket::stack_pop(MQTTPacket* top) {
  if (top == nullptr) {
    return nullptr;
  } else {
    MQTTPacket* node = top;
    top = top->next;
    return node;
  }
}

/** @brief Decrements the timeout value retransmits packets as necessary */
void MQTTPacket::stack_interval(MQTTPacket* top) {
  MQTTPacket* prev = nullptr;
  MQTTPacket* ptr = top;

  while (ptr != nullptr) {
    if (--ptr->timeout == 0) {
      if (++ptr->retries >= MQTT_PACKET_RETRIES) {
        if (prev == nullptr) {
          top = ptr->next;
        } else {
          prev->next = ptr->next;
        }
        delete ptr;
        Serial.println("Packet deleted: Too many packet resends");
      } else {
        ptr->timeout = MQTT_PACKET_TIMEOUT;
        ptr->retransmit();
        Serial.println("Packet timeout: Resending packet");
      }
    }
    prev = ptr;
    ptr = ptr->next;
  }
}

/** @brief Returns a count of the number of items in the stack */
const int MQTTPacket::stack_count(const MQTTPacket* top) {
  int count = 0;
  MQTTPacket* ptr = top;
  while (ptr != nullptr) {
    ++count;
    ptr = ptr->next;
  }
  return count;
}

/* MQTTTokenizer */

/** @brief  Copy Constructor */
MQTTTokenizer::MQTTTokenizer(const MQTTTokenizer& rhs) {
  count = rhs.count;
  valid = rhs.valid;
  if (rhs.first == nullptr) {
    first = nullptr;
  } else {
    first = new MQTTToken(*rhs.first);
    MQTTToken* ptr_rhs = rhs.first->next;
    MQTTToken* ptr_lhs = first;
    while (ptr_rhs != nullptr) {
      ptr_lhs->next = new MQTTToken(*ptr_rhs);
      ptr_lhs = ptr_lhs->next;
      ptr_rhs = ptr_rhs->next;
    }
  }
}
  
/** @brief  Move Constructor */
MQTTTokenizer::MQTTTokenizer(MQTTTokenizer&& rhs) {
  count = rhs.count;
  valid = rhs.valid;
  first = rhs.first;
  rhs.first = nullptr;
  rhs.count = 0;
  rhs.valid = false;
}

/** @brief  Move assignment operator */

MQTTTokenizer& MQTTTokenizer::operator=(MQTTTokenizer&& rhs) {
  if (&rhs == this) return *this;
  count = rhs.count;
  valid = rhs.valid;
  first = rhs.first;
  rhs.first = nullptr;
  rhs.count = 0;
  rhs.valid = false;
  return *this;  
}

/** @brief  Parse a topic name or topic filter string into a linked list of tokens
 *  @param  text  The topic or filter sting to Parse
 */
void MQTTTokenizer::tokenize() {
  clear();
  MQTTToken* ptr = nullptr;
  int len = text.length();

  if (len > 0) {
    // Strip off ending '/' character before starting recursive tokenize routine
    if (text.lastIndexOf('/') == len - 1) {
      // Don't start tokenize if text="/"
      if (len > 1) {
        _tokenize(text.substring(0,len-1), ptr);
      }
    } else {
      _tokenize(text, ptr);
    }
  }
}

/** @brief  Recursively tokenize text and add the tokens to the linked list.
 *  @remark Called by tokenize()
 */
void MQTTTokenizer::_tokenize(String text, MQTTToken* ptr) {
  MQTTToken* node = new MQTTToken;

  if (ptr == nullptr) {
    first = node;
  } else {
    ptr->next = node;
  }
      
  ++count;
  
  int pos = text.indexOf('/');
  if (pos < 0) {
    node->text = text;
  } else {
    node->text = text.substring(0,pos);
    if (pos < text.length() - 1) {
      _tokenize(text.substring(pos+1),node); 
    }
  }
}

/*String& MQTTTokenizer::getString(String& s) const {
  int i=0;
  int size=0;
  MQTTToken* ptr;
 
  // Determine how long the string needs to be
  ptr = first;
  while (ptr != nullptr) {
    switch (ptr->kind) {
      case tkSingleLevel: ++size; break;
      case tkMultiLevel: ++size; break;
      case tkValid: size += ptr->text.length(); break;
    }    
    ptr = ptr->next;
  }

  s = "";
  s.reserve(size); 

  ptr = first;
  while (ptr != nullptr) {
    switch (ptr->kind) {
      case tkSingleLevel: s += '+'; break;
      case tkMultiLevel: s += '#'; break;
      case tkValid: s += ptr->text; break;
    }
    if (i < count - 1) {
      s += '/';
    }
    ++i;
    ptr = ptr->next;
  }

  return s;
}*/

void MQTTTokenizer::clear() {
  MQTTToken* ptr;
  while (first != nullptr) {
    ptr = first->next;
    delete first;
    first = ptr;
  }
  first = nullptr;
  count = 0;
  text = "";
}

/* MQTTTopic */

/** @brief    Validates the tokenized topic string.
 *  @returns  True if the topic string is valid.
 *  @remark   An empty topic string is invalid
 *  @remark   If the topic string is found to be invalid the token list is cleared
 */
bool MQTTTopic::validate() {
  MQTTToken* ptr;

  // An empty topic string is invalid
  if (count == 0) return false;
  
  ptr = first;
  while (ptr != nullptr) {
    if (!validateToken(*ptr)) {
      clear();
      return false;
    }
    ptr = ptr->next;
  }

  return true;
}

/** @brief    Validates a token as a topic name. 
 *            Sets the token kind to tkValid or tkInvalid.
 *  @param    token   A reference to the token to validate
 *  @remark   Empty string is valid
 *  @remark   Any token containing a hash or a plus is invalid
 *  @returns  True if the token is valid 
 */ 
bool MQTTTopic::validateToken(MQTTToken& token) {   // TODO: Should token be const?
  if ((token.text.length() == 0) || ((token.text.indexOf('#') == -1) && (token.text.indexOf('+') == -1))) {
    token.kind = tkValid;
    return true;
  } else {
    token.kind = tkInvalid;
    return false;
  }
}

/* MQTTFilter */

/** @brief    Validates the tokenized filter string.
 *  @returns  True if the filter string is valid.
 *  @remark   An empty topic string is invalid
 *  @remark   If the topic string is determined to be invalid, the token list is cleared.
 */
bool MQTTFilter::validate() {
  MQTTToken* ptr;

  // An empty filter string is invalid
  if (first == nullptr) {
    return false;
  }

  ptr = first;
  while (ptr != nullptr) {
    if (!validateToken(*ptr)) {
      clear();
      return false;
    }
    ptr = ptr->next;
  }

  return true;
}

/** @brief    Validates a token as a topic name. 
 *            Sets the token kind to tkValid or tkInvalid.
 *  @param    token   The token to validate
 *  @remark   An empty string is always valid
 *  @remark   Any token not containing a special char is valid
 *  @remark   The hash character must only appear on its own
 *  @remark   The hash character must only be in the last in the list of tokens
 *  @remark   The plus character must only appear on its own
 *  @returns  True if the token is valid 
 */ 
bool MQTTFilter::validateToken(MQTTToken& token) { // TODO: Should token be const?
  size_t len;
  int hashPos;
  int plusPos;
  
  // An empty string is always valid
  len = token.text.length();
  if (len == 0) {
    token.kind = tkValid;
    return true;
  }

  hashPos = token.text.indexOf('#');
  plusPos = token.text.indexOf('+');

  // Any token not containing a special char is valid
  if ((hashPos == -1) && (plusPos == -1)) {
    token.kind = tkValid;
    return true;
  }

  // The hash and plus character must only appear on their own
  // The hash character must only be in the last in the list of tokens
  if ((hashPos > 0) || (plusPos > 0) || ((hashPos == 0) && (token.next != nullptr))) {
    token.kind = tkInvalid;
    return false;
  } 
  
  // Token is valid but set the token kind enum for special chars
  if (hashPos == 0) {
    token.kind = tkMultiLevel;
  } else if (plusPos == 0) {
    token.kind = tkSingleLevel;
  } else {
    token.kind = tkValid;
  }
  
  return true;
}

/** @brief    Returns true if the topic string matches the filter
 *  @param    topic The topic to match
 *  @Returns  True if the topic matches the filter
 */   
bool MQTTFilter::match(const MQTTTopic& topic) const {
  int i = 0;
  bool result = false;

  MQTTToken* filterPtr;
  MQTTToken* topicPtr;
  String s;

  filterPtr = first;
  topicPtr  = topic.first;

  while (filterPtr != nullptr) {
    if (i >= topic.count) {
      return (result && (filterPtr->kind == tokenKind_t::tkMultiLevel));
    }
    
    if (filterPtr->kind == tkInvalid) { 
      return false;
    } else if (filterPtr->kind == tokenKind_t::tkValid) {
      result = (filterPtr->text == topicPtr->text);
      if (!result) return result;
    } else if (filterPtr->kind == tkMultiLevel) {
      return true;
    } else {
      result = true;
    }
    
    ++i;
    filterPtr = filterPtr->next;
    topicPtr = topicPtr->next;
  }
  
  if (count < topic.count) {
    // Retrieve the count - 1 token from the filter list
    i = 0;
    filterPtr = first;
    while ((filterPtr != nullptr) && (i < count - 1)) {
      ++i;
      filterPtr = filterPtr->next;
    }
    result = (result && (filterPtr != nullptr) && filterPtr->kind == tkMultiLevel);
  }

  return result;
}

/** @brief    Returns true if the filters are equal
 *  @param    filter  The filter to compare
 *  @Returns  True if the filters are an exact match
 */   
bool MQTTFilter::equals(const MQTTFilter& filter) const {
  MQTTToken* lhs = first;
  MQTTToken* rhs = filter.first;

  while (lhs != nullptr) {
    if ((rhs == nullptr) || (lhs->kind == tkInvalid) || (rhs->kind == tkInvalid) || (lhs->kind != rhs->kind) || ((lhs->kind == tkValid) && !lhs->text.equals(rhs->text))) {
      return false;
    }
    lhs = lhs->next;
    rhs = rhs->next;
  }
  
  return (rhs == nullptr);
}

/* MQTTSubscription */


/* MQTTSubscriptionList */

void MQTTSubscriptionList::clear() {
  MQTTSubscription* ptr = first;
  while (ptr != nullptr) {
    first = ptr->next;
    delete ptr;
    ptr = first;
  }
}

void MQTTSubscriptionList::push(MQTTSubscriptionList& subs) {
  MQTTSubscription* ptr;
  MQTTSubscription* rhs_ptr;
  MQTTSubscription* lhs_ptr;

  rhs_ptr = subs.first;
  subs.first = nullptr;
  while (rhs_ptr != nullptr) {
    lhs_ptr = find(rhs_ptr->filter);
    if (lhs_ptr == nullptr) {
      lhs_ptr = new MQTTSubscription(std::move(*rhs_ptr));
      push(lhs_ptr);
    } else {
      lhs_ptr->filter = std::move(rhs_ptr->filter);
      lhs_ptr->handler_ = rhs_ptr->handler_;
      lhs_ptr->qos = rhs_ptr->qos;
      lhs_ptr->sent = rhs_ptr->sent;
    }
    ptr = rhs_ptr;
    rhs_ptr = rhs_ptr->next;
    delete ptr;
  }

}

MQTTSubscription* MQTTSubscriptionList::find(MQTTFilter& filter) {
  MQTTSubscription* ptr = first;
  while (ptr != nullptr) {
    if (ptr->filter.equals(filter)) return ptr;
    ptr = ptr->next;
  }
  return nullptr;
}

/* MQTTMessage */

/** @brief    Copy constructor */
MQTTMessage::MQTTMessage(const MQTTMessage& m): qos(m.qos), duplicate(m.duplicate), retain(m.retain), data_len(m.data_len), data_size(m.data_len), data_pos(m.data_len) {
  String s;
  topic.setText(m.topic.text);  //TODO: MQTTTopic, MQTTFilter, and MQTTTokenizer copy and move constructors
  data = (byte*) malloc(data_len);
  memcpy(data,m.data,data_len);
}

/** @brief    Printing a message object prints its data buffer */
size_t MQTTMessage::printTo(Print& p) const {
  size_t pos;
  for (pos=0;pos<data_len;pos++) { 
    p.print(data[pos]);
  }
  return data_len;
}

/** @brief    Reserve size bytes of memory for the data buffer.
 *            Use to reduce the number of memory allocations when the 
 *            size of the data buffer is known before hand. */
void MQTTMessage::reserve(size_t size) {
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
void MQTTMessage::pack() {
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
bool MQTTMessage::equals(const char* str) const {
  const String s(str);
  String d((char*)data);
  return s.equals(d);
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The String object to compare the data buffer with
 *  @remark   For a case insensitive compare see equalsIgnoreCase()
 */
bool MQTTMessage::equals(const String& str) const {
  String d((char*)data);
  return str.equals(d);
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The C string to compare the data buffer with
 *  @remark   For a case sensitive compare see equals()
 */
bool MQTTMessage::equalsIgnoreCase(const char* str) const {
  const String s(str);
  String d((char*)data);
  return s.equalsIgnoreCase(d);
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The String object to compare the data buffer with
 *  @remark   For a case insensitive compare see equals()
 */
bool MQTTMessage::equalsIgnoreCase(const String& str) const {
  String d((char*)data);
  return str.equalsIgnoreCase(d);
}

/** @brief   Reads a single byte from the data buffer and advances to the next character
 *  @returns -1 if there is no more data to be read
 */
int MQTTMessage::read() {
  if (data_pos < data_len) {
    return data[data_pos++];
  } else {
    return -1;
  }
}

/** @brief   Reads a single byte from the data buffer without advancing to the next character
 *  @returns -1 if there is no more data to be read
 */
int MQTTMessage::peek() const {
  if (data_pos < data_len) {
    return data[data_pos];
  } else {
    return -1;
  }
}

/** @brief  Writes a byte to the data buffer */
size_t MQTTMessage::write(const byte c) {
  data_len++;
  if (data_size == 0) {
    data = (byte *) malloc(MQTT_MESSAGE_ALLOC_BLOCK_SIZE);
  } else if (data_len >= data_size) {
    data_size += MQTT_MESSAGE_ALLOC_BLOCK_SIZE;
    data = (byte *) realloc(data,data_size);
  }
  data[data_pos++] = c; 
  return 1;
}
 
/** @brief Writes a block of data to the internal message data buffer */
size_t MQTTMessage::write(const byte* buffer, const size_t size) {
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

/* MQTTMessageQueue */

void MQTTMessageQueue::clear() {
  queuedMessage_t* ptr = first;
  while (first != NULL) {
    ptr = first;
    first = first->next;
    count--;
    if (ptr->message != NULL)
      delete ptr->message;
    free(ptr);
  }
  first = NULL;
  last = NULL;
}

void MQTTPUBLISHQueue::resend(queuedMessage_t* qm) { 
  #ifdef DEBUG
  Serial.println("Resending PUBLISH packet");
  #endif
  qm->message->duplicate = true; 
  client->sendPUBLISH(qm->message); 
}

void MQTTPUBRECQueue::resend(queuedMessage_t* qm) { 
  #ifdef DEBUG
  Serial.println("Resending PUBREC packet");
  #endif
  client->sendPUBREC(qm->packetid); 
}

void MQTTPUBRELQueue::resend(queuedMessage_t* qm) { 
  #ifdef DEBUG
  Serial.println("Resending PUBREL packet");
  #endif
  client->sendPUBREL(qm->packetid); 
}

/* MQTTBase */

bool MQTTBase::readRemainingLength(long* value) {
  long multiplier = 1;
  int i;
  byte encodedByte;

  *value = 0;
  do {
    i = stream.read();
    if (i > -1) {
      encodedByte = i;
      *value += (encodedByte & 127) * multiplier;
      multiplier *= 128;
      if (multiplier > 2097152) {
        return false;
      }
    } else {
      return false;
    }
  } while ((encodedByte & 128) > 0);
  return true;
}

bool MQTTBase::writeRemainingLength(const long value) {
  byte encodedByte;
  long lvalue;

  lvalue = value;
  do {
    encodedByte = lvalue % 128;
    lvalue = lvalue / 128;
    if (lvalue > 0) {
      encodedByte |= 128;
    }
    if (stream.write(encodedByte) != 1) {
      return false;
    }
  } while (lvalue > 0);
  return true;
}
 
bool MQTTBase::readWord(word* value) {
  int i;
  byte b;
  i = stream.read();
  if (i > -1) {
    b = i;
    *value = b << 8;
    i = stream.read();
    if (i > -1) {
      b = i;
      *value |= b;
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool MQTTBase::writeWord(const word value) {
  byte b = value >> 8;
  if (stream.write(b) == 1) {
    b = value & 0xFF;
    if (stream.write(b) == 1) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool MQTTBase::readStr(String& str) {
  word len;

  if (stream.available() < len) {
    if (readWord(&len)) {
      str.reserve(len);
      str = stream.readString();
      return (str.length() == len);
    } else {
      return false;
    }
  }
}

bool MQTTBase::writeStr(const String& str) {
  word len;

  len = str.length();
  if (writeWord(len)) {
    return  (stream.print(str) == str.length());
  } else {
    return false;
  }
}

/* MQTT Client */

void MQTTClient::reset() {
  pingIntervalRemaining = 0;
  pingCount = 0;
  PUBRECQueue.clear();
  PUBLISHQueue.clear();
  PUBRELQueue.clear();
  isConnected = false;
}

bool MQTTClient::connect(const String& clientID, const String& username, const String& password, const bool cleanSession, const word keepAlive) {
  byte flags;
  word rl;      // Remaining Length

  reset();
  
  #ifdef DEBUG
  Serial.print("Logging in as clientID: "); Serial.print(clientID);
  Serial.print(" username: "); Serial.println(username);
  #endif
  
  rl = 10 + 2 + clientID.length();

  if (username.length() > 0) {
    flags = 128;
    rl += username.length() + 2;
  } else {
    flags = 0;
  }

  if (password.length() > 0) {
    flags |= 64;
    rl += password.length() + 2;
  }

  if (willMessage.retain) {
    flags |= 32;
  }

  String& wmt = willMessage.topic.getText();

  flags |= (willMessage.qos << 3);
  if (willMessage.enabled) {
    flags |= 4;
    rl += wmt.length() + 2 + willMessage.data_len + 2;
  }

  if (cleanSession) {
    flags |= 2;
    PUBLISHQueue.clear();
    PUBRECQueue.clear();
    PUBRELQueue.clear();    
  }

  if ( (stream.write((byte)0x10) != 1) ||
       (!writeRemainingLength(rl)) ||
       (stream.write((byte)0) != 1) ||
       (stream.write((byte)4) != 1) ||
       (stream.write("MQTT") != 4) ||
       (stream.write((byte)4) != 1) 
     ) return false;

  if ( (stream.write(flags) != 1) ||
       (!writeWord(keepAlive)) ||
       (!writeStr(clientID))
     ) return false;

  if (willMessage.enabled) {
    if (!writeStr(wmt) || !writeWord(willMessage.data_len) || (stream.write(willMessage.data,willMessage.data_len) != willMessage.data_len)) {
      return false;
    }
  }

  if (username != NULL) {
    if (!writeStr(username)) {
      return false;
    }
  }

  if (password != NULL) {
    if (!writeStr(password)) {
      return false;
    }
  }

  stream.flush();

  pingIntervalRemaining = MQTT_DEFAULT_PING_INTERVAL;

  return true;
}

byte MQTTClient::recvCONNACK() {
  byte b;
  bool sessionPresent = false;
  byte returnCode = MQTT_CONNACK_SUCCESS;    // Default return code is success

  if (isConnected) return MQTT_ERROR_ALREADY_CONNECTED;

  int i = stream.read();
  if (i > -1) {
    sessionPresent = (i == 1);
  } else {
    return MQTT_ERROR_INSUFFICIENT_DATA;
  }

  if ((b & 0xFE) > 0) return MQTT_ERROR_PACKET_INVALID;

  i = stream.read();
  if (i > -1) {
    returnCode = i;
  } else {
    return MQTT_ERROR_INSUFFICIENT_DATA;
  }

  if (returnCode == MQTT_CONNACK_SUCCESS) {
    pingIntervalRemaining = MQTT_DEFAULT_PING_INTERVAL;
    pingCount = 0;
    connected();
    if (!sessionPresent) {
      initSession();
    }
    return MQTT_ERROR_NONE;
  } else {
    switch (returnCode) {
      case MQTT_CONNACK_UNACCEPTABLE_PROTOCOL : return MQTT_ERROR_UNACCEPTABLE_PROTOCOL;
      case MQTT_CONNACK_CLIENTID_REJECTED     : return MQTT_ERROR_CLIENTID_REJECTED;
      case MQTT_CONNACK_SERVER_UNAVAILABLE    : return MQTT_ERROR_SERVER_UNAVAILABLE;
      case MQTT_CONNACK_BAD_USERNAME_PASSWORD : return MQTT_ERROR_BAD_USERNAME_PASSWORD;
      case MQTT_CONNACK_NOT_AUTHORIZED        : return MQTT_ERROR_NOT_AUTHORIZED;
    }
  }
  return MQTT_ERROR_UNKNOWN;
}

void MQTTClient::connected() {
  isConnected = true;
  if (stream.available() > 0) {
    dataAvailable();
  }
  if (connectMessage.enabled) {
    publish(connectMessage);
  }
}

void MQTTClient::disconnect() {
  if (disconnectMessage.enabled) {
    disconnectMessage.qos = qtAT_MOST_ONCE;
    publish(disconnectMessage);
  }
  stream.write((byte)0xE0);
  stream.write((byte)0);
  isConnected = false;
}

void MQTTClient::disconnected() {
  #ifdef DEBUG
  Serial.println("Server terminated the MQTT connection");
  #endif
  isConnected = false;
  pingIntervalRemaining = 0;
}

bool MQTTClient::sendPINGREQ() {
  #ifdef DEBUG
  Serial.println("sendPINGREQ");
  #endif
  bool result;
  if (isConnected) {
    result = (stream.write((byte)12 << 4) == 1);
    result &= (stream.write((byte)0) == 1);
    return result;
  } else {
    return false;
    #ifdef DEBUG
    Serial.println("sendPINGREQ failed");
    #endif
  }
}

byte MQTTClient::pingInterval() {
  if (pingIntervalRemaining == 1) {
    if (pingCount >= 2) {
      pingCount = 0;
      pingIntervalRemaining = 0;
      return MQTT_ERROR_NO_PING_RESPONSE;
    }
    sendPINGREQ();
    if (pingCount == 0) {
      pingIntervalRemaining = MQTT_DEFAULT_PING_INTERVAL;
    } else {
      pingIntervalRemaining = MQTT_DEFAULT_PING_RETRY_INTERVAL;
    }
    pingCount++;
  } else {
    if (pingIntervalRemaining > 1) {
      pingIntervalRemaining--;
    }
  }
  return MQTT_ERROR_NONE;
}

bool MQTTClient::queueInterval() {
  bool result;

  result = PUBLISHQueue.interval();
  result &= PUBRECQueue.interval();
  result &= PUBRELQueue.interval();

  return result;
}

byte MQTTClient::intervalTimer() {
  if (!queueInterval()) {
    return MQTT_ERROR_PACKET_QUEUE_TIMEOUT;
  } else {
    return pingInterval();
  }
}

word MQTTClient::getNextPacketID() {
  if (nextPacketID == 65535) {
    nextPacketID = 0;
  } else
    nextPacketID++;
  
  return nextPacketID;
}

bool MQTTClient::subscribe(MQTTSubscriptionList& subs) {
  if (isConnected) sendSUBSCRIBE(subs);
  subscriptions_.push(subs);
}

bool MQTTClient::subscribe(const String& filter, const qos_t qos, const MQTTMessageHandlerFunc handler) {
  MQTTSubscriptionList subs;
  MQTTSubscription* sub = new MQTTSubscription(filter,qos,handler);
  if (sub->filter.valid) {
    subs.push(sub);
    subscribe(subs);
  }
}

/** @brief  Creates an MQTT Subscribe packet from a list of subscriptions and sends it to the server */
bool MQTTClient::sendSUBSCRIBE(const MQTTSubscriptionList& subscriptions) {
  bool result;
  MQTTSubscription* sub;
  int rl = 2;
  word packetid = getNextPacketID();

  /** The payload of a SUBSCRIBE packet MUST contain at least one Topic Filter / QoS pair. 
   * A SUBSCRIBE packet with no payload is a protocol violation [MQTT-3.8.3-3] */
  if (subscriptions.first == nullptr) return false;
  
  sub = subscriptions.first;
  while (sub != nullptr) {
    rl += (3 + sub->filter.getText().length());  // Not very efficient
    sub = sub->next;
  }

  result = (stream.write((byte)0x82) == 1);
  result &= writeRemainingLength(rl);
  result &= writeWord(packetid);

  if (!result) return result;

  sub = subscriptions.first;
  while (sub != nullptr) {
    result &= writeStr(sub->filter.getText());
    result &= (stream.write(sub->qos) == 1);
    if (!result) return result;
    sub = sub->next;
  }
    
  MQTTSubscriptionListWaitingAck* node = new MQTTSubscriptionListWaitingAck;
  
  node->packetid = packetid;
  node->list = subscriptions;
  node->retries = 0;
  node->timeout = MQTT_PACKET_TIMEOUT;
  node->next = nullptr;

  if (subsWaitingAck_ == nullptr) {
    subsWaitingAck_ = node;
  } else {
    MQTTSubscriptionListWaitingAck* ptr = subsWaitingAck_;
    while (ptr != nullptr) {
      if (ptr->next = nullptr) {
        ptr->next = node;
        ptr = nullptr;
      } else {
        ptr = ptr->next;
      }
    }
  }
  
  return result;
}

byte MQTTClient::recvSUBACK(const long remainingLength) {
  int i;
  byte rc;
  long rl;
  word packetid;

  #ifdef DEBUG
  Serial.println("Received SUBACK");
  #endif

  if (!isConnected) {
    return MQTT_ERROR_NOT_CONNECTED;
  }

  if (readWord(&packetid)) {
    rl = remainingLength-2;
    while (rl-- > 0) {
      i = stream.read();
      if (i > -1) {
        rc = i;
        subscribed(packetid,rc);
      } else {
        return MQTT_ERROR_PAYLOAD_INVALID;
      }
    }
    return MQTT_ERROR_NONE;
  } else {
    return MQTT_ERROR_VARHEADER_INVALID;
  }
}

bool MQTTClient::unsubscribe(const String& filter) {
  /*
  bool result;

  if (filter != NULL) {
    result = (stream.write((byte)0xA2) == 1);
    result &= writeRemainingLength(2+2+filter.length());
    result &= writeWord(packetid);
    result &= writeStr(filter);
    return result;
  } else {
    return false;
  }*/
}

byte MQTTClient::recvUNSUBACK() {
  word packetid;

  #ifdef DEBUG
  Serial.println("Received UNSUBACK");
  #endif

  if (!isConnected) {
    return MQTT_ERROR_NOT_CONNECTED;
  }
  if (readWord(&packetid)) {
    unsubscribed(packetid);
    return MQTT_ERROR_NONE;
  } else {
    return MQTT_ERROR_VARHEADER_INVALID;
  }
}

bool MQTTClient::publish(const String& topic, byte* data, const size_t data_len, const qos_t qos, const bool retain) {
  MQTTMessage* msg = new MQTTMessage(topic);
  msg->qos = qos;
  msg->retain = retain;
  msg->data = data;
  msg->data_len = data_len;
  return sendPUBLISH(msg);
}

bool MQTTClient::publish(const String& topic, const String& data, const qos_t qos, const bool retain) {
  MQTTMessage* msg = new MQTTMessage(topic);
  msg->qos = qos;
  msg->retain = retain;
  msg->data = (byte*)data.c_str();
  msg->data_len = data.length();
  return sendPUBLISH(msg);
}

bool MQTTClient::sendPUBLISH(MQTTMessage* msg) {
  byte flags = 0;
  word packetid;
  long remainingLength;
  bool result;

  if (msg != NULL) {
    #ifdef DEBUG
    Serial.println("sending PUBLISH");
    #endif
    
    String& mt = msg->topic.getText();
    
    if ((mt.length()>0) && (msg->qos < qtMAX_VALUE) && (isConnected)) {

      flags |= (msg->qos << 1);
      if (msg->duplicate) {
        flags |= 8;
      }
      if (msg->retain) {
        flags |= 1;
      }

      remainingLength = 2 + mt.length() + msg->data_len;
      if (msg->qos>0) {
        remainingLength += 2;
      }

      packetid = getNextPacketID();

      #ifdef DEBUG
      Serial.print("flags="); Serial.println(flags);
      Serial.print("qos="); Serial.println(msg->qos);
      Serial.print("packetid="); Serial.println(packetid);
      Serial.print("topic="); Serial.println(mt);
      Serial.print("rl="); Serial.println(remainingLength);
      Serial.print("data_len="); Serial.println(msg->data_len);
      #endif

      result = (
        (stream.write(0x30 | flags) == 1) &&
        writeRemainingLength(remainingLength) &&
        writeStr(mt)
      );

      if (result && (msg->qos > 0)) {
        result = writeWord(packetid);
      }

      if (result && (msg->data != NULL) && (msg->data_len > 0)) {
        result = (stream.write(msg->data,msg->data_len) == msg->data_len);
      }

      if (result && (msg->qos > 0)) {
        Serial.println("Adding message to PUBLISHQueue");
        queuedMessage_t* qm = new queuedMessage_t;
        qm->packetid = packetid;
        qm->timeout = MQTT_PACKET_TIMEOUT;
        qm->retries = 0;
        qm->message = msg;
        PUBLISHQueue.push(qm);
      } else {
        free(msg);
      }

      return result;

    } else { 
      free(msg);
      return false;
    }
  } else return false;
}

byte MQTTClient::recvPUBLISH(const byte flags, const long remainingLength) {
  MQTTMessage* msg;
  queuedMessage_t* qm;
  word packetid=0;
  long rl;
  int i;
  
  #ifdef DEBUG
  Serial.println("Received PUBLISH");
  #endif

  msg = new MQTTMessage();
  String topic;

  msg->duplicate = (flags & 8) > 0;
  msg->retain = (flags & 1) > 0;
  msg->qos = (qos_t)((flags & 6) >> 1);

  if (!isConnected) return MQTT_ERROR_NOT_CONNECTED;

  if (!readStr(topic)) return MQTT_ERROR_VARHEADER_INVALID;

  if (!msg->topic.setString(topic)) return MQTT_ERROR_VARHEADER_INVALID;

  rl = remainingLength - topic.length() - 2;
  
  if (msg->qos>0) {
    if (readWord(&packetid)) {
      rl -= 2;
    } else {
      return MQTT_ERROR_VARHEADER_INVALID;
    }
  }

  if (rl < 0)  {
    i = 0;
  } else {
    i = rl;
  }
  msg->reserve(i);
  msg->data_len = i;

  if (stream.readBytes(msg->data,i) == i) {
    if (msg->qos != qtEXACTLY_ONCE) {
      receiveMessage(*msg);
      if (msg->qos==qtAT_LEAST_ONCE) {
        sendPUBACK(packetid);
      }
      delete msg;
    } else {
      qm = new queuedMessage_t;
      qm->packetid = packetid;
      qm->retries = 0;
      qm->timeout = MQTT_PACKET_TIMEOUT;
      qm->message = msg;
      PUBRECQueue.push(qm);
      sendPUBREC(packetid);
    }
    return MQTT_ERROR_NONE;
  } else {
    return MQTT_ERROR_PAYLOAD_INVALID;
  }
}

byte MQTTClient::recvPUBACK() {
  word packetid;
  int iterations;
  queuedMessage_t *qm;

  #ifdef DEBUG
  Serial.println("Received PUBACK");
  #endif

  if (readWord(&packetid)) {
    #ifdef DEBUG
    Serial.print("  packetid="); Serial.println(packetid);
    #endif
    iterations = PUBLISHQueue.getCount();
    Serial.print("  iterations="); Serial.println(iterations);
    if (iterations > 0) {
      do {
        qm = PUBLISHQueue.pop();
        if (qm->packetid == packetid) {
          delete qm->message;
          free(qm);
          return MQTT_ERROR_NONE;
        }
        PUBLISHQueue.push(qm);
      } while (--iterations > 0);
    }
    return MQTT_ERROR_PACKETID_NOT_FOUND;
  } else {
   return MQTT_ERROR_PAYLOAD_INVALID;
  }
}

bool MQTTClient::sendPUBACK(const word packetid) {
  bool result;
  
  #ifdef DEBUG
  Serial.println("Sending PUBACK");
  #endif

  if (isConnected) {
    result =  (stream.write(0x40) == 1);
    result &= (stream.write(0x02) == 1);
    result &= writeWord(packetid);
    return result;
  } else {
    return false;
  }
}

byte MQTTClient::recvPUBREC() {
  word packetid;
  int iterations;
  queuedMessage_t *qm;

  #ifdef DEBUG
  Serial.println("Received PUBREC");
  #endif

  if (readWord(&packetid)) {
    iterations = PUBLISHQueue.getCount();
    if (iterations > 0) {
      do {
        qm = PUBLISHQueue.pop();
        if (qm->packetid == packetid) {
          delete qm->message;
          free(qm);
          if (sendPUBREL(packetid)) {
            return MQTT_ERROR_NONE;
          } else {
            return MQTT_ERROR_SEND_PUBCOMP_FAILED;
          }
        }
        PUBRELQueue.push(qm);
       } while (--iterations > 0);
    }
    return MQTT_ERROR_PACKETID_NOT_FOUND;
  } else {
   return MQTT_ERROR_PAYLOAD_INVALID;
  }
}

bool MQTTClient::sendPUBREC(const word packetid) {
  bool result;

  #ifdef DEBUG
  Serial.println("Sending PUBREC");
  #endif

  if (isConnected) {
    result =  (stream.write(0x50) == 1);
    result &= (stream.write(0x02) == 1);
    result &= writeWord(packetid);
    return result;
  } else {
    return false;
  }
}

byte MQTTClient::recvPUBREL() {
  word packetid;
  int iterations;
  queuedMessage_t *qm;

  #ifdef DEBUG
  Serial.println("Received PUBREL");
  #endif

  if (readWord(&packetid)) {
    iterations = PUBRECQueue.getCount();
    if (iterations > 0) {
      do {
        qm = PUBRECQueue.pop();
        if (qm->packetid == packetid) {
          receiveMessage(*qm->message);
          delete qm->message;
          delete(qm);
          if (sendPUBCOMP(packetid)) {
            return MQTT_ERROR_NONE;
          } else {
            return MQTT_ERROR_SEND_PUBCOMP_FAILED;
          }
        }
        PUBRECQueue.push(qm);
      } while (--iterations > 0);
    }
    return MQTT_ERROR_PACKETID_NOT_FOUND;
  } else {
   return MQTT_ERROR_PAYLOAD_INVALID;
  }
}

bool MQTTClient::sendPUBREL(const word packetid) {
  bool result;
  queuedMessage_t* qm;

  #ifdef DEBUG
  Serial.println("Sending PUBREL");
  #endif

  if (isConnected) {
    result =  (stream.write(0x62) == 1);
    result &= (stream.write(0x02) == 1);
    result &= writeWord(packetid);
    if (result) {
      qm = new queuedMessage_t;
      qm->packetid = packetid;
      qm->timeout  = MQTT_PACKET_TIMEOUT;
      qm->retries  = 0;
      qm->message  = NULL;
      PUBRELQueue.push(qm);
    }
    return result;
  } else {
    return false;
  }
}

byte MQTTClient::recvPUBCOMP() {
  word packetid;
  int iterations;
  queuedMessage_t *qm;

  #ifdef DEBUG
  Serial.println("Received PUBCOMP");
  #endif

  if (readWord(&packetid)) {
    iterations == PUBRELQueue.getCount();
    if (iterations > 0) {
      do {
        qm = PUBRELQueue.pop();
        if (qm->packetid == packetid) {
          delete qm;
          return MQTT_ERROR_NONE;
        }
        PUBRELQueue.push(qm);
      } while (--iterations > 0);
    }
    return MQTT_ERROR_PACKETID_NOT_FOUND;
  } else {
   return MQTT_ERROR_PAYLOAD_INVALID;
  }
}

bool MQTTClient::sendPUBCOMP(const word packetid) {
  bool result;

  #ifdef DEBUG
  Serial.println("Sending PUBCOMP");
  #endif

  if (isConnected) {
    result = (stream.write(0x70) == 1);
    result &= (stream.write(0x02) == 1);
    result &= (stream.write(packetid) == 1);
    return result;
  } else {
    return false;
  }
}

byte MQTTClient::dataAvailable() {
  int i;
  byte b;
  byte flags;
  byte packetType;
  long remainingLength=0;

  i = stream.read();
  if (i > -1) {
    b = i;
    flags = b & 0x0F;
    packetType = b >> 4;
  } else {
    return MQTT_ERROR_INSUFFICIENT_DATA;
  }
  
  if (!readRemainingLength(&remainingLength)) {
    return MQTT_ERROR_INSUFFICIENT_DATA;
  }
  
  pingIntervalRemaining = MQTT_DEFAULT_PING_INTERVAL;
  pingCount = 0;

  switch (packetType) {
    case ptCONNACK   : return recvCONNACK(); break;
    case ptSUBACK    : return recvSUBACK(remainingLength); break;
    case ptUNSUBACK  : return recvUNSUBACK(); break;
    case ptPUBLISH   : return recvPUBLISH(flags,remainingLength); break;
    #ifdef DEBUG
    case ptPINGRESP  : Serial.println("PINGRESP"); return MQTT_ERROR_NONE; break;
    #else
    case ptPINGRESP  : return MQTT_ERROR_NONE; break;
    #endif
    case ptPUBACK    : return recvPUBACK(); break;
    case ptPUBREC    : return recvPUBREC(); break;
    case ptPUBREL    : return recvPUBREL(); break;
    case ptPUBCOMP   : return recvPUBCOMP(); break;
    default: return MQTT_ERROR_UNHANDLED_PACKETTYPE;
  }

  return MQTT_ERROR_NONE;
}
