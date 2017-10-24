#define ptBROKERCONNECT                         0
#define ptCONNECT                               1
#define ptCONNACK                               2
#define ptPUBLISH                               3
#define ptPUBACK                                4
#define ptPUBREC                                5
#define ptPUBREL                                6
#define ptPUBCOMP                               7
#define ptSUBSCRIBE                             8
#define ptSUBACK                                9
#define ptUNSUBSCRIBE                          10
#define ptUNSUBACK                             11
#define ptPINGREQ                              12
#define ptPINGRESP                             13
#define ptDISCONNECT                           14

#define qtAT_MOST_ONCE                          0
#define qtAT_LEAST_ONCE                         1
#define qtEXACTLY_ONCE                          2

#define MQTT_CONNACK_SUCCESS                    0
#define MQTT_CONNACK_UNACCEPTABLE_PROTOCOL      1
#define MQTT_CONNACK_CLIENTID_REJECTED          2
#define MQTT_CONNACK_SERVER_UNAVAILABLE         3
#define MQTT_CONNACK_BAD_USERNAME_PASSWORD      4
#define MQTT_CONNACK_NOT_AUTHORIZED             5

#define MQTT_ERROR_NONE                         0
#define MQTT_ERROR_ALREADY_CONNECTED            101
#define MQTT_ERROR_NOT_CONNECTED                102
#define MQTT_ERROR_INSUFFICIENT_DATA            103
#define MQTT_ERROR_REMAINING_LENGTH_ENCODING    104
#define MQTT_ERROR_INVALID_PACKET_FLAGS         105
#define MQTT_ERROR_PACKET_INVALID               106  // Packet was parsed successfully but failed final validation
#define MQTT_ERROR_PAYLOAD_INVALID              107
#define MQTT_ERROR_VARHEADER_INVALID            108
#define MQTT_ERROR_UNACCEPTABLE_PROTOCOL        109
#define MQTT_ERROR_CLIENTID_REJECTED            110
#define MQTT_ERROR_SERVER_UNAVAILABLE           111
#define MQTT_ERROR_BAD_USERNAME_PASSWORD        112
#define MQTT_ERROR_NOT_AUTHORIZED               113
#define MQTT_ERROR_NO_CLIENTID                  114
#define MQTT_ERROR_WILLMESSAGE_INVALID          115
#define MQTT_ERROR_NO_PING_RESPONSE             116
#define MQTT_ERROR_UNHANDLED_PACKETTYPE         117
#define MQTT_ERROR_NO_SUBSCRIPTION_LIST         118
#define MQTT_ERROR_INVALID_SUBSCRIPTION_ENTRIES 119
#define MQTT_ERROR_INVALID_RETURN_CODES         120
#define MQTT_ERROR_CONNECT_TIMEOUT              121

#define MQTT_ERROR_UNKNOWN                      255 
  
#define MQTT_DEFAULT_PING_INTERVAL               30
#define MQTT_DEFAULT_KEEPALIVE                   60

struct WillMessage {
  char *topic  = 0;
  char *data   = 0;
  bool enabled = false;
  bool retain  = false;
  byte QoS = qtAT_LEAST_ONCE;  
};

class MQTTClient {
  private:
    int pingIntervalRemaining;
    byte pingCount;
    // Packet handlers
    void handleCONNACK();
    void handleSUBACK();
    void handleUNSUBACK();
    void handlePUBLISH(byte flags);
    void handlePUBACK();
    void handlePUBREC();
    void handlePUBREL();
    void handlePUBCOMP();
    void handlePINGRESP();
    // Helper functions  
    bool readByte(byte *b);
    bool writeByte(const byte b);
    bool readWord(word *value);
    bool writeWord(const word value);
    bool readStr(char **str);
    bool writeStr(char *str);
    bool readRemainingLength(unsigned long *value);
    bool writeRemainingLength(const unsigned long value);
    // Timer functions
    void startPingTimer();
    void stopPingTimer();
    void resetPingTimer();
    bool ping();
  protected:
    virtual void connected(); 
    virtual void disconnected();
    virtual void initSession();
    virtual void bail(byte errorCode);
  public:
    Stream *stream;
    bool isConnected = false;
    bool connect(char *clientID, char *username, char *password, WillMessage *willMessage, bool cleanSession = false, word keepAlive = MQTT_DEFAULT_KEEPALIVE);
    bool disconnect();
    byte dataAvailable();
    void doPingInterval();    
};

MQTTClient client;

