#include "connection.h"

using namespace mqtt;

Connection::Connection(Stream& stream): Network(stream) {
  for (int i;i=0;i<Event::MAX) {
    events_[i] = nullptr;
  }
} 

void Connection::reset() {
  if (state != ConnectionState::DISCONNECTED) {
    disconnected();
  pingIntervalRemaining = 0;
  pingCount = 0;
  if (cleanSession) {
    Pending.clear();
  }
  state = ConnectionState::DISCONNECTED;
}

bool Connection::connect(const char* clientid, const char* username, const char* password) {
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

byte Connection::recvCONNACK() {
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

void Connection::connected() {
  state = ConnectionState::CONNECTED;
  if (client.available() > 0) {
    dataAvailable(); //???
  }
  if (connectMessage.enabled) {
    connectMessage.send(*this);
  }
}

void Connection::disconnect() {
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

void Connection::disconnected() {
  #ifdef DEBUG
  Serial.println("Server terminated the MQTT connection");
  #endif
  state = ConnectionState::DISCONNECTED;
  pingIntervalRemaining = 0;
  if (events_[Event::DISCONNECTED] != nullptr) events_[DISCONNECTED]();
}

bool Connection::sendPINGREQ() {
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

byte Connection::pingInterval() {
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

void Connection::poll() {
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

byte Connection::handlePacket(PacketType packetType, byte flags, long remainingLength) {
  switch (packetType) {
    case CONNACK   : return recvCONNACK(); break;
    //case SUBACK    : return recvSUBACK(remainingLength); break;
    //case UNSUBACK  : return recvUNSUBACK(); break;
    //case PUBLISH   : return recvPUBLISH(flags,remainingLength); break;
    #ifdef DEBUG
    case PINGRESP  : Serial.println("PINGRESP"); return MQTT_ERROR_NONE; break;
    #else
    case PINGRESP  : return ErrorCode::NONE; break;
    #endif
    //case PUBACK    : return recvPUBACK(); break;
    //case PUBREC    : return recvPUBREC(); break;
    //case PUBREL    : return recvPUBREL(); break;
    //case PUBCOMP   : return recvPUBCOMP(); break;
    default: return ErrorCode::UNHANDLED_PACKETTYPE;
  }
}

byte Connection::dataAvailable() {
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
