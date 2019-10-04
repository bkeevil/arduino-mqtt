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
  MQTTToken_t* next = nullptr;
};

class MQTTTokenizer {
  public:
    ~MQTTTokenizer() { clear(); }
    static bool checkTopicMatchesFilter(MQTTTokenizer& topic, MQTTTokenizer& filter);
    void clear();  
    String asString() const;
    inline bool tokenizeTopic(const String& topic) { return tokenize(topic,false); } 
    inline bool tokenizeFilter(const String& filter) { return tokenize(filter,true); }
    byte count = 0;
    MQTTToken_t* first = nullptr; 
  private:
    bool tokenize(String text, bool isFilter);
    void _tokenize(String& text, MQTTToken_t* ptr);
    bool validateTopicName();
    bool validateTopicFilter();  
    bool validateTopicName(MQTTToken_t* token);
    bool validateTopicFilter(MQTTToken_t* token, bool isLast); 
};

#endif