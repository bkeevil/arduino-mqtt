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

/* MQTT Client */

bool MQTTClient::readByte(byte* b) {
  if (stream->available() == 0) {
    delay(100);
  }
  if (stream->available() > 0) {
    short s = stream->read();
    if (s == -1) {
      return false;
    } else {
      *b = s;
      return true;
    }
  } else {
    return false;
  }
}

bool MQTTClient::writeByte(const byte b) {
  if (stream->write(b) == 0) {
    stream->flush();
    return (stream->write(b) == 1);
  } else {
    return true;
  }
}

bool MQTTClient::readRemainingLength(long* value) {
  long multiplier = 1;
  byte encodedByte;

  //Serial.print("value="); Serial.println(*value);
  *value = 0;
  do {
    if (readByte(&encodedByte)) {
      //Serial.print("encodedByte="); Serial.println(encodedByte);
      *value += (encodedByte & 127) * multiplier;
      //Serial.print("value="); Serial.println(*value);
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

bool MQTTClient::writeRemainingLength(const long value) {
  byte encodedByte;
  long lvalue;

  lvalue = value;
  do {
    encodedByte = lvalue % 128;
    lvalue = lvalue / 128;
    if (lvalue > 0) {
      encodedByte |= 128;
    }
    if (!writeByte(encodedByte)) {
      return false;
    }
  } while (lvalue > 0);
  return true;
}

bool MQTTClient::readWord(word *value) {
  byte b;
  if (readByte(&b)) {
    *value = b << 8;
    if (readByte(&b)) {
      *value |= b;
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool MQTTClient::writeWord(const word value) {
  byte b = value >> 8;
  if (writeByte(b)) {
    b = value & 0xFF;
    if (writeByte(b)) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool MQTTClient::readData(char* data, const word len) {
  byte* ptr;
  word remaining = len;
  ptr = (byte*)data;

  //Serial.print("remaining="); Serial.println(remaining);
  while (remaining>0) {
    if (readByte(ptr)) {
      ptr++;
      remaining--;
    } else {
      return false;
    }
  }
  return true;
}

bool MQTTClient::writeData(char* data, const word len) {
  char *ptr;
  word rl = len;

  ptr = data;
  while (rl > 0) {
    if (!writeByte(byte(*ptr))) {
      return false;
    }
    ptr++;
    rl--;
  }
  return true;
}

bool MQTTClient::writeData(String data) {
  char c;
  int i = 0;
  int l = data.length();

  while (l > 0) {
    c = data.charAt(i);
    if (!writeByte(byte(c))) {
      return false;
    }
    i++;
    l--;
  }
  return true;
}

bool MQTTClient::readStr(char *str, const word len) {
  word l;

  if (readWord(&l)) {
    if (l < len) {
      return readData(str,l);
    } else {
      readData(str,len);
      return false;
    }
  } else {
    return false;
  }
}

bool MQTTClient::writeStr(char *str) {
  char *ptr;
  word len;

  len = strlen(str);
  if (writeWord(len)) {
    ptr = str;
    while (len > 0) {
      if (!writeByte(byte(*ptr))) {
        return false;
      }
      ptr++;
      len--;
    }
    return true;
  } else {
    return false;
  }
}

bool MQTTClient::writeStr(String str) {
  unsigned int i;
  char chr;
  word len;

  len = str.length();
  if (writeWord(len)) {
    i = 0;
    while (len > 0) {
      chr = str.charAt(i);
      if (!writeByte(byte(chr))) {
        return false;
      }
      i++;
      len--;
    }
    return true;
  } else {
    return false;
  }
}

void MQTTClient::reset() {
  pingIntervalRemaining = 0;
  pingCount = 0;
  incomingPUBLISHQueueCount = 0;
  outgoingPUBLISHQueueCount = 0;
  PUBRELQueueCount = 0;
  isConnected = false;
}

bool MQTTClient::connect(String clientID, String username, String password, bool cleanSession, word keepAlive)
{
  byte flags;
  word rl;      // Remaining Length

  reset();

  rl = 10 + 2 + clientID.length();

  if (username != NULL) {
    flags = 128;
    rl += username.length() + 2;
  } else {
    flags = 0;
  }

  if (password != NULL) {
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

  if ((!writeByte(0x10)) ||
     (!writeRemainingLength(rl)) ||
     (!writeByte(0)) ||
     (!writeByte(4)) ||
     (!writeByte('M')) ||
     (!writeByte('Q')) ||
     (!writeByte('T')) ||
     (!writeByte('T')) ||
     (!writeByte(4)))
       { return false; }

  if ((!writeByte(flags)) ||
     (!writeWord(keepAlive)) ||
     (!writeStr(clientID)))
       { return false; }

  if (willMessage.enabled) {
    if (!writeStr(willMessage.topic) || !writeStr(willMessage.data)) {
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

  if (readByte(&b)) {
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
  if (readByte(&b)) {
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

bool MQTTClient::disconnect() {
  if (writeByte(0xE0) && writeByte(0)) {
    isConnected = false;
    return true;
  } else {
    return false;
  }
}

void MQTTClient::disconnected() {
  isConnected = false;
  pingIntervalRemaining = 0;
}

bool MQTTClient::sendPINGREQ() {
  bool result;
  //Serial.println("sendPINGREQ");
  if (isConnected) {
    result = writeByte(12 << 4);
    result &= writeByte(0);
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

bool MQTTClient::addToOutgoingQueue(word packetid, qos_t qos, bool retain, bool duplicate, char* topic, char* data) {
  if (outgoingPUBLISHQueueCount == MQTT_PACKET_QUEUE_SIZE) {
    //Serial.println("Error: outgoingPUBLISHQueue overflow");
    return false;
  }
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].packetid = packetid;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].timeout = MQTT_PACKET_TIMEOUT;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].retries = 0;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].qos = qos;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].retain = retain;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].duplicate = duplicate;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].topic = String(topic);
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].data = String(data);
  //strlcpy(outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].topic,topic,MQTT_MAX_TOPIC_LEN);
  //strlcpy(outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].data,data,MQTT_MAX_DATA_LEN);
  outgoingPUBLISHQueueCount++;
  return true;
}

bool MQTTClient::addToOutgoingQueue(word packetid, qos_t qos, bool retain, bool duplicate, String topic, String data) {
  if (outgoingPUBLISHQueueCount == MQTT_PACKET_QUEUE_SIZE) {
    //Serial.println("Error: outgoingPUBLISHQueue overflow");
    return false;
  }
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].packetid = packetid;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].timeout = MQTT_PACKET_TIMEOUT;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].retries = 0;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].qos = qos;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].retain = retain;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].duplicate = duplicate;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].topic = topic;
  outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].data = data;
  //strlcpy(outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].topic,topic.c_str(),MQTT_MAX_TOPIC_LEN);
  //strlcpy(outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].data,data.c_str(),MQTT_MAX_DATA_LEN);
  outgoingPUBLISHQueueCount++;
  return true;
}

