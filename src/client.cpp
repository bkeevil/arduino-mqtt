#include "client.h"

using namespace mqtt;

void Client::reset() {
  pingIntervalRemaining = 0;
  pingCount = 0;
  PUBRECQueue.clear();
  PUBLISHQueue.clear();
  PUBRELQueue.clear();
  isConnected = false;
}

bool Client::connect(const String& clientID, const String& username, const String& password, const bool cleanSession, const word keepAlive) {
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

  pingIntervalRemaining = clientConfiguration.pingInterval;

  return true;
}

byte Client::recvCONNACK() {
  byte b;
  bool sessionPresent = false;
  byte returnCode = ConnackResult::SUCCESS;    // Default return code is success

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

  if (returnCode == ConnackResult::SUCCESS) {
    pingIntervalRemaining = clientConfiguration.pingInterval;
    pingCount = 0;
    connected();
    if (!sessionPresent) {
      initSession();
    }
    return MQTT_ERROR_NONE;
  } else {
    switch (returnCode) {
      case ConnackResult::UNNACCEPTABLE_PROTOCOL : return MQTT_ERROR_UNACCEPTABLE_PROTOCOL;
      case ConnackResult::CLIENTID_REJECTED      : return MQTT_ERROR_CLIENTID_REJECTED;
      case ConnackResult::SERVER_UNAVAILABLE     : return MQTT_ERROR_SERVER_UNAVAILABLE;
      case ConnackResult::BAD_USERNAME_PASSWORD  : return MQTT_ERROR_BAD_USERNAME_PASSWORD;
      case ConnackResult::NOT_AUTHORIZED         : return MQTT_ERROR_NOT_AUTHORIZED;
    }
  }
  return MQTT_ERROR_UNKNOWN;
}

void Client::connected() {
  isConnected = true;
  if (stream.available() > 0) {
    dataAvailable();
  }
  if (connectMessage.enabled) {
    publish(connectMessage);
  }
}

void Client::disconnect() {
  if (disconnectMessage.enabled) {
    disconnectMessage.qos = qtAT_MOST_ONCE;
    publish(disconnectMessage);
  }
  stream.write((byte)0xE0);
  stream.write((byte)0);
  isConnected = false;
}

void Client::disconnected() {
  #ifdef DEBUG
  Serial.println("Server terminated the MQTT connection");
  #endif
  isConnected = false;
  pingIntervalRemaining = 0;
}

bool Client::sendPINGREQ() {
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

byte Client::pingInterval() {
  if (pingIntervalRemaining == 1) {
    if (pingCount >= 2) {
      pingCount = 0;
      pingIntervalRemaining = 0;
      return MQTT_ERROR_NO_PING_RESPONSE;
    }
    sendPINGREQ();
    if (pingCount == 0) {
      pingIntervalRemaining = clientConfiguration.pingInterval;
    } else {
      pingIntervalRemaining = clientConfiguration.pingRetryInterval;
    }
    pingCount++;
  } else {
    if (pingIntervalRemaining > 1) {
      pingIntervalRemaining--;
    }
  }
  return MQTT_ERROR_NONE;
}

bool Client::queueInterval() {
  bool result;

  result = PUBLISHQueue.interval();
  result &= PUBRECQueue.interval();
  result &= PUBRELQueue.interval();

  return result;
}

byte Client::intervalTimer() {
  if (!queueInterval()) {
    return MQTT_ERROR_PACKET_QUEUE_TIMEOUT;
  } else {
    return pingInterval();
  }
}

word Client::getNextPacketID() {
  if (nextPacketID == 65535) {
    nextPacketID = 0;
  } else
    nextPacketID++;
  
  return nextPacketID;
}

bool Client::subscribe(MQTTSubscriptionList& subs) {
  if (isConnected) sendSUBSCRIBE(subs);
  subscriptions_.push(subs);
}

bool Client::subscribe(const String& filter, const qos_t qos, const MQTTMessageHandlerFunc handler) {
  MQTTSubscriptionList subs;
  MQTTSubscription* sub = new MQTTSubscription(filter,qos,handler);
  if (sub->filter.valid) {
    subs.push(sub);
    subscribe(subs);
  }
}

/** @brief  Creates an MQTT Subscribe packet from a list of subscriptions and sends it to the server */
bool Client::sendSUBSCRIBE(const MQTTSubscriptionList& subscriptions) {
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
  node->timeout = clientConfiguration.packetTimeout;
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

byte Client::recvSUBACK(const long remainingLength) {
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

bool Client::unsubscribe(const String& filter) {
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

byte Client::recvUNSUBACK() {
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

bool Client::publish(const String& topic, byte* data, const size_t data_len, const qos_t qos, const bool retain) {
  MQTTMessage* msg = new MQTTMessage(topic);
  msg->qos = qos;
  msg->retain = retain;
  msg->data = data;
  msg->data_len = data_len;
  return sendPUBLISH(msg);
}

bool Client::publish(const String& topic, const String& data, const qos_t qos, const bool retain) {
  MQTTMessage* msg = new MQTTMessage(topic);
  msg->qos = qos;
  msg->retain = retain;
  msg->data = (byte*)data.c_str();
  msg->data_len = data.length();
  return sendPUBLISH(msg);
}

bool Client::sendPUBLISH(const char* topic, const byte* data, const size_t data_len, const QoS qos, const bool retain, const bool duplicate) {

  if (topic != nullptr) {
    #ifdef DEBUG
    Serial.println("sending PUBLISH");
    #endif
    
    if ((strlen(topic) > 0) && (qos < QoS::MAX_VALUE) && (isConnected)) {

      byte flags = (qos << 1);
      if (duplicate) {
        flags |= 8;
      }
      if (retain) {
        flags |= 1;
      }

      long remainingLength = 2 + strlen(topic) + data_len;
      if (qos>0) {
        remainingLength += 2;
      }

      word packetid = getNextPacketID();

      #ifdef DEBUG
      Serial.print("flags="); Serial.println(flags);
      Serial.print("qos="); Serial.println(qos);
      Serial.print("packetid="); Serial.println(packetid);
      Serial.print("topic="); Serial.println(mt);
      Serial.print("remainingLength="); Serial.println(remainingLength);
      Serial.print("data_len="); Serial.println(data_len);
      #endif

      bool result = (
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
        qm->timeout = clientConfiguration.packetTimeout;
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

byte Client::recvPUBLISH(const byte flags, const long remainingLength) {
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
      qm->timeout = clientConfiguration.packetTimeout;
      qm->message = msg;
      PUBRECQueue.push(qm);
      sendPUBREC(packetid);
    }
    return MQTT_ERROR_NONE;
  } else {
    return MQTT_ERROR_PAYLOAD_INVALID;
  }
}

byte Client::recvPUBACK() {
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

bool Client::sendPUBACK(const word packetid) {
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

byte Client::recvPUBREC() {
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

bool Client::sendPUBREC(const word packetid) {
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

byte Client::recvPUBREL() {
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

bool Client::sendPUBREL(const word packetid) {
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
      qm->timeout  = clientConfiguration.packetTimeout;
      qm->retries  = 0;
      qm->message  = NULL;
      PUBRELQueue.push(qm);
    }
    return result;
  } else {
    return false;
  }
}

byte Client::recvPUBCOMP() {
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

bool Client::sendPUBCOMP(const word packetid) {
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

byte Client::dataAvailable() {
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
  
  pingIntervalRemaining = clientConfiguration.pingInterval;
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
