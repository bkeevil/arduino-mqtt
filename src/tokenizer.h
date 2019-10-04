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
    MQTTTokenizer() : count_(0), first_(nullptr), valid_(false) {}
    MQTTTokenizer(const String& text) : count_(0), first_(nullptr) { tokenize(text); }
    ~MQTTTokenizer() { clear(); }
    void clear();
    bool valid() const { return valid_; }
    byte count() const { return count_; }
    MQTTToken* first() const { return first_; }
    bool setString(const String& s) { tokenize(s); return validate(); }; 
    String& getString(String& s) const; 
  protected:
    void tokenize(const String& text);
    virtual bool validate() = 0;
    byte count_;
    MQTTToken* first_;
    bool valid_;
  private:
    void _tokenize(String text, MQTTToken* ptr);
};

class MQTTTopic;

class MQTTFilter : public MQTTTokenizer {
  public:
    MQTTFilter() : MQTTTokenizer() {}
    MQTTFilter(const String& filter) : MQTTTokenizer(filter) { valid_ = validate(); }
    bool match(const MQTTTopic& topic) const;
  protected:
    virtual bool validate() override;
  private:
    bool validateToken(MQTTToken& token);
};

class MQTTTopic : public MQTTTokenizer {
  public:
    MQTTTopic() : MQTTTokenizer() {}
    MQTTTopic(const String& topic) : MQTTTokenizer(topic) { valid_ = validate(); }
    bool match(MQTTFilter& filter) const { return filter.match(*this); };
  protected:
    virtual bool validate() override;
  private:
    bool validateToken(MQTTToken& token);    
};

#endif
