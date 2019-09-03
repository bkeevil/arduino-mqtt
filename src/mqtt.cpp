#include "mqtt.h"

/**
 * @brief Prints the data buffer to any object descended from the Print class 
 * 
 * @param p The object to print to
 * @return size_t The number of bytes printed
 */
size_t MQTTMessage::printTo(Print& p) {
  size_t pos;
  for (pos=0;pos<_data_len;pos++) { 
    p.print(_data[pos]);
  }
  return _data_len;
}

/**
 * @brief Pre-allocate bytes in the data buffer to prevent reallocation and fragmentation
 * 
 * @param size Number of bytes to reserve
 */
void MQTTMessage::reserve(size_t size) {
if (data_size == 0) {
    data = (uint8_t *) malloc(size);
  } else 
    data = (uint8_t *) realloc(data,size);
  }  
  data_size = size;
}

/**
 * @brief Call after writing the data buffer is done to dispose of any extra reserved memory
 * 
 */
void MQTTMessage::pack() {
  if (data_size > data_len) {
    if (data_len == 0) {
      dispose(data);
      data = NULL;
    } else {
      data = (uint8_t *) realloc(data,data_len);
    }
    data_size = data_len;
  }
}

/**
 * @brief Reads a byte from the data buffer and advance to the next character
 * 
 * @return int The byte read, or -1 if there is no more data
 */
int MQTTMessage::read() {
  if (data_pos < data_len) {
    return data[data_pos++];
  } else {
    return -1;
  }
}

/**
 * @brief Read a byte from the data buffer without advancing to the next character
 * 
 * @return int The byte read, or -1 if there is no more data
 */
int MQTTMessage::peek() {
  if (data_pos < data_len) {
    return data[data_pos];
  } else {
    return -1;
  }
}

/**
 * @brief Writes a byte to the end of the data buffer
 * 
 * @param c The byte to be written
 * @return size_t The number of bytes written
 */
size_t MQTTMessage::write(uint8_t c) {
  data_len++;
  if (data_size == 0) {
    data = (uint8_t *) malloc(MQTT_MESSAGE_ALLOC_BLOCK_SIZE);
  } else if (data_len >= data_size) {
    data_size += MQTT_MESSAGE_ALLOC_BLOCK_SIZE;
    data = (uint8_t *) realloc(data,data_size);
  }
  data[data_pos++] = c; 
  return 1;
}

/**
 * @brief Writes a block of data to the data buffer
 * 
 * @param buffer The data to be written
 * @param size The size of the data to be written
 * @return size_t The number of bytes actually written to the buffer
 */
size_t MQTTMessage::write(const uint8_t *buffer, size_t size) {
  data_len += size;
   if (data_size == 0) {
    data = (uint8_t *) malloc(data_len);
    data_size = data_len;
  } else if (data_len >= data_size) {
    data_size = data_len;
    data = (uint8_t *) realloc(data,data_size);
  }
  return size;
}

/* MQTT Message Queue */

void MQTTMessageQueue::clear() {
  queuedMessage_t* ptr = first;
  while (first != null) {
    ptr = first;
    first = first->next;
    count--;
    if (ptr->message != null)
      dispose(ptr->message);
    free(ptr);
  }
}

void MQTTMessageQueue::push(queuedMessage_t *qm) {
  if (last != null) {
    last->next = qm;
    qm->next = null;
    last = qm;
  } else {
    first = qm;
    last = qm;
    qm->next = null;
  }
  count++;
}

queuedMessage_t MQTTMessageQueue::pop() {
  queuedMessage_t* ptr;
  if (first != null) {
    ptr = first;
    first = first->next;
    count--;
    return ptr;
  } else {
    return null;
  }
}

/* MQTTBase */