bool MQTTClient::readByte(byte *b) {
  if (stream->available() == 0) {
    delay(100);
  }
  if (stream->available() > 0) {
    *b = stream->read();
    return (b == 1);
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
    
bool MQTTClient::readRemainingLength(unsigned long *value) {
  unsigned long multiplier = 1;
  byte encodedByte;
  
  *value = 0;
  do {
    if (readByte(&encodedByte)) {
      value += (encodedByte & 127) * multiplier;
      multiplier *= 128;
      if (multiplier > 128*128*128) {
        return false;
        exit;
      }
    } else {
      return false;
      exit;
    }    
  } while ((encodedByte and 128) > 0);
  return true;
}

bool MQTTClient::writeRemainingLength(const unsigned long value) {
  byte encodedByte;
  unsigned long lvalue;

  lvalue = value;
  do {
    encodedByte = lvalue % 128;
    lvalue = lvalue / 128;
    if (lvalue > 0) {
      encodedByte |= 128;
    }  
    if (!writeByte(encodedByte)) {
      return false;
      exit;
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

bool MQTTClient::writeStr(char *str) {
  char *ptr;
  word len;
  
  len = strlen(str);
  if (writeWord(len)) {
    ptr = str;
    while (len > 0) {  
      if (!writeByte(byte(*ptr))) {
        return false;
        exit;
      }
      ptr++;
      len--;
    }
    return true;
  } else {
    return false;  
  }
}

bool MQTTClient::readStr(char **str) {
  word len;
  char *ptr;
  byte b;
  if (readWord(&len)) {
    *str = (char *)malloc(len);
    ptr = *str;
    while (len>0) {
      if (readByte(ptr)) {
        ptr++;
        len--;
      } else {
        free(*str);
        *str = NULL;
        return false;
        exit;
      }
    }
    return true;  
  } else {
    return false;
  }
}

bool MQTTClient::connect(char *clientID, char *username, char *password, WillMessage *willMessage, bool cleanSession = false, word keepAlive = MQTT_DEFAULT_KEEPALIVE)
{
  byte flags;
  word rl;      // Remaining Length

  rl = 10 + 2 + strlen(clientID);
  
  if (username != NULL) {
    flags = 128;
    rl += strlen(username) + 2;
  } else {
    flags = 0;
  }  
  
  if (password != NULL) {
    flags |= 64;
    rl += strlen(password) + 2;
  }
  
  if (willMessage != NULL) {
    if (willMessage->retain) {
      flags |= 32;
    }
    flags |= (willMessage->QoS << 3);
    if (willMessage->enabled) {
      flags |= 4;
      rl += strlen(willMessage->topic) + 2 + strlen(willMessage->data) + 2;
    }
  }
      
  if (cleanSession) {
    flags |= 2;
  }  

  if ((!writeByte(0x10)) || 
     (!writeWord(rl)) ||
     (!writeByte(0)) ||
     (!writeByte(4)) ||
     (!writeByte('M')) ||
     (!writeByte('Q')) ||
     (!writeByte('T')) ||
     (!writeByte('T')) ||
     (!writeByte(4))) 
       { return false; exit; }
  
  if ((!writeByte(flags)) || 
     (!writeWord(keepAlive)) || 
     (!writeStr(clientID))) 
       { return false; exit; }
  
  if ((willMessage != NULL) && (willMessage->enabled)) {
    if (!writeStr(willMessage->topic) || !writeStr(willMessage->data)) {
      return false; exit;
    }
  }
  
  if (username != NULL) {
    if (!writeStr(username)) {
      return false; exit;
    }
  }
  
  if (password != NULL) {
    if (!writeStr(password)) {
      return false; exit;
    }
  }
  
  return true;
}

bool MQTTClient::disconnect() {
  if (writeByte(0xE0) && writeByte(0)) {
    isConnected = false;
    return true; 
  } else {
    return false;
  }
}

byte MQTTClient::dataAvailable() {
  byte b;
  byte flags;
  byte packetType;
  unsigned long remainingLength; // remaining length

  if (readByte(b)) {
    flags = b & 0x0F;
    packetType = b >> 4;
  } else {
    return MQTT_ERROR_INSUFFICIENT_DATA;
    exit;
  }
  
  if (!readRemainingLength(&remainingLength)) {
    return MQTT_ERROR_INSUFFICIENT_DATA;
    exit;
  }

  resetPingTimer();
  
  switch (packetType) {
    case ptCONNACK   : handleCONNACK(); break;
    case ptSUBACK    : handleSUBACK(); break;
    case ptUNSUBACK  : handleUNSUBACK(); break;
    case ptPUBLISH   : handlePUBLISH(flags); break;
    case ptPUBACK    : handlePUBACK(); break;
    case ptPINGRESP  : handlePINGRESP(); break;
    case ptPUBREC    : handlePUBREC(); break;
    case ptPUBREL    : handlePUBREL(); break;
    case ptPUBCOMP   : handlePUBCOMP(); break;
    default: return MQTT_ERROR_UNHANDLED_PACKETTYPE; exit;
  }

  return MQTT_ERROR_NONE;
}

void MQTTClient::connected() {
  isConnected = true;
}

void MQTTClient::disconnected() {
  isConnected = false;
  stopPingTimer();    
}

void MQTTClient::initSession() {
  //FSubscriptions.Clear;
  //FWaitingForAck.Clear;
  //FPendingTransmission.Clear;
  //FPendingReceive.Clear;   
}

void MQTTClient::bail(byte errorCode) {
  disconnected(); 
}

void MQTTClient::startPingTimer() {
  pingIntervalRemaining = MQTT_DEFAULT_PING_INTERVAL;
  pingCount = 0;  
}

void MQTTClient::stopPingTimer() {
  pingIntervalRemaining = 0;  
}

void MQTTClient::resetPingTimer() {
  pingIntervalRemaining = MQTT_DEFAULT_PING_INTERVAL;    
}

void MQTTClient::doPingInterval() {
  if (pingIntervalRemaining == 1) {
    if (pingCount >= 2) {
      pingCount = 0;
      pingIntervalRemaining = 0;
      bail(MQTT_ERROR_NO_PING_RESPONSE);
      exit;
    }
    pingCount++;
    pingIntervalRemaining = 6;
    ping();
  } else {
    if (pingIntervalRemaining > 1) {
      pingIntervalRemaining--;
    }
  }
}

bool MQTTClient::ping() {
  
}

void MQTTClient::handleCONNACK() {
  byte b;
  bool sessionPresent = false;
  byte returnCode = MQTT_CONNACK_SUCCESS;    // Default return code is success

  if (readByte(&b)) {
    sessionPresent = (b == 1);    
  } else {
    bail(MQTT_ERROR_INSUFFICIENT_DATA);
    exit;
  }
  
  if ((b & 0xFE) > 0) {
    bail(MQTT_ERROR_PACKET_INVALID);
    exit;
  }
  
  if (readByte(&b)) {
    returnCode = b;    
  } else {
    bail(MQTT_ERROR_INSUFFICIENT_DATA);
    exit;
  }

  if (returnCode = MQTT_CONNACK_SUCCESS) {
    startPingTimer();
    if (!sessionPresent) {
      initSession();
    }
    connected();
  } else {
    switch (returnCode) {
      case MQTT_CONNACK_UNACCEPTABLE_PROTOCOL : bail(MQTT_ERROR_UNACCEPTABLE_PROTOCOL);
      case MQTT_CONNACK_CLIENTID_REJECTED     : bail(MQTT_ERROR_CLIENTID_REJECTED);
      case MQTT_CONNACK_SERVER_UNAVAILABLE    : bail(MQTT_ERROR_SERVER_UNAVAILABLE);
      case MQTT_CONNACK_BAD_USERNAME_PASSWORD : bail(MQTT_ERROR_BAD_USERNAME_PASSWORD);
      case MQTT_CONNACK_NOT_AUTHORIZED        : bail(MQTT_ERROR_NOT_AUTHORIZED);
    }
  }
}

void MQTTClient::handleSUBACK() {
  
}

void MQTTClient::handleUNSUBACK() {
  
}

void MQTTClient::handlePUBLISH(byte flags) {
  
}

void MQTTClient::handlePUBACK() {
  
}

void MQTTClient::handlePUBREC() {
  
}

void MQTTClient::handlePUBREL() {
  
}

void MQTTClient::handlePUBCOMP() {
  
}

void MQTTClient::handlePINGRESP() {
  
}
    
void setup() {
  /*WillMessage wm; 
  wm.topic   = "Coop/Online";
  wm.data    = "0";
  wm.retain  = true;
  wm.enabled = true; */
  client.stream = &Serial;
  client.connect("testclient",NULL,NULL,NULL,false,60);
}

void loop() {
  byte count;
  if (Serial.available() > 1)
    client.dataAvailable();
  delay(100);
  if (count++ == 9) {
    client.doPingInterval();
    count = 0;
  }
}
