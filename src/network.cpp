#include "network.h"

using namespace mqtt;

bool Network::readRemainingLength(long* value) {
  long multiplier = 1;
  int i;
  byte encodedByte;

  *value = 0;
  do {
    i = client.read();
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

bool Network::writeRemainingLength(const long value) {
  byte encodedByte;
  long lvalue;

  lvalue = value;
  do {
    encodedByte = lvalue % 128;
    lvalue = lvalue / 128;
    if (lvalue > 0) {
      encodedByte |= 128;
    }
    if (client.write(encodedByte) != 1) {
      return false;
    }
  } while (lvalue > 0);
  return true;
}
 
bool Network::readWord(word* value) {
  int i;
  byte b;
  i = client.read();
  if (i > -1) {
    b = i;
    *value = b << 8;
    i = client.read();
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

bool Network::writeWord(const word value) {
  byte b = value >> 8;
  if (client.write(b) == 1) {
    b = value & 0xFF;
    if (client.write(b) == 1) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

bool Network::readStr(String& str) {
  word len;

  if (client.available() < len) {
    if (readWord(&len)) {
      str.reserve(len);
      str = client.readString();
      return (str.length() == len);
    } else {
      return false;
    }
  }
}

bool Network::writeStr(const String& str) {
  word len;

  len = str.length();
  if (writeWord(len)) {
    return  (client.print(str) == str.length());
  } else {
    return false;
  }
}