#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "Arduino.h"

enum TokenKind_t {
  tkUnknown=0,
  tkInvalid,
  tkValid,
  tkMultiLevel,
  tkSingleLevel
};

class MQTTToken {
  public:
    MQTTToken() : kind(tkUnknown), next(nullptr) {};
    String text;
    TokenKind_t kind;
    MQTTToken* next;
};

class MQTTTokenizer {
  public:
    MQTTTokenizer() : count(0)_, first_(nullptr) {}
    ~MQTTTokenizer() { clear(); }
    void clear();
    bool setString(String& s) { tokenize(s); return validate(); }; 
    String& getString(String& s) const; 
  protected:
    static bool checkTopicMatchesFilter(MQTTTokenizer& filter, MQTTTokenizer& topic);
    virtual bool validate() = 0;
  private:
    void tokenize(const String& text);
    void _tokenize(String& text, MQTTToken* ptr);
    byte count_;
    MQTTToken* first_;
};

class MQTTTopic;

class MQTTFilter : public MQTTTokenizer {
  public:
    MQTTFilter() : MQTTTokenizer() {}
    MQTTFilter(const String& filter) : MQTTTokenizer() { tokenize(filter); valid_ = validate(); }
    bool valid() { return valid_; }
    bool match(MQTTTopic& topic) { return checkTopicMatchesFilter(this, topic); };
  protected:
    virtual bool validate() override;
  private:
    bool validateToken(MQTTToken& token);
    bool valid_;
};

class MQTTTopic : public MQTTTokenizer {
  public:
    MQTTTopic() : MQTTTokenizer() {}
    MQTTTopic(const String& topic) : MQTTTokenizer() { tokenize(topic); valid_ = validate(); }
    bool valid() { return valid_; }
    bool match(MQTTFilter& filter) { return checkTopicMatchesFilter(filter,this); };
  protected:
    virtual bool validate() override;
  private:
    bool validateToken(MQTTToken& token);    
    bool valid_;
};

#endif
