#ifndef MQTT_TOKENIZER_H
#define MQTT_TOKENIZER_H

#include "Arduino.h"


namespace mqtt {

  class Token;
  class Tokenizer;

  /** Classifies a token in a topic name or filter string */
  enum TokenKind {
    UNKNOWN=0,        /**< Validate() has not been called */
    INVALID,          /**< The token is invalid */
    VALID,            /**< The token is valid */
    MULTILEVEL,       /**< The token is the multi level wildcard character "# */
    SINGLELEVEL       /**< The token is the single level wildcard character "+" */
  };

  class Token {
    public:
      Token() = default;
      Token(const Token& rhs) : text(rhs.text), kind(rhs.kind), next(nullptr) {}
      Token(Token&& rhs) : text(std::move(rhs.text)), kind(rhs.kind), next(rhs.next) {}
      String text;
      TokenKind kind {UNKNOWN};
      Token* next {nullptr};
  };

  class Tokenizer {
    public:
      Tokenizer() : count(0), first(nullptr), valid(false) {}
      Tokenizer(const String& text) : count(0), first(nullptr), text(text), valid(false) { tokenize(); }
      Tokenizer(const Tokenizer& rhs);
      Tokenizer(Tokenizer&& rhs);
      Tokenizer& operator=(Tokenizer&& rhs);
      ~Tokenizer() { clear(); }
      void clear();
      bool setText(const String& s) { text = s; tokenize(); return validate(); }; 
      const String& getText() const { return text; } 
      byte count;
      Token* first;
      bool valid;  protected:
      void tokenize();
      virtual bool validate() = 0;
    private:
      void _tokenize(String text, Token* ptr);
      String text;
  };

  class Filter : public Tokenizer {
    public:
      Filter() : Tokenizer() {}
      Filter(const String& filter) : Tokenizer(filter) { valid = validate(); }
      Filter(const Filter& rhs) : Tokenizer(rhs) {}
      Filter(Filter&& rhs) : Tokenizer(std::move(rhs)) {}
      Filter& operator=(Filter&& rhs) { Tokenizer::operator=(std::move(rhs)); return *this; }
      bool match(const Topic& topic) const;
      bool equals(const Filter& filter) const;
      virtual bool validate() override;
    private:
      bool validateToken(Token& token);
  };

  class Topic : public Tokenizer {
    public:
      Topic() : Tokenizer() {}
      Topic(const String& topic) : Tokenizer(topic) { valid = validate(); }
      Topic(const Topic& rhs) : Tokenizer(rhs) {}
      Topic(Topic&& rhs) : Tokenizer(std::move(rhs)) {}
      bool match(const Filter& filter) const { return filter.match(*this); };
    protected:
      virtual bool validate() override;
    private:
      bool validateToken(Token& token);    
  };

}

#endif