bool MQTTClient::addToIncomingQueue(word packetid, qos_t qos, bool retain, bool duplicate, char* topic, char* data) {
  if (incomingPUBLISHQueueCount == MQTT_PACKET_QUEUE_SIZE) {
    //Serial.println("Error: incomingPUBLISHQueue overflow");
    return false;
  }
  incomingPUBLISHQueue[incomingPUBLISHQueueCount].packetid = packetid;
  incomingPUBLISHQueue[incomingPUBLISHQueueCount].timeout = MQTT_PACKET_TIMEOUT;
  incomingPUBLISHQueue[incomingPUBLISHQueueCount].retries = 0;
  incomingPUBLISHQueue[incomingPUBLISHQueueCount].qos = qos;
  incomingPUBLISHQueue[incomingPUBLISHQueueCount].retain = retain;
  incomingPUBLISHQueue[incomingPUBLISHQueueCount].duplicate = duplicate;
  incomingPUBLISHQueue[incomingPUBLISHQueueCount].topic = String(topic);
  incomingPUBLISHQueue[incomingPUBLISHQueueCount].data = String(data);
  //strlcpy(incomingPUBLISHQueue[incomingPUBLISHQueueCount].topic,topic,MQTT_MAX_TOPIC_LEN);
  //strlcpy(incomingPUBLISHQueue[incomingPUBLISHQueueCount].data,data,MQTT_MAX_DATA_LEN);
  incomingPUBLISHQueueCount++;
  return true;
}

bool MQTTClient::addToPUBRELQueue(word packetid) {
  if (PUBRELQueueCount == MQTT_PACKET_QUEUE_SIZE) {
    //Serial.println("Error: PUBRELQueue overflow");
    return false;
  }
  PUBRELQueue[PUBRELQueueCount].packetid = packetid;
  PUBRELQueue[PUBRELQueueCount].timeout = MQTT_PACKET_TIMEOUT;
  PUBRELQueue[PUBRELQueueCount].retries = 0;
  PUBRELQueueCount++;
  return true;
}

