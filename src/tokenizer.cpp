#include "tokenizer.h"

using namespace mqtt;

/** @brief  Copy Constructor */
Tokenizer::Tokenizer(const Tokenizer& rhs) {
  count = rhs.count;
  valid = rhs.valid;
  if (rhs.first == nullptr) {
    first = nullptr;
  } else {
    first = new Token(*rhs.first);
    Token* ptr_rhs = rhs.first->next;
    Token* ptr_lhs = first;
    while (ptr_rhs != nullptr) {
      ptr_lhs->next = new Token(*ptr_rhs);
      ptr_lhs = ptr_lhs->next;
      ptr_rhs = ptr_rhs->next;
    }
  }
}
  
/** @brief  Move Constructor */
Tokenizer::Tokenizer(Tokenizer&& rhs) {
  count = rhs.count;
  valid = rhs.valid;
  first = rhs.first;
  rhs.first = nullptr;
  rhs.count = 0;
  rhs.valid = false;
}

/** @brief  Move assignment operator */

Tokenizer& Tokenizer::operator=(Tokenizer&& rhs) {
  if (&rhs == this) return *this;
  count = rhs.count;
  valid = rhs.valid;
  first = rhs.first;
  rhs.first = nullptr;
  rhs.count = 0;
  rhs.valid = false;
  return *this;  
}

/** @brief  Parse a topic name or topic filter string into a linked list of tokens
 *  @param  text  The topic or filter sting to Parse
 */
void Tokenizer::tokenize() {
  clear();
  Token* ptr = nullptr;
  int len = text.length();

  if (len > 0) {
    // Strip off ending '/' character before starting recursive tokenize routine
    if (text.lastIndexOf('/') == len - 1) {
      // Don't start tokenize if text="/"
      if (len > 1) {
        _tokenize(text.substring(0,len-1), ptr);
      }
    } else {
      _tokenize(text, ptr);
    }
  }
}

/** @brief  Recursively tokenize text and add the tokens to the linked list.
 *  @remark Called by tokenize()
 */
void Tokenizer::_tokenize(String text, Token* ptr) {
  Token* node = new Token;

  if (ptr == nullptr) {
    first = node;
  } else {
    ptr->next = node;
  }
      
  ++count;
  
  int pos = text.indexOf('/');
  if (pos < 0) {
    node->text = text;
  } else {
    node->text = text.substring(0,pos);
    if (pos < text.length() - 1) {
      _tokenize(text.substring(pos+1),node); 
    }
  }
}

void Tokenizer::clear() {
  Token* ptr;
  while (first != nullptr) {
    ptr = first->next;
    delete first;
    first = ptr;
  }
  first = nullptr;
  count = 0;
  text = "";
}

/* Topic */

/** @brief    Validates the tokenized topic string.
 *  @returns  True if the topic string is valid.
 *  @remark   An empty topic string is invalid
 *  @remark   If the topic string is found to be invalid the token list is cleared
 */
bool Topic::validate() {
  Token* ptr;

  // An empty topic string is invalid
  if (count == 0) return false;
  
  ptr = first;
  while (ptr != nullptr) {
    if (!validateToken(*ptr)) {
      clear();
      return false;
    }
    ptr = ptr->next;
  }

  return true;
}

/** @brief    Validates a token as a topic name. 
 *            Sets the token kind to tkValid or tkInvalid.
 *  @param    token   A reference to the token to validate
 *  @remark   Empty string is valid
 *  @remark   Any token containing a hash or a plus is invalid
 *  @returns  True if the token is valid 
 */ 
bool Topic::validateToken(Token& token) {   // TODO: Should token be const?
  if ((token.text.length() == 0) || ((token.text.indexOf('#') == -1) && (token.text.indexOf('+') == -1))) {
    token.kind = tkValid;
    return true;
  } else {
    token.kind = tkInvalid;
    return false;
  }
}

/* Filter */

