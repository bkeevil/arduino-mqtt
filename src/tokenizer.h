#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "Arduino.h"

enum TokenKind_t {
  tkInvalid=0,
  tkValid,
  tkMultiLevel,
  tkSingleLevel
};

struct MQTTToken_t {
  String text;
  TokenKind_t kind;
  MQTTToken_t* next;
};

class MQTTTokenizer {
  public:
    ~MQTTTokenizer() { clear(); }
    void clear();  
    String&& asString() const;
    bool tokenize(String&& text, bool isFilter);  
    byte count;
    MQTTToken_t* first; 
  private:
    bool validateTopicName();
    bool validateTopicFilter();  
    bool validateTopicName(MQTTToken_t* token);
    bool validateTopicFilter(MQTTToken_t* token, bool isLast); 
};

bool checkTopicMatchesFilter(MQTTTokenizer& topic, MQTTTokenizer& filter);

#endif