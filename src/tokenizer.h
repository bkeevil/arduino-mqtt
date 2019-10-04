#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "Arduino.h"

enum TokenKind_t {
  tkInvalid=0,
  tkValid,
  tkMultiLevel,
  tkSingleLevel
};

class MQTTToken {
  friend class MQTTTokenizer;
  public:
    MQTTToken() : kind(tkInvalid), next(nullptr) {};
    String text;
    TokenKind_t kind;
    MQTTToken* next;
  private:
    bool validateTopicName();
    bool validateTopicFilter(bool isLast); 
};

class MQTTTokenizer {
  public:
    MQTTTokenizer() : count(0), first(nullptr) {}
    ~MQTTTokenizer() { clear(); }
    static bool checkTopicMatchesFilter(MQTTTokenizer& topic, MQTTTokenizer& filter);
    void clear();  
    String& asString(String&s) const;
    inline bool tokenizeTopic(const String& topic) { return tokenize(topic,false); } 
    inline bool tokenizeFilter(const String& filter) { return tokenize(filter,true); }
    byte count;
    MQTTToken* first; 
  private:
    bool tokenize(String text, bool isFilter);
    void _tokenize(String& text, MQTTToken* ptr);
    bool validateTopicName();
    bool validateTopicFilter();  
};

#endif
