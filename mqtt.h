#include <Arduino.h>

#define MQTT_DEFAULT_PING_INTERVAL               30 // Number of seconds between pings
#define MQTT_DEFAULT_PING_RETRY_INTERVAL          6 // Frequency of pings in seconds after a failed ping response.
#define MQTT_DEFAULT_KEEPALIVE                   60 // Number of seconds of inactivity before disconnect
#define MQTT_MAX_TOPIC_LEN                       64 // Bytes
#define MQTT_MAX_DATA_LEN                        64 // Bytes
#define MQTT_PACKET_QUEUE_SIZE                    8
#define MQTT_MIN_PACKETID                       256 // The first 256 packet IDs are reserved for subscribe/unsubscribe packet ids
#define MQTT_MAX_PACKETID                     65535
#define MQTT_PACKET_TIMEOUT                       3 // Number of seconds before a packet is resent
#define MQTT_PACKET_RETRIES                       2 // Number of retry attempts to send a packet before the connection is considered dead

#define ptBROKERCONNECT                           0
#define ptCONNECT                                 1
#define ptCONNACK                                 2
#define ptPUBLISH                                 3
#define ptPUBACK                                  4
#define ptPUBREC                                  5
#define ptPUBREL                                  6
#define ptPUBCOMP                                 7
#define ptSUBSCRIBE                               8
#define ptSUBACK                                  9
#define ptUNSUBSCRIBE                            10
#define ptUNSUBACK                               11
#define ptPINGREQ                                12
#define ptPINGRESP                               13
#define ptDISCONNECT                             14

#define qtAT_MOST_ONCE                            0
#define qtAT_LEAST_ONCE                           1
#define qtEXACTLY_ONCE                            2 

#define MQTT_CONNACK_SUCCESS                      0
#define MQTT_CONNACK_UNACCEPTABLE_PROTOCOL        1
#define MQTT_CONNACK_CLIENTID_REJECTED            2
#define MQTT_CONNACK_SERVER_UNAVAILABLE           3
#define MQTT_CONNACK_BAD_USERNAME_PASSWORD        4
#define MQTT_CONNACK_NOT_AUTHORIZED               5

#define MQTT_ERROR_NONE                           0

#define MQTT_ERROR_ALREADY_CONNECTED            101
#define MQTT_ERROR_NOT_CONNECTED                102
#define MQTT_ERROR_INSUFFICIENT_DATA            103
#define MQTT_ERROR_REMAINING_LENGTH_ENCODING    104
#define MQTT_ERROR_INVALID_PACKET_FLAGS         105
#define MQTT_ERROR_PACKET_INVALID               106
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
#define MQTT_ERROR_NOT_IMPLEMENTED              122
#define MQTT_ERROR_PACKET_QUEUE_FULL            123
#define MQTT_ERROR_PACKETID_NOT_FOUND           124
#define MQTT_ERROR_SEND_PUBCOMP_FAILED          125
#define MQTT_ERROR_SEND_PUBREL_FAILED           126
#define MQTT_ERROR_PACKET_QUEUE_TIMEOUT         127

#define MQTT_ERROR_UNKNOWN                      255 

struct WillMessage {
  char topic[MQTT_MAX_TOPIC_LEN+1];
  char data[MQTT_MAX_DATA_LEN+1];
  bool enabled;
  bool retain;
  byte qos;  
};

struct PublishMessage {
  word packetid;
  byte timeout;
  byte retries;
  byte qos;
  bool retain;
  bool duplicate;
  char topic[MQTT_MAX_TOPIC_LEN+1];
  char data[MQTT_MAX_DATA_LEN+1];
};

struct PacketMessage {
  word packetid;
  byte timeout;
  byte retries;
};