/** @brief  Reads the remaining length field of an MQTT packet 
 *  @param  value Receives the remaining length
 *  @return True if successful, false otherwise
*/
bool MQTTBase::readRemainingLength(long* value) {
  long multiplier = 1;
  byte encodedByte;

  *value = 0;
  do {
    if (stream.read(encodedByte) == 1) {
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

/** @brief  Writes the remaining length field of an MQTT packet
 *  @param  value The remaining length to write
 *  @return True if successful, false otherwise */
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

/** @brief  Reads a word from the stream in big endian order */
bool MQTTBase::readWord(uint16_t *value) {
  uint8_t b;
  if (stream.read(b) == 1) {
    *value = b << 8;
    if (stream.read(b) == 1) {
      *value |= b;
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

/** @brief  Writes a word to the stream in big endian order */
bool MQTTBase::writeWord(const uint16_t value) {
  uint8_t b = value >> 8;
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

/** @brief    Reads a UTF8 string from the stream in the format required by the MQTT protocol */
bool MQTTBase::readStr(String str&) {
  uint16_t len;

  if (stream.dataAvailable() < len) {
    if (readWord(&len)) {
      str.reserve(len);
      str = stream.readString();
      return (str.length() == len);
    } else {
      return false;
    }
  }
}

/** @brief    Writes a UTF8 string to the stream in the format required by the MQTT protocol */
bool MQTTBase::writeStr(const String str&) {
  uint16_t len;

  len = str.length();
  if (writeWord(len)) {
    return  (stream.write(str) == str.length());
  } else {
    return false;
  }
}

/* MQTT Client */

MQTTClient::MQTTClient() {
  MQTTMessageQueue incommmingPUBLISHQueue; 
  MQTTMessageQueue outgoingPUBLISHQueue;
  MQTTMessageQueue PUBRELQueue;
}

MQTTClient::~MQTTClient() {
  dispose(incommingPUBLISHQueue);
  dispose(outgoingPUBLISHQueue);
  dispose(PUBRELQueue);
}


void MQTTClient::reset() {
  pingIntervalRemaining = 0;
  pingCount = 0;
  incomingPUBLISHQueue.clear();
  outgoingPUBLISHQueue.clear();
  PUBRELQueue.clear();
  isConnected = false;
}

bool MQTTClient::connect(const String clientID&, const String username&, const String password&, const bool cleanSession = false, const word keepAlive = MQTT_DEFAULT_KEEPALIVE) {
  byte flags;
  word rl;      // Remaining Length

  reset();

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

  flags |= (willMessage.qos << 3);
  if (willMessage.enabled) {
    flags |= 4;
    rl += willMessage.topic.length() + 2 + willMessage.data.length() + 2;
  }

  if (cleanSession) {
    flags |= 2;
  }

  if ( (stream.write(0x10) != 1) ||
       (!writeRemainingLength(rl)) ||
       (stream.write(0) != 1) ||
       (stream.write(4) != 1) ||
       (stream.write("MQTT") != 4) ||
       (stream.write(4) != 1) 
     ) return false;

  if ( (stream.write(flags) != 1) ||
       (!writeWord(keepAlive)) ||
       (!writeStr(clientID))
     ) return false;

  if (willMessage.enabled) {
    if (!writeStr(willMessage.topic) || (stream.write(willMessage.data,willMessage.data_len) != willMessage.data_len) {
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

  pingIntervalRemaining = MQTT_DEFAULT_PING_INTERVAL;

  return true;
}

byte MQTTClient::recvCONNACK() {
  byte b;
  bool sessionPresent = false;
  byte returnCode = MQTT_CONNACK_SUCCESS;    // Default return code is success
  //Serial.println(stream->available());
  //Serial.println("recvCONNACK");
  if (isConnected) {
    return MQTT_ERROR_ALREADY_CONNECTED;
  }

  if (stream.read(b) == 1) {
    sessionPresent = (b == 1);
  } else {
    return MQTT_ERROR_INSUFFICIENT_DATA;
  }

  /*Serial.print("SessionPresent=");
  if (sessionPresent) {
    Serial.println("true");
  } else {
    Serial.println("false");
  }*/

  if ((b & 0xFE) > 0) {
    return MQTT_ERROR_PACKET_INVALID;
  }

  //Serial.println("packetinvalid");
  //Serial.println(stream->available());
  if (stream.read(b) == 1) {
    returnCode = b;
  } else {
    return MQTT_ERROR_INSUFFICIENT_DATA;
  }

  //Serial.println("Readreturncode");

  if (returnCode == MQTT_CONNACK_SUCCESS) {
    pingIntervalRemaining = MQTT_DEFAULT_PING_INTERVAL;
    pingCount = 0;
    isConnected = true;
    //Serial.println("Calling connected()");
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

/** @brief Disconnects the MQTT connection */
void MQTTClient::disconnect() {
  stream.write(0xE0);
  stream.write(0);
  isConnected = false;
}

/** @brief Called when the MQTT server terminates the connection
 *  @details Override disconnected() to perform additional actions when the the server terminates the MQTT connection */
void MQTTClient::disconnected() {
  isConnected = false;
  pingIntervalRemaining = 0;
}

bool MQTTClient::sendPINGREQ() {
  bool result;
  //Serial.println("sendPINGREQ");
  if (isConnected) {
    result = (stream.write(12 << 4) == 1);
    result &= (stream.write(0) == 1);
    return result;
  } else {
    return false;
  }
}

byte MQTTClient::recvPINGRESP() {
  //Serial.println("recvPINGRESP");
  return MQTT_ERROR_NONE;
}

byte MQTTClient::pingInterval() {
  //Serial.print("pingIntervalRemaining="); Serial.println(pingIntervalRemaining);
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
  queuedMessage_t* qm;
  bool result = 0;

  qm = PUBLISHQueue.pop();
  if (qm != null) {
    if (--qm->timeout == 0) {
      if (++qm->retries >= MQTT_PACKET_RETRIES) {
        result = false;
        delete pm->message;
        free(pm);
      } else {
        pm->timeout = MQTT_PACKET_TIMEOUT;
        pm->message.duplicate = true;
        outgoingPUBLISHQueue.push(qm);
        sendPUBLISH(qm);
      }
    }
  }

  qm = PUBRECQueue.pop();
  if (qm != null) {
    if (--qm->timeout == 0) {
      if (++qm->retries >= MQTT_PACKET_RETRIES) {
        result = false;
        delete pm->message;
        free(pm);
      } else {
        pm->timeout = MQTT_PACKET_TIMEOUT;
        incomingPUBLISHQueue.push(qm);
        sendPUBREC(qm)
      }
    }
  }
  
  qm = PUBRELQueue.pop();
  if (qm != null) {
    if (--qm->timeout == 0) {
      if (++qm->retries >= MQTT_PACKET_RETRIES) {
        result = false;
        delete pm->message;
        free(pm);
      } else {
        pm->timeout = MQTT_PACKET_TIMEOUT;
        PUBRELQueue.push(qm);
        sendPUBREL(qm)
      }
    }
  }

  return result;
}

byte MQTTClient::intervalTimer() {
  if (!queueInterval()) {
    return MQTT_ERROR_PACKET_QUEUE_TIMEOUT;
  } else {
    return pingInterval();
  }
}

bool MQTTClient::subscribe(const word packetid, const String filter&, const qos_t qos) {
  bool result;

  if (filter != NULL) {
    result = (stream.write(0x82) == 1);
    result &= writeRemainingLength(2 + 2 + 1 + strlen(filter));
    result &= writeWord(packetid);
    result &= writeStr(filter);
    result &= (stream.write(qos) == 1);
    return result;
  } else {
    return false;
  }
}

byte MQTTClient::recvSUBACK(long remainingLength) {
  byte rc;
  long rl;
  word packetid;

  //Serial.println("recvSUBACK");
  //Serial.print("remaininglength="); Serial.println(remainingLength);

  if (!isConnected) {
    return MQTT_ERROR_NOT_CONNECTED;
  }

  if (readWord(&packetid)) {
    //Serial.print("packetid="); Serial.println(packetid);
    rl = remainingLength-2;
    //Serial.print("remaininglength="); Serial.println(rl);
    while (rl-- > 0) {
      if (readByte(&rc)) {
        //Serial.print("subscribed "); Serial.print(packetid); Serial.print(" "); Serial.println(rc);
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

bool MQTTClient::unsubscribe(word packetid, char *filter) {
  bool result;

  if (filter != NULL) {
    result = writeByte(0xA2);
    result &= writeRemainingLength(2+2+strlen(filter));
    result &= writeWord(packetid);
    result &= writeStr(filter);
    return result;
  } else {
    return false;
  }
}

byte MQTTClient::recvUNSUBACK() {
  word packetid;
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

/** @brief   Publish a message to the server.
 *  @remark  In this version of the function, data is provided as a String object.
 *  @warning The data sent does not include the trailing null character */
bool MQTTClient::publish(String topic, uint8_t *data, size_t data_len, qos_t qos, bool retain, bool duplicate) {
  MQTTMessage msg(topic,data,data_len,qos,retain,duplicate);
  sendPUBLISH(msg);
}

/** @brief   Publish a message to the server.
 *  @remark  In this version of the function, data is provided as a String object.
 *  @warning If the data sent might include null characters, use the alternate version of this function.
 *  @warning The data sent by this function does not include a trailing null character */
bool MQTTClient::publish(String topic, String data, qos_t qos, bool retain, bool duplicate) {
  MQTTMessage msg(topic,(uint8_t *)data.c_str(),data.length(),qos,retain,duplicate);
  sendPUBLISH(msg);
}

/** @brief    Sends an MQTT publish packet. Do not call directly. */  
bool MQTTClient::sendPUBLISH(MQTTMessage msg) {
  byte flags = 0;
  word packetid;
  long remainingLength;
  bool result;

  if (msg != NULL) {
    if ((msg->topic != NULL) && (msg->topic.length()>0) && (msg->qos<3) && (isConnected)) {

      //Serial.print("sendPUBLISH topic="); Serial.print(msg->topic); Serial.print(" data="); Serial.print(msg); Serial.print(" qos="); Serial.println(msg->qos);
      flags |= (msg->qos << 1);
      if (msg->duplicate) {
        flags |= 8;
      }
      if (msg->retain) {
        flags |= 1;
      }

      remainingLength = 2 + msg->topic.length() + msg->data_len;
      if (msg->qos>0) {
        remainingLength += 2;
      }

      packetid = nextPacketID++;
      if (nextPacketID >= MQTT_MAX_PACKETID) {
        nextPacketID = MQTT_MIN_PACKETID;
      }

      result = (
        writeByte(0x30 | flags) &&
        writeRemainingLength(remainingLength) &&
        writeStr(msg->topic)
      );

      if (result && (msg->qos > 0)) {
        result = writeWord(packetid);
      }

      if (result && (msg->data != NULL)) {
        result = writeData(msg->data,msg->data_len);
      }

      if (result && (qos > 0)) {
        queuedMessage* qm = malloc(sizeof(queuedMessage_t));
        qm->packetid = packetid;
        qm->timeout = MQTT_PACKET_TIMEOUT;
        qm->retries = 0;
        qm->message = msg;
        outgoingPUBLISHQueue.push(qm);
      }

      return result;

    } else return false;
  } else return false;
}

byte MQTTClient::recvPUBLISH(byte flags, long remainingLength) {
  char topic[MQTT_MAX_TOPIC_LEN+1];
  char data[MQTT_MAX_DATA_LEN+1];
  byte qos;
  bool retain;
  bool duplicate;
  word packetid=0;
  long rl;
  byte i;

  for (i=0;i<MQTT_MAX_TOPIC_LEN+1;i++) {
    topic[i] = 0;
  }

  for (i=0;i<MQTT_MAX_DATA_LEN+1;i++) {
    data[i] = 0;
  }

  duplicate = (flags & 8) > 0;
  retain = (flags & 1) > 0;
  qos = (flags & 6) >> 1;

  /*Serial.print("recvPUBLISH ");
  Serial.print("flags=");
  if (duplicate) Serial.print("duplicate,");
  if (retain) Serial.print("retain,");
  if (qos==0) Serial.println("QOS0");
  if (qos==1) Serial.println("QOS1");
  if (qos==2) Serial.println("QOS2");*/

  //Serial.print(" remainingLength="); Serial.println(remainingLength);

  if (!isConnected) {
    return MQTT_ERROR_NOT_CONNECTED;
  }

  if (!readStr(topic,MQTT_MAX_TOPIC_LEN)) {
    return MQTT_ERROR_VARHEADER_INVALID;
  }

  rl = remainingLength - strlen(topic) - 2;
  //Serial.print("readtopic rl="); Serial.println(rl);
  //Serial.print("topic="); Serial.println(topic);

  if (qos>0) {
    if (readWord(&packetid)) {
      //Serial.print("packetid="); Serial.println(packetid);
      rl -= 2;
    } else {
      return MQTT_ERROR_VARHEADER_INVALID;
    }
  }

  //Serial.print("readmessage rl="); Serial.println(rl);
  if (rl > MQTT_MAX_DATA_LEN) {
    //Serial.println("Payload1");
    return MQTT_ERROR_PAYLOAD_INVALID;
  }


  //Serial.print("rl="); Serial.println(rl);
  word datalen;
  if (rl < 0)  {
    datalen = 0;
  } else {
    datalen = rl;
  }

  if (readData(data,datalen)) {
    //Serial.print("data="); Serial.println(data);
    if (qos<2) {
      receiveMessage(topic,data,retain,duplicate);
      if (qos==1) {
        sendPUBACK(packetid);
      }
    } else {
      if (addToIncomingQueue(packetid,qos,retain,duplicate,topic,data)) {
        sendPUBREC(packetid);
      } else {
        return MQTT_ERROR_PACKET_QUEUE_FULL;
      }
    }
    return MQTT_ERROR_NONE;
  } else {
    //Serial.println("Payload2");
    return MQTT_ERROR_PAYLOAD_INVALID;
  }
}

byte MQTTClient::recvPUBACK() {
  word packetid;

  if (readWord(&packetid)) {
    //Serial.print("recvPUBACK("); Serial.print(packetid); Serial.println(")");
    for (byte i=0;i<outgoingPUBLISHQueueCount;i++) {
      if (outgoingPUBLISHQueue[i].packetid == packetid) {
        deleteFromOutgoingQueue(i);
        return MQTT_ERROR_NONE;
      }
    }
    return MQTT_ERROR_PACKETID_NOT_FOUND;
  } else {
   return MQTT_ERROR_PAYLOAD_INVALID;
  }
}

bool MQTTClient::sendPUBACK(word packetid) {
  bool result;
  if (isConnected) {
    //Serial.print("sendPUBACK("); Serial.print(packetid); Serial.println(")");
    result = writeByte(0x40);
    result &= writeByte(0x02);
    result &= writeWord(packetid);
    return result;
  } else {
    return false;
  }
}

byte MQTTClient::recvPUBREC() {
  word packetid;

  if (readWord(&packetid)) {
    //Serial.print("recvPUBREC("); Serial.print(packetid); Serial.println(")");
    for (byte i=0;i<outgoingPUBLISHQueueCount;i++) {
      if (outgoingPUBLISHQueue[i].packetid == packetid) {
        deleteFromOutgoingQueue(i);
        if (sendPUBREL(packetid)) {
          return MQTT_ERROR_NONE;
        } else {
          return MQTT_ERROR_SEND_PUBREL_FAILED;
        }
      }
    }
    return MQTT_ERROR_PACKETID_NOT_FOUND;
  } else {
   return MQTT_ERROR_PAYLOAD_INVALID;
  }
}

bool MQTTClient::sendPUBREC(word packetid) {
  bool result;
  if (isConnected) {
    //Serial.print("sendPUBREC("); Serial.print(packetid); Serial.println(")");
    result = writeByte(0x50);
    result &= writeByte(0x02);
    result &= writeWord(packetid);
    return result;
  } else {
    return false;
  }
}

byte MQTTClient::recvPUBREL() {
  word packetid;

  if (readWord(&packetid)) {
    //Serial.print("recvPUBREL("); Serial.print(packetid); Serial.println(")");
    for (byte i=0;i<incomingPUBLISHQueueCount;i++) {
      if (incomingPUBLISHQueue[i].packetid == packetid) {
        receiveMessage(incomingPUBLISHQueue[i].topic,incomingPUBLISHQueue[i].data,incomingPUBLISHQueue[i].retain,incomingPUBLISHQueue[i].duplicate);
        deleteFromIncomingQueue(i);
        if (sendPUBCOMP(packetid)) {
          return MQTT_ERROR_NONE;
        } else {
          return MQTT_ERROR_SEND_PUBCOMP_FAILED;
        }
      }
    }
    return MQTT_ERROR_PACKETID_NOT_FOUND;
  } else {
   return MQTT_ERROR_PAYLOAD_INVALID;
  }
}

bool MQTTClient::sendPUBREL(word packetid) {
  bool result;
  if (isConnected) {
    //Serial.print("sendPUBREL("); Serial.print(packetid); Serial.println(")");
    result = writeByte(0x62);
    result &= writeByte(0x02);
    result &= writeWord(packetid);
    if (result) {
      addToPUBRELQueue(packetid);
    }
    return result;
  } else {
    return false;
  }
}

byte MQTTClient::recvPUBCOMP() {
  word packetid;

  if (readWord(&packetid)) {
    //Serial.print("recvPUBCOMP("); Serial.print(packetid); Serial.println(")");
    for (byte i=0;i<PUBRELQueueCount;i++) {
      if (PUBRELQueue[i].packetid == packetid) {
        deleteFromPUBRELQueue(i);
        return MQTT_ERROR_NONE;
      }
    }
    return MQTT_ERROR_PACKETID_NOT_FOUND;
  } else {
   return MQTT_ERROR_PAYLOAD_INVALID;
  }
}

bool MQTTClient::sendPUBCOMP(word packetid) {
  bool result;
  if (isConnected) {
    //Serial.print("sendPUBCOMP("); Serial.print(packetid); Serial.println(")");
    result = writeByte(0x70);
    result &= writeByte(0x02);
    result &= writeWord(packetid);
    return result;
  } else {
    return false;
  }
}

byte MQTTClient::dataAvailable() {
  byte b;
  byte flags;
  byte packetType;
  long remainingLength=0; // remaining length

  if (readByte(&b)) {
    flags = b & 0x0F;
    packetType = b >> 4;
  } else {
    return MQTT_ERROR_INSUFFICIENT_DATA;
  }
  //Serial.print("rlavailable="); Serial.println(stream->available());
  if (!readRemainingLength(&remainingLength)) {
    return MQTT_ERROR_INSUFFICIENT_DATA;
  }
  //Serial.print("remainingLength="); Serial.println(remainingLength);

  pingIntervalRemaining = MQTT_DEFAULT_PING_INTERVAL;
  pingCount = 0;

  switch (packetType) {
    case ptCONNACK   : return recvCONNACK(); break;
    case ptSUBACK    : return recvSUBACK(remainingLength); break;
    case ptUNSUBACK  : return recvUNSUBACK(); break;
    case ptPUBLISH   : return recvPUBLISH(flags,remainingLength); break;
    case ptPINGRESP  : return recvPINGRESP(); break;
    case ptPUBACK    : return recvPUBACK(); break;
    case ptPUBREC    : return recvPUBREC(); break;
    case ptPUBREL    : return recvPUBREL(); break;
    case ptPUBCOMP   : return recvPUBCOMP(); break;
    default: return MQTT_ERROR_UNHANDLED_PACKETTYPE;
  }

  return MQTT_ERROR_NONE;
}
