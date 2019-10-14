#include "connection.h"

using namespace mqtt;

mqtt::Connection::Connection(Client& client): Network(client) {
  // Init event function pointers to nullptr
  for (int i;i=0;i<Event::MAX) {
    events_[i] = nullptr;
  }
} 

/** @brief Reset is called internally on a protocol error to drop a connection and reset it to a disconnected state
 *         without sending a DISCONNECT packet to the server. */
void mqtt::Connection::reset() {
  if (state != ConnectionState::DISCONNECTED) {
    disconnected();
  }
  pingIntervalRemaining = 0;
  pingCount = 0;
  if (cleanSession) {
    pending.clear();
  }
  state = ConnectionState::DISCONNECTED;
  if (autoReconnect) {
    lastMillis = millis();
  }
}

bool mqtt::Connection::connect(const char* clientid, const char* username, const char* password) {
  int len;
  byte flags = 0;
  word remamingLength;
  
  reset();
  
  #ifdef DEBUG
  Serial.print("Logging in as clientID: "); Serial.print(clientid);
  Serial.print(" username: "); Serial.println(username);
  #endif
  
  remainingLength = 10 + 2 + strlen(clientid);
  
  if (username != nullptr) {
    len = strlen(username);
    if (len > 0) {
      flags = 128;
      remainingLength += len + 2;
    }
  }

  if (password != nullptr) {
    len = strlen(password);
    if (len > 0) {
      flags |= 64;
      remainingLength += len + 2;
    }
  }

  if (willMessage.retained()) {
    flags |= 32;
  }

  String& wmt = willMessage.topic.getText();

  flags |= (willMessage.qos<< 3);
  if (willMessage.enabled) {
    flags |= 4;
    remainingLength += wmt.length() + 2 + willMessage.data_len + 2;
  }

  if (cleanSession) {
    flags |= 2;
    //pending.clear();    // Called by reset() above
  }

  if ( (client.write((byte)0x10) != 1) ||
       (!writeRemainingLength(remainingLength)) ||
       (client.write((byte)0) != 1) ||
       (client.write((byte)4) != 1) ||
       (client.write("MQTT") != 4) ||
       (client.write((byte)4) != 1) 
     ) return false;

  if ( (client.write(flags) != 1) ||
       (!writeWord(keepAlive)) ||
       (!writeStr(clientID))
     ) return false;

  if (willMessage.enabled) {
    if (!writeStr(wmt) || !writeWord(willMessage.data_len) || (client.write(willMessage.data,willMessage.data_len) != willMessage.data_len)) {
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

  client.flush();

  ConnectionState::CONNECTING;
  lastMillis = millis();

  return true;
}

byte mqtt::Connection::recvCONNACK() {
  byte b;
  bool sessionPresent = false;
  byte returnCode = ConnackResult::SUCCESS;    // Default return code is success

  if (state == ConnectionState::CONNECTED) return ErrorCode::ALREADY_CONNECTED;

  int i = client.read();
  if (i > -1) {
    sessionPresent = (i == 1);
  } else {
    return ErrorCode::INSUFFICIENT_DATA;
  }

  if ((b & 0xFE) > 0) return ErrorCode::PACKET_INVALID;

  i = client.read();
  if (i > -1) {
    returnCode = i;
  } else {
    return ErrorCode::INSUFFICIENT_DATA;
  }

  if (returnCode == ConnackResult::SUCCESS) {
    pingIntervalRemaining = pingInterval;
    pingCount = 0;
    connected();
    if (!sessionPresent) {
      initSession();
    }
    return ErrorCode::NONE;
  } else {
    switch (returnCode) {
      case ConnackResult::UNACCEPTABLE_PROTOCOL  : return ErrorCode::UNACCEPTABLE_PROTOCOL;
      case ConnackResult::CLIENTID_REJECTED      : return ErrorCode::CLIENTID_REJECTED;
      case ConnackResult::SERVER_UNAVAILABLE     : return ErrorCode::SERVER_UNAVAILABLE;
      case ConnackResult::BAD_USERNAME_PASSWORD  : return ErrorCode::BAD_USERNAME_PASSWORD;
      case ConnackResult::NOT_AUTHORIZED         : return ErrorCode::NOT_AUTHORIZED;
    }
  }
  return ErrorCode::UNKNOWN;
}

void mqtt::Connection::connected() {
  state = ConnectionState::CONNECTED;
  if (client.available() > 0) {
    dataAvailable(); //???
  }
  if (connectMessage.enabled) {
    connectMessage.send(*this);
  }
}

void mqtt::Connection::disconnect() {
  if (disconnectMessage.enabled) {
    disconnectMessage.qos = QoS::AT_MOST_ONCE;
    disconnectMessage.send(*this);
  }
  client.flush();
  client.write((byte)0xE0);
  client.write((byte)0);
  client.flush();
  state = ConnectionState::DISCONNECTED;
}

void mqtt::Connection::disconnected() {
  #ifdef DEBUG
  Serial.println("Server terminated the MQTT connection");
  #endif
  state = ConnectionState::DISCONNECTED;
  pingIntervalRemaining = 0;
  if (events_[Event::DISCONNECTED] != nullptr) events_[DISCONNECTED]();
}

bool mqtt::Connection::sendPINGREQ() {
  #ifdef DEBUG
  Serial.println("sendPINGREQ");
  #endif
  bool result;
  if (state == ConnectionState::CONNECTED) {
    result = (client.write((byte)12 << 4) == 1);
    result &= (client.write((byte)0) == 1);
    return result;
  } else {
    return false;
    #ifdef DEBUG
    Serial.println("sendPINGREQ failed");
    #endif
  }
}

byte mqtt::Connection::pingInterval() {
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

void mqtt::Connection::poll() {
  unsigned long int currentMillis;

  if (state == ConnectionState::CONNECTED) {
    if (client.connected()) {
    
      if (client.available()) {
        dataAvailable();
      }
      
      currentMillis = millis();

      if (lastMillis + 1000UL > currentMillis) {
        lastMillis = currentMillis;
        pending.interval();
        pingInterval();
      }

    } else {
      if (state == ConnectionState::CONNECTED) {
        disconnected();
      }
    }
  } else if (state == ConnectionState::DISCONNECTED) {
    currentMillis = millis();
    if (autoReconnect && client.connected() && (lastMillis + reconnectTimeout > currentMillis)) {
      Serial.println("Re-establishing mqtt connection")
      lastMillis = currentMillis();
      reconnect();
    }
  } else if (state == ConnectionState::CONNECTING) {
    unsigned long int currentMillis = millis();
    if (!client.connected() || (lastMillis + connectTimeout > currentMillis) {
      Serial.println("Error: Connection timed out");
      state = ConnectionState::DISCONNECTED;
      lastMillis = currentMillis;
    } 
  }
 
}

byte mqtt::Connection::handlePacket(PacketType packetType, byte flags, long remainingLength) {
  switch (packetType) {
    case CONNACK   : return recvCONNACK(); break;
    case SUBACK    : return recvSUBACK(remainingLength); break;
    case UNSUBACK  : return recvUNSUBACK(); break;
    case PUBLISH   : return recvPUBLISH(flags,remainingLength); break;
    #ifdef DEBUG
    case PINGRESP  : Serial.println("PINGRESP"); return MQTT_ERROR_NONE; break;
    #else
    case PINGRESP  : return ErrorCode::NONE; break;
    #endif
    case PUBACK    : return recvPUBACK(); break;
    case PUBREC    : return recvPUBREC(); break;
    case PUBREL    : return recvPUBREL(); break;
    case PUBCOMP   : return recvPUBCOMP(); break;
    default: return ErrorCode::UNHANDLED_PACKETTYPE;
  }
}

byte mqtt::Connection::dataAvailable() {
  int i;
  byte b;
  byte flags;
  PacketType packetType;
  long remainingLength=0;

  i = client.read();
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

  return handlePacket(packetType,flags,remainingLength);
}

bool mqtt::Connection::subscribe(MQTTSubscriptionList& subs) {
  if (isConnected) sendSUBSCRIBE(subs);
  subscriptions_.push(subs);
}

bool mqtt::Connection::subscribe(const String& filter, const qos_t qos, const MQTTMessageHandlerFunc handler) {
  MQTTSubscriptionList subs;
  MQTTSubscription* sub = new MQTTSubscription(filter,qos,handler);
  if (sub->filter.valid) {
    subs.push(sub);
    subscribe(subs);
  }
}

/** @brief  Creates an MQTT Subscribe packet from a list of subscriptions and sends it to the server */
bool mqtt::Connection::sendSUBSCRIBE(const MQTTSubscriptionList& subscriptions) {
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

byte mqtt::Connection::recvSUBACK(const long remainingLength) {
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

bool mqtt::Connection::unsubscribe(const String& filter) {
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

byte mqtt::Connection::recvUNSUBACK() {
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

bool mqtt::Connection::publish(const String& topic, byte* data, const size_t data_len, const qos_t qos, const bool retain) {
  MQTTMessage* msg = new MQTTMessage(topic);
  msg->qos = qos;
  msg->retain = retain;
  msg->data = data;
  msg->data_len = data_len;
  return sendPUBLISH(msg);
}

bool mqtt::Connection::publish(const String& topic, const String& data, const qos_t qos, const bool retain) {
  MQTTMessage* msg = new MQTTMessage(topic);
  msg->qos = qos;
  msg->retain = retain;
  msg->data = (byte*)data.c_str();
  msg->data_len = data.length();
  return sendPUBLISH(msg);
}

bool mqtt::Connection::sendPUBLISH(const char* topic, const byte* data, const size_t data_len, const QoS qos, const bool retain, const bool duplicate) {

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
        (client.write(0x30 | flags) == 1) &&
        writeRemainingLength(remainingLength) &&
        writeStr(topic)
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

byte mqtt::Connection::recvPUBLISH(const byte flags, const long remainingLength) {
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

byte mqtt::Connection::recvPUBACK() {
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

bool mqtt::Connection::sendPUBACK(const word packetid) {
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

byte mqtt::Connection::recvPUBREC() {
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

bool mqtt::Connection::sendPUBREC(const word packetid) {
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

byte mqtt::Connection::recvPUBREL() {
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

bool mqtt::Connection::sendPUBREL(const word packetid) {
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

byte mqtt::Connection::recvPUBCOMP() {
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

bool mqtt::Connection::sendPUBCOMP(const word packetid) {
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

} // namespace