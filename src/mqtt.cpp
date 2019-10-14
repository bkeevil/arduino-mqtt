#include "mqtt.h"

namespace mqtt {

/** @brief Return code sent with a CONNACK packet */
enum class CONNACKResult : byte {
  SUCCESS=0,
  UNACCEPTABLE_PROTOCOL,
  CLIENTID_REJECTED,
  SERVER_UNAVAILABLE,
  BAD_USERNAME_PASSWORD,
  NOT_AUTHORIZED
};

/** @brief Used to identify the type of a received packet */
enum class PacketType : byte {
  BROKERCONNECT = 0, 
  CONNECT = 1, 
  CONNACK = 2, 
  PUBLISH = 3, 
  PUBACK = 4,
  PUBREC = 5, 
  PUBREL = 6, 
  PUBCOMP = 7, 
  SUBSCRIBE = 8, 
  SUBACK = 9, 
  UNSUBSCRIBE = 10,
  UNSUBACK = 11, 
  PINGREQ = 12, 
  PINGRESP = 13, 
  DISCONNECT = 14
};

/* MQTTTokenizer */

/** @brief  Parse a topic name or topic filter string into a linked list of tokens
 *  @param  text  The topic or filter sting to Parse
 */
void MQTTTokenizer::tokenize(const String& text) {
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

  String s;
  //Serial.print("  getString="); Serial.println(getString(s));
}

/** @brief  Recursively tokenize text and add the tokens to the linked list.
 *  @remark Called by tokenize()
 */
void MQTTTokenizer::_tokenize(String text, MQTTToken* ptr) {
  MQTTToken* node = new MQTTToken;

  if (ptr == nullptr) {
    first_ = node;
  } else {
    ptr->next = node;
  }
      
  ++count_;
  
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

String& MQTTTokenizer::getString(String& s) const {
  int i=0;
  int size=0;
  MQTTToken* ptr;

  s = "";
 
  ptr = first_;
  while (ptr != nullptr) {
    switch (ptr->kind) {
      case TokenKind::SINGLELEVEL: ++size; break;
      case TokenKind::MULTILEVEL: ++size; break;
      case TokenKind::VALID: size += ptr->text.length(); break;
    }    
    ++size;
    ptr = ptr->next;
  }

  s.reserve(size-1);

  ptr = first_;
  while (ptr != nullptr) {
  
    switch (ptr->kind) {
      case TokenKind::SINGLELEVEL: s += '+'; break;
      case TokenKind::MULTILEVEL: s += '#'; break;
      case TokenKind::VALID: s += ptr->text; break;
    }
    if (i < count_ - 1) {
      s += '/';
    }
    ++i;
    ptr = ptr->next;
  }

  return s;
}

void MQTTTokenizer::clear() {
  MQTTToken* ptr;
  while (first_ != nullptr) {
    ptr = first_->next;
    delete first_;
    first_ = ptr; 
  }
  first_ = nullptr;
  count_ = 0;
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
  if (count() == 0) return false;
  
  ptr = first();
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
 *            Sets the token kind to TokenKind::VALID or TokenKind::INVALID.
 *  @param    token   A reference to the token to validate
 *  @remark   Empty string is valid
 *  @remark   Any token containing a hash or a plus is invalid
 *  @returns  True if the token is valid 
 */ 
bool MQTTTopic::validateToken(MQTTToken& token) {   // TODO: Should token be const?
  if ((token.text.length() == 0) || ((token.text.indexOf('#') == -1) && (token.text.indexOf('+') == -1))) {
    token.kind = TokenKind::VALID;
    return true;
  } else {
    token.kind = TokenKind::INVALID;
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
  if (first() == nullptr) {
    return false;
  }

  ptr = first();
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
 *            Sets the token kind to TokenKind::VALID or TokenKind::INVALID.
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
    token.kind = TokenKind::VALID;
    return true;
  }

  hashPos = token.text.indexOf('#');
  plusPos = token.text.indexOf('+');

  // Any token not containing a special char is valid
  if ((hashPos == -1) && (plusPos == -1)) {
    token.kind = TokenKind::VALID;
    return true;
  }

  // The hash and plus character must only appear on their own
  // The hash character must only be in the last in the list of tokens
  if ((hashPos > 0) || (plusPos > 0) || ((hashPos == 0) && (token.next != nullptr))) {
    token.kind = TokenKind::INVALID;
    return false;
  } 
  
  // Token is valid but set the token kind enum for special chars
  if (hashPos == 0) {
    token.kind = TokenKind::MULTILEVEL;
  } else if (plusPos == 0) {
    token.kind = TokenKind::SINGLELEVEL;
  } else {
    token.kind = TokenKind::VALID;
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

  filterPtr = first();
  topicPtr  = topic.first();

  while (filterPtr != nullptr) {
    if (i >= topic.count()) {
      return (result && (filterPtr->kind == TokenKind::MULTILEVEL));
    }
    
    if (filterPtr->kind == TokenKind::INVALID) { 
      return false;
    } else if (filterPtr->kind == TokenKind::VALID) {
      result = (filterPtr->text == topicPtr->text);
      if (!result) return result;
    } else if (filterPtr->kind == TokenKind::MULTILEVEL) {
      return true;
    } else {
      result = true;
    }
    
    ++i;
    filterPtr = filterPtr->next;
    topicPtr = topicPtr->next;
  }
  
  if (count() < topic.count()) {
    // Retrieve the count - 1 token from the filter list
    i = 0;
    filterPtr = first();
    while ((filterPtr != nullptr) && (i < count() - 1)) {
      ++i;
      filterPtr = filterPtr->next;
    }
    result = (result && (filterPtr != nullptr) && filterPtr->kind == TokenKind::MULTILEVEL);
  }

  return result;
}

/* MQTTMessage */

/** @brief    Copy constructor */
MQTTMessage::MQTTMessage(const MQTTMessage& m): topic(m.topic), qos(m.qos), duplicate(m.duplicate), retain(m.retain), data_len(m.data_len), data_size(m.data_len), data_pos(m.data_len) {
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
 *  @remark   For a case insensitive compare see dataEqualsIgnoreCase()
 */
bool MQTTMessage::dataEquals(const char* str) const {
  const String s(str);
  String d((char*)data);
  return s.equals(d);
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The String object to compare the data buffer with
 *  @remark   For a case insensitive compare see dataEqualsIgnoreCase()
 */
bool MQTTMessage::dataEquals(const String& str) const {
  String d((char*)data);
  return str.equals(d);
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The C string to compare the data buffer with
 *  @remark   For a case sensitive compare see dataEquals()
 */
bool MQTTMessage::dataEqualsIgnoreCase(const char* str) const {
  const String s(str);
  String d((char*)data);
  return s.equalsIgnoreCase(d);
}

/** @brief    Returns true if the message data buffer equals str
 *  @param    str   The String object to compare the data buffer with
 *  @remark   For a case insensitive compare see dataEquals()
 */
bool MQTTMessage::dataEqualsIgnoreCase(const String& str) const {
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

size_t MQTTMessage::write(const byte c) {
  data_len++;
  if (data_size == 0) {
    data = (byte *) malloc(messageAllocBlockSize);
  } else if (data_len >= data_size) {
    data_size += messageAllocBlockSize;
    data = (byte *) realloc(data,data_size);
  }
  data[data_pos++] = c; 
  return 1;
}
 
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
  QueuedMessage* ptr = first;
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

bool MQTTMessageQueue::interval() {
  bool result = true;
  QueuedMessage* qm;

  qm = pop();
  if (qm != NULL) {
    Serial.println("Queue Message Popped");
    if (--qm->timeout == 0) {
      if (++qm->retries >= packetRetries) {
        result = false;
        delete qm->message;
        free(qm);
        Serial.println("Too many packet resent retries");
      } else {
        qm->timeout = packetTimeout;
        push(qm);
        resend(qm);
        Serial.println("Resending packet");
      }
    }
  }
  return result;
}

void MQTTMessageQueue::push(QueuedMessage* qm) {
  
  if (last != NULL) {
    last->next = qm;
    qm->next = NULL;
    last = qm;
  } else {
    first = qm;
    last = qm;
    qm->next = NULL;
  }
  count++;
}

QueuedMessage* MQTTMessageQueue::pop() {
  QueuedMessage* ptr;
  if (first != NULL) {
    ptr = first;
    first = first->next;
    count--;
    if (count == 0) {
      last = NULL;
    }
    return ptr;
  } else {
    return NULL;
  }
}

void MQTTPUBLISHQueue::resend(QueuedMessage* qm) { 
  #ifdef DEBUG
  Serial.println("Resending PUBLISH packet");
  #endif
  qm->message->duplicate = true; 
  client->sendPUBLISH(qm->message); 
}

void MQTTPUBRECQueue::resend(QueuedMessage* qm) { 
  #ifdef DEBUG
  Serial.println("Resending PUBREC packet");
  #endif
  client->sendPUBREC(qm->packetid); 
}

void MQTTPUBRELQueue::resend(QueuedMessage* qm) { 
  #ifdef DEBUG
  Serial.println("Resending PUBREL packet");
  #endif
  client->sendPUBREL(qm->packetid); 
}

/* SubscriptionList */

void SubscriptionList::clear() {
  Subscription* ptr = top_;
  while (ptr != nullptr) {
    top_ = top_->next;
    delete ptr;
  }
}

void SubscriptionList::add(const char* filter, QoS qos) {
  Subscription* ptr = top_;
  while (ptr != nullptr) {
    if (ptr->next == nullptr) {
      ptr->next = new Subscription;
      strcpy(ptr->next->filter,filter);
      ptr->next->qos = qos;
      ptr->next->next = nullptr;
      ptr = nullptr;
    } else {
      ptr = ptr->next;
    }
  }
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

bool MQTTClient::connect(const String& clientID, const String& username, const String& password, const bool cleanSession) {
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

  flags |= (static_cast<byte>(willMessage.qos) << 3);
  if (willMessage.enabled) {
    flags |= 4;
    rl += willMessage.topic.length() + 2 + willMessage.data_len + 2;
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
    if (!writeStr(willMessage.topic) || !writeWord(willMessage.data_len) || (stream.write(willMessage.data,willMessage.data_len) != willMessage.data_len)) {
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

  pingIntervalRemaining = pingInterval;

  return true;
}

ErrorCode MQTTClient::recvCONNACK() {
  byte b;
  bool sessionPresent = false;
  CONNACKResult returnCode = CONNACKResult::SUCCESS;    // Default return code is success

  if (isConnected) return ErrorCode::ALREADY_CONNECTED;

  int i = stream.read();
  if (i > -1) {
    sessionPresent = (i == 1);
  } else {
    return ErrorCode::INSUFFICIENT_DATA;
  }

  if ((b & 0xFE) > 0) return ErrorCode::PACKET_INVALID;

  i = stream.read();
  if (i > -1) {
    returnCode = static_cast<CONNACKResult>(i);
  } else {
    return ErrorCode::INSUFFICIENT_DATA;
  }

  if (returnCode == CONNACKResult::SUCCESS) {
    pingIntervalRemaining = pingInterval;
    pingCount = 0;
    connected();
    if (!sessionPresent) {
      initSession();
    }
    return ErrorCode::NONE;
  } else {
    switch (returnCode) {
      case CONNACKResult::UNACCEPTABLE_PROTOCOL : return ErrorCode::UNACCEPTABLE_PROTOCOL;
      case CONNACKResult::CLIENTID_REJECTED     : return ErrorCode::CLIENTID_REJECTED;
      case CONNACKResult::SERVER_UNAVAILABLE    : return ErrorCode::SERVER_UNAVAILABLE;
      case CONNACKResult::BAD_USERNAME_PASSWORD : return ErrorCode::BAD_USERNAME_PASSWORD;
      case CONNACKResult::NOT_AUTHORIZED        : return ErrorCode::NOT_AUTHORIZED;
    }
  }
  return ErrorCode::UNKNOWN;
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
    disconnectMessage.qos = QoS::AT_MOST_ONCE;
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

ErrorCode MQTTClient::pingIntervalTimer() {
  if (pingIntervalRemaining == 1) {
    if (pingCount >= 2) {
      pingCount = 0;
      pingIntervalRemaining = 0;
      return ErrorCode::NO_PING_RESPONSE;
    }
    sendPINGREQ();
    if (pingCount == 0) {
      pingIntervalRemaining = pingInterval;
    } else {
      pingIntervalRemaining = pingRetryInterval;
    }
    pingCount++;
  } else {
    if (pingIntervalRemaining > 1) {
      pingIntervalRemaining--;
    }
  }
  return ErrorCode::NONE;
}

bool MQTTClient::queueIntervalTimer() {
  bool result;

  result = PUBLISHQueue.interval();
  result &= PUBRECQueue.interval();
  result &= PUBRELQueue.interval();

  return result;
}

ErrorCode MQTTClient::intervalTimer() {
  if (!queueIntervalTimer()) {
    return ErrorCode::PACKET_QUEUE_TIMEOUT;
  } else {
    return pingIntervalTimer();
  }
}

bool MQTTClient::subscribe(const word packetid, const String& filter, const QoS qos) {
  bool result;

  if (filter != NULL) {
    result = (stream.write((byte)0x82) == 1);
    result &= writeRemainingLength(2 + 2 + 1 + filter.length());
    result &= writeWord(packetid);
    result &= writeStr(filter);
    result &= (stream.write(static_cast<byte>(qos)) == 1);
    return result;
  } else {
    return false;
  }
}

ErrorCode MQTTClient::recvSUBACK(const long remainingLength) {
  int i;
  byte rc;
  long rl;
  word packetid;

  #ifdef DEBUG
  Serial.println("Received SUBACK");
  #endif

  if (!isConnected) {
    return ErrorCode::NOT_CONNECTED;
  }

  if (readWord(&packetid)) {
    rl = remainingLength-2;
    while (rl-- > 0) {
      i = stream.read();
      if (i > -1) {
        rc = i;
        subscribed(packetid,rc);
      } else {
        return ErrorCode::PAYLOAD_INVALID;
      }
    }
    return ErrorCode::NONE;
  } else {
    return ErrorCode::VARHEADER_INVALID;
  }
}

bool MQTTClient::unsubscribe(const word packetid, const String& filter) {
  bool result;

  if (filter != NULL) {
    result = (stream.write((byte)0xA2) == 1);
    result &= writeRemainingLength(2+2+filter.length());
    result &= writeWord(packetid);
    result &= writeStr(filter);
    return result;
  } else {
    return false;
  }
}

ErrorCode MQTTClient::recvUNSUBACK() {
  word packetid;

  #ifdef DEBUG
  Serial.println("Received UNSUBACK");
  #endif

  if (!isConnected) {
    return ErrorCode::NOT_CONNECTED;
  }
  if (readWord(&packetid)) {
    unsubscribed(packetid);
    return ErrorCode::NONE;
  } else {
    return ErrorCode::VARHEADER_INVALID;
  }
}

bool MQTTClient::publish(const String& topic, byte* data, const size_t data_len, const QoS qos, const bool retain) {
  MQTTMessage* msg = new MQTTMessage();
  msg->topic = topic;
  msg->qos = qos;
  msg->retain = retain;
  msg->data = data;
  msg->data_len = data_len;
  return sendPUBLISH(msg);
}

bool MQTTClient::publish(const String& topic, const String& data, const QoS qos, const bool retain) {
  MQTTMessage* msg = new MQTTMessage();
  msg->topic = topic;
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
    if ((msg->topic != NULL) && (msg->topic.length()>0) && (msg->qos < QoS::MAX) && (isConnected)) {

      flags |= (static_cast<byte>(msg->qos) << 1);
      if (msg->duplicate) {
        flags |= 8;
      }
      if (msg->retain) {
        flags |= 1;
      }

      remainingLength = 2 + msg->topic.length() + msg->data_len;
      if (static_cast<byte>(msg->qos) > 0) {
        remainingLength += 2;
      }

      packetid = nextPacketID++;
      if (nextPacketID >= maxPacketID) {
        nextPacketID = minPacketID;
      }

      #ifdef DEBUG
      Serial.print("flags="); Serial.println(flags);
      Serial.print("qos="); Serial.println(static_cast<byte>(msg->qos));
      Serial.print("packetid="); Serial.println(packetid);
      Serial.print("topic="); Serial.println(msg->topic);
      Serial.print("rl="); Serial.println(remainingLength);
      Serial.print("data_len="); Serial.println(msg->data_len);
      #endif

      result = (
        (stream.write(0x30 | flags) == 1) &&
        writeRemainingLength(remainingLength) &&
        writeStr(msg->topic)
      );

      if (result && (static_cast<byte>(msg->qos) > 0)) {
        result = writeWord(packetid);
      }

      if (result && (msg->data != NULL) && (msg->data_len > 0)) {
        result = (stream.write(msg->data,msg->data_len) == msg->data_len);
      }

      if (result && (static_cast<byte>(msg->qos) > 0)) {
        Serial.println("Adding message to PUBLISHQueue");
        QueuedMessage* qm = new QueuedMessage;
        qm->packetid = packetid;
        qm->timeout = MQTTMessageQueue::packetTimeout;
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

ErrorCode MQTTClient::recvPUBLISH(const byte flags, const long remainingLength) {
  MQTTMessage* msg;
  QueuedMessage* qm;
  word packetid=0;
  long rl;
  int i;
  
  #ifdef DEBUG
  Serial.println("Received PUBLISH");
  #endif

  msg = new MQTTMessage();

  msg->duplicate = (flags & 8) > 0;
  msg->retain = (flags & 1) > 0;
  msg->qos = (QoS)((flags & 6) >> 1);

  if (!isConnected) return ErrorCode::NOT_CONNECTED;

  if (!readStr(msg->topic)) return ErrorCode::VARHEADER_INVALID;

  rl = remainingLength - msg->topic.length() - 2;
  
  if (static_cast<byte>(msg->qos) > 0) {
    if (readWord(&packetid)) {
      rl -= 2;
    } else {
      return ErrorCode::VARHEADER_INVALID;
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
    if (msg->qos != QoS::EXACTLY_ONCE) {
      receiveMessage(*msg);
      if (msg->qos==QoS::AT_LEAST_ONCE) {
        sendPUBACK(packetid);
      }
      delete msg;
    } else {
      qm = new QueuedMessage;
      qm->packetid = packetid;
      qm->retries = 0;
      qm->timeout = MQTTMessageQueue::packetTimeout;
      qm->message = msg;
      PUBRECQueue.push(qm);
      sendPUBREC(packetid);
    }
    return ErrorCode::NONE;
  } else {
    return ErrorCode::PAYLOAD_INVALID;
  }
}

ErrorCode MQTTClient::recvPUBACK() {
  word packetid;
  int iterations;
  QueuedMessage *qm;

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
          return ErrorCode::NONE;
        }
        PUBLISHQueue.push(qm);
      } while (--iterations > 0);
    }
    return ErrorCode::PACKETID_NOT_FOUND;
  } else {
   return ErrorCode::PAYLOAD_INVALID;
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

ErrorCode MQTTClient::recvPUBREC() {
  word packetid;
  int iterations;
  QueuedMessage *qm;

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
            return ErrorCode::NONE;
          } else {
            return ErrorCode::SEND_PUBCOMP_FAILED;
          }
        }
        PUBRELQueue.push(qm);
       } while (--iterations > 0);
    }
    return ErrorCode::PACKETID_NOT_FOUND;
  } else {
   return ErrorCode::PAYLOAD_INVALID;
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

ErrorCode MQTTClient::recvPUBREL() {
  word packetid;
  int iterations;
  QueuedMessage *qm;

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
            return ErrorCode::NONE;
          } else {
            return ErrorCode::SEND_PUBCOMP_FAILED;
          }
        }
        PUBRECQueue.push(qm);
      } while (--iterations > 0);
    }
    return ErrorCode::PACKETID_NOT_FOUND;
  } else {
   return ErrorCode::PAYLOAD_INVALID;
  }
}

bool MQTTClient::sendPUBREL(const word packetid) {
  bool result;
  QueuedMessage* qm;

  #ifdef DEBUG
  Serial.println("Sending PUBREL");
  #endif

  if (isConnected) {
    result =  (stream.write(0x62) == 1);
    result &= (stream.write(0x02) == 1);
    result &= writeWord(packetid);
    if (result) {
      qm = new QueuedMessage;
      qm->packetid = packetid;
      qm->timeout  = MQTTMessageQueue::packetTimeout;
      qm->retries  = 0;
      qm->message  = NULL;
      PUBRELQueue.push(qm);
    }
    return result;
  } else {
    return false;
  }
}

ErrorCode MQTTClient::recvPUBCOMP() {
  word packetid;
  int iterations;
  QueuedMessage *qm;

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
          return ErrorCode::NONE;
        }
        PUBRELQueue.push(qm);
      } while (--iterations > 0);
    }
    return ErrorCode::PACKETID_NOT_FOUND;
  } else {
   return ErrorCode::PAYLOAD_INVALID;
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

ErrorCode MQTTClient::dataAvailable() {
  int i;
  byte b;
  byte flags;
  PacketType packetType;
  long remainingLength=0;

  i = stream.read();
  if (i > -1) {
    b = i;
    flags = b & 0x0F;
    packetType = static_cast<PacketType>(b >> 4);
  } else {
    return ErrorCode::INSUFFICIENT_DATA;
  }
  
  if (!readRemainingLength(&remainingLength)) {
    return ErrorCode::INSUFFICIENT_DATA;
  }
  
  pingIntervalRemaining = pingInterval;
  pingCount = 0;

  switch (packetType) {
    case PacketType::CONNACK   : return recvCONNACK(); break;
    case PacketType::SUBACK    : return recvSUBACK(remainingLength); break;
    case PacketType::UNSUBACK  : return recvUNSUBACK(); break;
    case PacketType::PUBLISH   : return recvPUBLISH(flags,remainingLength); break;
    #ifdef DEBUG
    case PacketType::PINGRESP  : Serial.println("PINGRESP"); return ErrorCode::NONE; break;
    #else
    case PacketType::PINGRESP  : return ErrorCode::NONE; break;
    #endif
    case PacketType::PUBACK    : return recvPUBACK(); break;
    case PacketType::PUBREC    : return recvPUBREC(); break;
    case PacketType::PUBREL    : return recvPUBREL(); break;
    case PacketType::PUBCOMP   : return recvPUBCOMP(); break;
    default: return ErrorCode::UNHANDLED_PACKETTYPE;
  }

  return ErrorCode::NONE;
}

};