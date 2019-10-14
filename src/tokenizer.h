#ifndef MQTT_TOKENIZER_H
#define MQTT_TOKENIZER_H

#include "Arduino.h"

namespace mqtt {

  // Forward Declarations
  class Token;
  class Tokenizer;
  class Topic;
  class Filter;

  /** @brief Classifies a token in a topic name or filter string */
  enum class TokenKind : byte {
    UNKNOWN=0,        /**< Validate() has not been called */
    INVALID,          /**< The token is invalid */
    VALID,            /**< The token is valid */
    MULTILEVEL,       /**< The token is the multi level wildcard character "# */
    SINGLELEVEL       /**< The token is the single level wildcard character "+" */
  };

  /** @brief    Represents a token in a topic stric or filter 
   *  @details  Token is used internally for validating topic names and filters and for matching topics to subscriptions.
   */
  class Token {
    public:
      Token() = default;
      Token(const Token& rhs) : text(rhs.text), kind(rhs.kind), next(nullptr) {}
      Token(Token&& rhs) : text(std::move(rhs.text)), kind(rhs.kind), next(rhs.next) {}
      String text;
      TokenKind kind {TokenKind::UNKNOWN};
      Token* next {nullptr};
  };

  /** @brief    Parses a topic string or filter into tokens
   *  @details  Tokenizer is the abstract ancestor class of Topic and Filter and provides methods for parsing
   *            a string into a list of tokens and validating those tokens according to MQTT protocol specs
   */
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

  /** @brief    Represents a filter string in a subscription 
   *  @details  Internally stores the string as a list of parsed tokens
   *            Validates a filter string to ensure it conforms to the MQTT protocol
   *            Provides an efficient mechanism for comparing a Topic to see if it matches the Filter
   */
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

  /** @brief    Represents a topic string in a message 
   *  @details  Internally stores the string as a list of parsed tokens
   *            Validates the topic string to ensure it conforms to the MQTT protocol
   */
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

}; // Namespace mqtt

#endif