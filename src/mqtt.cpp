#include "mqtt.h"

/* MQTTMessage */

MQTTMessage::MQTTMessage(const MQTTMessage& m): topic(m.topic), qos(m.qos), duplicate(m.duplicate), retain(m.retain), data_len(m.data_len), data_size(m.data_len), data_pos(m.data_len) {
  data = (byte*) malloc(data_len);
  memcpy(data,m.data,data_len);
}

size_t MQTTMessage::printTo(Print& p) const {
  size_t pos;
  for (pos=0;pos<data_len;pos++) { 
    p.print(data[pos]);
  }
  return data_len;
}

void MQTTMessage::reserve(size_t size) {
  if (data_size == 0) {
    data = (byte*) malloc(size);
  } else {
    data = (byte*) realloc(data,size);
  }  
  data_size = size;
}

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

int MQTTMessage::read() {
  if (data_pos < data_len) {
    return data[data_pos++];
  } else {
    return -1;
  }
}

int MQTTMessage::peek() {
  if (data_pos < data_len) {
    return data[data_pos];
  } else {
    return -1;
  }
}

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
}

bool MQTTMessageQueue::interval() {
  bool result = true;
  queuedMessage_t* qm;

  qm = pop();
  if (qm != NULL) {
    if (--qm->timeout == 0) {
      if (++qm->retries >= MQTT_PACKET_RETRIES) {
        result = false;
        delete qm->message;
        free(qm);
      } else {
        qm->timeout = MQTT_PACKET_TIMEOUT;
        push(qm);
        resend(qm);
      }
    }
  }
  return result;
}

void MQTTMessageQueue::push(queuedMessage_t* qm) {
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

queuedMessage_t* MQTTMessageQueue::pop() {
  queuedMessage_t* ptr;
  if (first != NULL) {
    ptr = first;
    first = first->next;
    count--;
    return ptr;
  } else {
    return NULL;
  }
}

void MQTTPUBLISHQueue::resend(queuedMessage_t* qm) { 
  qm->message->duplicate = true; 
  client->sendPUBLISH(qm->message); 
}

void MQTTPUBRECQueue::resend(queuedMessage_t* qm) { 
  client->sendPUBREC(qm->packetid); 
}

void MQTTPUBRELQueue::resend(queuedMessage_t* qm) { 
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
  
  //Serial.print("Logging in as clientID: "); Serial.print(clientID);
  //Serial.print(" username: "); Serial.println(username);

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
    rl += willMessage.topic.length() + 2 + willMessage.data_len + 2;
  }

  if (cleanSession) {
    flags |= 2;
  }

  //Serial.print("rl="); Serial.println(rl);
  //Serial.print("keepAlive="); Serial.println(keepAlive);

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

  pingIntervalRemaining = MQTT_DEFAULT_PING_INTERVAL;

  return true;
}

byte MQTTClient::recvCONNACK() {
  byte b;
  bool sessionPresent = false;
  byte returnCode = MQTT_CONNACK_SUCCESS;    // Default return code is success

  Serial.println("recvCONNACK");

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
  isConnected = false;
  pingIntervalRemaining = 0;
}

bool MQTTClient::sendPINGREQ() {
  bool result;
  if (isConnected) {
    result = (stream.write((byte)12 << 4) == 1);
    result &= (stream.write((byte)0) == 1);
    return result;
  } else {
    return false;
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

bool MQTTClient::subscribe(const word packetid, const String& filter, const qos_t qos) {
  bool result;

  if (filter != NULL) {
    result = (stream.write((byte)0x82) == 1);
    result &= writeRemainingLength(2 + 2 + 1 + filter.length());
    result &= writeWord(packetid);
    result &= writeStr(filter);
    result &= (stream.write(qos) == 1);
    return result;
  } else {
    return false;
  }
}

byte MQTTClient::recvSUBACK(const long remainingLength) {
  int i;
  byte rc;
  long rl;
  word packetid;

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

bool MQTTClient::publish(const String& topic, byte* data, const size_t data_len, const qos_t qos, const bool retain) {
  MQTTMessage* msg = new MQTTMessage();
  msg->topic = topic;
  msg->qos = qos;
  msg->retain = retain;
  msg->data = data;
  msg->data_len = data_len;
  return sendPUBLISH(msg);
}

bool MQTTClient::publish(const String& topic, const String& data, const qos_t qos, const bool retain) {
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
    if ((msg->topic != NULL) && (msg->topic.length()>0) && (msg->qos<3) && (isConnected)) {

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
        (stream.write(0x30 | flags) == 1) &&
        writeRemainingLength(remainingLength) &&
        writeStr(msg->topic)
      );

      if (result && (msg->qos > 0)) {
        result = writeWord(packetid);
      }

      if (result && (msg->data != NULL)) {
        result = (stream.write(msg->data,msg->data_len) == msg->data_len);
      }

      if (result && (msg->qos > 0)) {
        queuedMessage_t* qm = new queuedMessage_t;
        qm->packetid = packetid;
        qm->timeout = MQTT_PACKET_TIMEOUT;
        qm->retries = 0;
        qm->message = msg;
        PUBLISHQueue.push(qm);
      }

      return result;

    } else return false;
  } else return false;
}

byte MQTTClient::recvPUBLISH(const byte flags, const long remainingLength) {
  MQTTMessage* msg;
  queuedMessage_t* qm;
  word packetid=0;
  long rl;
  int i;
  
  msg = new MQTTMessage();

  msg->duplicate = (flags & 8) > 0;
  msg->retain = (flags & 1) > 0;
  msg->qos = (qos_t)((flags & 6) >> 1);

  if (!isConnected) return MQTT_ERROR_NOT_CONNECTED;

  if (!readStr(msg->topic)) return MQTT_ERROR_VARHEADER_INVALID;

  rl = remainingLength - msg->topic.length() - 2;
  
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

  if (readWord(&packetid)) {
    iterations == PUBLISHQueue.getCount();
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

  if (readWord(&packetid)) {
    iterations == PUBLISHQueue.getCount();
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

  if (readWord(&packetid)) {
    iterations == PUBRECQueue.getCount();
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
    case ptPINGRESP  : return MQTT_ERROR_NONE; break;
    case ptPUBACK    : return recvPUBACK(); break;
    case ptPUBREC    : return recvPUBREC(); break;
    case ptPUBREL    : return recvPUBREL(); break;
    case ptPUBCOMP   : return recvPUBCOMP(); break;
    default: return MQTT_ERROR_UNHANDLED_PACKETTYPE;
  }

  return MQTT_ERROR_NONE;
}