/** @brief    Validates the tokenized filter string.
 *  @returns  True if the filter string is valid.
 *  @remark   An empty topic string is invalid
 *  @remark   If the topic string is determined to be invalid, the token list is cleared.
 */
bool Filter::validate() {
  Token* ptr;

  // An empty filter string is invalid
  if (first == nullptr) {
    return false;
  }

  ptr = first;
  while (ptr != nullptr) {
    if (!validateToken(*ptr)) {
      clear();
      return false;
    }
    ptr = ptr->next;
  }

  return true;
}

/** @brief    Validates a token as a topic name. 
 *            Sets the token kind to tkValid or tkInvalid.
 *  @param    token   The token to validate
 *  @remark   An empty string is always valid
 *  @remark   Any token not containing a special char is valid
 *  @remark   The hash character must only appear on its own
 *  @remark   The hash character must only be in the last in the list of tokens
 *  @remark   The plus character must only appear on its own
 *  @returns  True if the token is valid 
 */ 
bool Filter::validateToken(Token& token) { // TODO: Should token be const?
  size_t len;
  int hashPos;
  int plusPos;
  
  // An empty string is always valid
  len = token.text.length();
  if (len == 0) {
    token.kind = tkValid;
    return true;
  }

  hashPos = token.text.indexOf('#');
  plusPos = token.text.indexOf('+');

  // Any token not containing a special char is valid
  if ((hashPos == -1) && (plusPos == -1)) {
    token.kind = tkValid;
    return true;
  }

  // The hash and plus character must only appear on their own
  // The hash character must only be in the last in the list of tokens
  if ((hashPos > 0) || (plusPos > 0) || ((hashPos == 0) && (token.next != nullptr))) {
    token.kind = tkInvalid;
    return false;
  } 
  
  // Token is valid but set the token kind enum for special chars
  if (hashPos == 0) {
    token.kind = tkMultiLevel;
  } else if (plusPos == 0) {
    token.kind = tkSingleLevel;
  } else {
    token.kind = tkValid;
  }
  
  return true;
}

/** @brief    Returns true if the topic string matches the filter
 *  @param    topic The topic to match
 *  @Returns  True if the topic matches the filter
 */   
bool Filter::match(const Topic& topic) const {
  int i = 0;
  bool result = false;

  Token* filterPtr;
  Token* topicPtr;
  String s;

  filterPtr = first;
  topicPtr  = topic.first;

  while (filterPtr != nullptr) {
    if (i >= topic.count) {
      return (result && (filterPtr->kind == tokenKind_t::tkMultiLevel));
    }
    
    if (filterPtr->kind == tkInvalid) { 
      return false;
    } else if (filterPtr->kind == tokenKind_t::tkValid) {
      result = (filterPtr->text == topicPtr->text);
      if (!result) return result;
    } else if (filterPtr->kind == tkMultiLevel) {
      return true;
    } else {
      result = true;
    }
    
    ++i;
    filterPtr = filterPtr->next;
    topicPtr = topicPtr->next;
  }
  
  if (count < topic.count) {
    // Retrieve the count - 1 token from the filter list
    i = 0;
    filterPtr = first;
    while ((filterPtr != nullptr) && (i < count - 1)) {
      ++i;
      filterPtr = filterPtr->next;
    }
    result = (result && (filterPtr != nullptr) && filterPtr->kind == tkMultiLevel);
  }

  return result;
}

/** @brief    Returns true if the filters are equal
 *  @param    filter  The filter to compare
 *  @Returns  True if the filters are an exact match
 */   
bool Filter::equals(const Filter& filter) const {
  Token* lhs = first;
  Token* rhs = filter.first;

  while (lhs != nullptr) {
    if ((rhs == nullptr) || (lhs->kind == tkInvalid) || (rhs->kind == tkInvalid) || (lhs->kind != rhs->kind) || ((lhs->kind == tkValid) && !lhs->text.equals(rhs->text))) {
      return false;
    }
    lhs = lhs->next;
    rhs = rhs->next;
  }
  
  return (rhs == nullptr);
}