class MQTTClient {
  private:
    PublishMessage outgoingPUBLISHQueue[MQTT_PACKET_QUEUE_SIZE];
    PublishMessage  incomingPUBLISHQueue[MQTT_PACKET_QUEUE_SIZE];
    PacketMessage  PUBRELQueue[MQTT_PACKET_QUEUE_SIZE];
    byte incomingPUBLISHQueueCount;
    byte outgoingPUBLISHQueueCount;
    byte PUBRELQueueCount;
    word nextPacketID = MQTT_MIN_PACKETID;
    int  pingIntervalRemaining;
    byte pingCount;
    //
    bool readByte(byte* b);
    bool writeByte(const byte b);    
    bool readWord(word *value);
    bool writeWord(const word value);
    bool readRemainingLength(long *value);
    bool writeRemainingLength(const long value);
    bool readData(char* data, const word len);
    bool writeData(char* data, const word len);
    bool writeStr(char* str);
    bool readStr(char* str, const word len);
    //
    void reset();
    byte pingInterval();
    bool queueInterval();
    bool addToOutgoingQueue(word packetid, byte qos, bool retain, bool duplicate, char* topic, char* data);
    bool addToIncomingQueue(word packetid, byte qos, bool retain, bool duplicate, char* topic, char* data);
    bool addToPUBRELQueue(word packetid);
    void deleteFromOutgoingQueue(byte i);
    void deleteFromIncomingQueue(byte i); 
    void deleteFromPUBRELQueue(byte i);
    //
    byte recvCONNACK();
    byte recvPINGRESP();
    byte recvSUBACK(long remainingLength);
    byte recvUNSUBACK();
    byte recvPUBLISH(byte flags, long remainingLength);
    byte recvPUBACK();
    byte recvPUBREC();
    byte recvPUBREL();
    byte recvPUBCOMP();  
    //
    bool sendPINGREQ();
    bool sendPUBACK(word packetid);
    bool sendPUBREL(word packetid);
    bool sendPUBREC(word packetid);
    bool sendPUBCOMP(word packetid);
  public:
    Stream* stream;
    WillMessage willMessage;
    bool isConnected;
    // Events
    virtual void connected() {};
    virtual void initSession() {};
    virtual void subscribed(word packetID, byte resultCode) {};
    virtual void unsubscribed(word packetID) {};
    virtual void receiveMessage(char *topic, char *data, bool retain, bool duplicate) {};
    // Methods
    bool connect(char *clientID, char *username, char *password, bool cleanSession = false, word keepAlive = MQTT_DEFAULT_KEEPALIVE);
    bool disconnect();
    void disconnected();
    bool subscribe(word packetid, char *filter, byte qos = qtAT_MOST_ONCE);
    bool unsubscribe(word packetid, char *filter);
    bool publish(char *topic, char *data, byte qos = qtAT_MOST_ONCE, bool retain=false, bool duplicate=false);
    byte dataAvailable(); // Needs to be called whenever there is data available
    byte intervalTimer(); // Needs to be called by program every second  
};

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

void MQTTClient::reset() {
  pingIntervalRemaining = 0;
  pingCount = 0;
  incomingPUBLISHQueueCount = 0;
  outgoingPUBLISHQueueCount = 0;
  PUBRELQueueCount = 0;
  isConnected = false;
}

bool MQTTClient::connect(char *clientID, char *username, char *password, bool cleanSession, word keepAlive)
{
  byte flags;
  word rl;      // Remaining Length

  reset();
  
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

  if (willMessage.retain) {
    flags |= 32;
  }
  flags |= (willMessage.QoS << 3);
  if (willMessage.enabled) {
    flags |= 4;
    rl += strlen(willMessage.topic) + 2 + strlen(willMessage.data) + 2;
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

bool MQTTClient::addToOutgoingQueue(word packetid, byte qos, bool retain, bool duplicate, char* topic, char* data) {
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
  strlcpy(outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].topic,topic,MQTT_MAX_TOPIC_LEN);
  strlcpy(outgoingPUBLISHQueue[outgoingPUBLISHQueueCount].data,data,MQTT_MAX_DATA_LEN);
  outgoingPUBLISHQueueCount++;
  return true;
}

bool MQTTClient::addToIncomingQueue(word packetid, byte qos, bool retain, bool duplicate, char* topic, char* data) {
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
  strlcpy(incomingPUBLISHQueue[incomingPUBLISHQueueCount].topic,topic,MQTT_MAX_TOPIC_LEN);
  strlcpy(incomingPUBLISHQueue[incomingPUBLISHQueueCount].data,data,MQTT_MAX_DATA_LEN);
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
    strlcpy(outgoingPUBLISHQueue[j].topic,outgoingPUBLISHQueue[j+1].topic,MQTT_MAX_TOPIC_LEN);
    strlcpy(outgoingPUBLISHQueue[j].data,outgoingPUBLISHQueue[j+1].topic,MQTT_MAX_DATA_LEN);            
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
    strlcpy(incomingPUBLISHQueue[j].topic,incomingPUBLISHQueue[j+1].topic,MQTT_MAX_TOPIC_LEN);
    strlcpy(incomingPUBLISHQueue[j].data,incomingPUBLISHQueue[j+1].topic,MQTT_MAX_DATA_LEN);            
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

bool MQTTClient::subscribe(word packetid, char *filter, byte qos) {
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

bool MQTTClient::publish(char *topic, char *data, byte qos, bool retain, bool duplicate) {
  byte flags = 0;
  word packetid;
  long remainingLength;
  bool result;

  
  if ((topic != NULL) && (strlen(topic)>0) && (qos<3) && (isConnected)) {

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