void MQTTClient::deleteFromOutgoingQueue(byte i) {
  for (byte j=i;j<outgoingPUBLISHQueueCount - 1;j++) {
    outgoingPUBLISHQueue[j].packetid = outgoingPUBLISHQueue[j+1].packetid;
    outgoingPUBLISHQueue[j].timeout = outgoingPUBLISHQueue[j+1].timeout;
    outgoingPUBLISHQueue[j].retries = outgoingPUBLISHQueue[j+1].retries;
    outgoingPUBLISHQueue[j].qos = outgoingPUBLISHQueue[j+1].qos;
    outgoingPUBLISHQueue[j].retain = outgoingPUBLISHQueue[j+1].retain;
    outgoingPUBLISHQueue[j].duplicate = outgoingPUBLISHQueue[j+1].duplicate;
    outgoingPUBLISHQueue[j].topic = outgoingPUBLISHQueue[j+1].topic;
    outgoingPUBLISHQueue[j].data = outgoingPUBLISHQueue[j+1].data;
    //strlcpy(outgoingPUBLISHQueue[j].topic,outgoingPUBLISHQueue[j+1].topic,MQTT_MAX_TOPIC_LEN);
    //strlcpy(outgoingPUBLISHQueue[j].data,outgoingPUBLISHQueue[j+1].topic,MQTT_MAX_DATA_LEN);
  }
  outgoingPUBLISHQueueCount--;
}

void MQTTClient::deleteFromIncomingQueue(byte i) {
  for (byte j=i;j<incomingPUBLISHQueueCount - 1;j++) {
    incomingPUBLISHQueue[j].packetid = incomingPUBLISHQueue[j+1].packetid;
    incomingPUBLISHQueue[j].timeout = incomingPUBLISHQueue[j+1].timeout;
    incomingPUBLISHQueue[j].retries = incomingPUBLISHQueue[j+1].retries;
    incomingPUBLISHQueue[j].qos = incomingPUBLISHQueue[j+1].qos;
    incomingPUBLISHQueue[j].retain = incomingPUBLISHQueue[j+1].retain;
    incomingPUBLISHQueue[j].duplicate = incomingPUBLISHQueue[j+1].duplicate;
    incomingPUBLISHQueue[j].topic = incomingPUBLISHQueue[j+1].topic;
    incomingPUBLISHQueue[j].data = incomingPUBLISHQueue[j+1].data;
    //strlcpy(incomingPUBLISHQueue[j].topic,incomingPUBLISHQueue[j+1].topic,MQTT_MAX_TOPIC_LEN);
    //strlcpy(incomingPUBLISHQueue[j].data,incomingPUBLISHQueue[j+1].topic,MQTT_MAX_DATA_LEN);
  }
  incomingPUBLISHQueueCount--;
}

void MQTTClient::deleteFromPUBRELQueue(byte i) {
  for (byte j=i;j<PUBRELQueueCount - 1;j++) {
    PUBRELQueue[j].packetid = PUBRELQueue[j+1].packetid;
    PUBRELQueue[j].timeout = PUBRELQueue[j+1].timeout;
    PUBRELQueue[j].retries = PUBRELQueue[j+1].retries;
  }
  PUBRELQueueCount--;
}

bool MQTTClient::queueInterval() {
  int i;
  bool result = true;

  // Outgoing PUBLISH
  if (outgoingPUBLISHQueueCount > 0) {
    //Serial.println("Outgoingqueuecount");
    for (i=outgoingPUBLISHQueueCount-1;i>=0;i--) {
      //Serial.println(i);
      if (--outgoingPUBLISHQueue[i].timeout == 0) {
        outgoingPUBLISHQueue[i].retries++;
        if (outgoingPUBLISHQueue[i].retries >= MQTT_PACKET_RETRIES) {
          deleteFromOutgoingQueue(i);
          result = false;
        } else {
          //Serial.println('publish');
          publish(outgoingPUBLISHQueue[i].topic,outgoingPUBLISHQueue[i].data,outgoingPUBLISHQueue[i].qos,outgoingPUBLISHQueue[i].retain,true);
          outgoingPUBLISHQueue[i].timeout = MQTT_PACKET_TIMEOUT;
        }
      }
    }
  }

  // Incoming PUBLISH
  if (incomingPUBLISHQueueCount > 0) {
    //Serial.println("Incomingqueuecount");
    for (i=incomingPUBLISHQueueCount-1;i>=0;i--) {
      if (--incomingPUBLISHQueue[i].timeout == 0) {
        incomingPUBLISHQueue[i].retries++;
        if (incomingPUBLISHQueue[i].retries >= MQTT_PACKET_RETRIES) {
          deleteFromIncomingQueue(i);
          result = false;
        } else {
          sendPUBREC(incomingPUBLISHQueue[i].packetid);
          incomingPUBLISHQueue[i].timeout = MQTT_PACKET_TIMEOUT;
        }
      }
    }
  }

  // PUBRELQueue
  if (PUBRELQueueCount > 0) {
    //Serial.println("PUBRELQueueCount");
    for (i=PUBRELQueueCount-1;i>=0;i--) {
      if (--PUBRELQueue[i].timeout == 0) {
        PUBRELQueue[i].retries++;
        if (PUBRELQueue[i].retries >= MQTT_PACKET_RETRIES) {
          deleteFromPUBRELQueue(i);
          result = false;
        } else {
          sendPUBREL(PUBRELQueue[i].packetid);
          PUBRELQueue[i].timeout = MQTT_PACKET_TIMEOUT;
        }
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

bool MQTTClient::subscribe(word packetid, char *filter, qos_t qos) {
  bool result;

  if (filter != NULL) {
    result = writeByte(0x82);
    result &= writeRemainingLength(2 + 2 + 1 + strlen(filter));
    result &= writeWord(packetid);
    result &= writeStr(filter);
    result &= writeByte(qos);
    return result;
  } else {
    return false;
  }
}

bool MQTTClient::subscribe(word packetid, String filter, qos_t qos) {
  bool result;

  if (filter != NULL) {
    result = writeByte(0x82);
    result &= writeRemainingLength(2 + 2 + 1 + filter.length());
    result &= writeWord(packetid);
    result &= writeStr(filter);
    result &= writeByte(qos);
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

bool MQTTClient::publish(char *topic, char *data, qos_t qos, bool retain, bool duplicate) {
  byte flags = 0;
  word packetid;
  long remainingLength;
  bool result;


  if ((topic != NULL) && (strlen(topic)>0) && (qos<=qtMAX_VALUE) && (isConnected)) {

    //Serial.print("sendPUBLISH topic="); Serial.print(topic); Serial.print(" data="); Serial.print(data); Serial.print(" qos="); Serial.println(qos);
    flags |= (qos << 1);
    if (duplicate) {
      flags |= 8;
    }
    if (retain) {
      flags |= 1;
    }

    remainingLength = 2 + strlen(topic) + strlen(data);
    if (qos>0) {
      remainingLength += 2;
    }

    packetid = nextPacketID++;
    if (nextPacketID >= MQTT_MAX_PACKETID) {
      nextPacketID = MQTT_MIN_PACKETID;
    }

    result = (
      writeByte(0x30 | flags) &&
      writeRemainingLength(remainingLength) &&
      writeStr(topic)
    );

    if (result && (qos > 0)) {
      result = writeWord(packetid);
    }

    if (result && (data != NULL)) {
      result = writeData(data,strlen(data));
    }

    if (result && (qos > 0)) {
      addToOutgoingQueue(packetid,qos,retain,duplicate,topic,data);
    }

    return result;

  } else return false;
}

bool MQTTClient::publish(String topic, String data, qos_t qos, bool retain, bool duplicate) {
  byte flags = 0;
  word packetid;
  long remainingLength;
  bool result;


  if ((topic != NULL) && (topic.length()>0) && (qos<3) && (isConnected)) {

    //Serial.print("sendPUBLISH topic="); Serial.print(topic); Serial.print(" data="); Serial.print(data); Serial.print(" qos="); Serial.println(qos);
    flags |= (qos << 1);
    if (duplicate) {
      flags |= 8;
    }
    if (retain) {
      flags |= 1;
    }

    remainingLength = 2 + topic.length() + data.length();
    if (qos>0) {
      remainingLength += 2;
    }

    packetid = nextPacketID++;
    if (nextPacketID >= MQTT_MAX_PACKETID) {
      nextPacketID = MQTT_MIN_PACKETID;
    }

    result = (
      writeByte(0x30 | flags) &&
      writeRemainingLength(remainingLength) &&
      writeStr(topic)
    );

    if (result && (qos > 0)) {
      result = writeWord(packetid);
    }

    if (result && (data != NULL)) {
      result = writeData(data);
    }

    if (result && (qos > 0)) {
      addToOutgoingQueue(packetid,qos,retain,duplicate,topic,data);
    }

    return result;

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
