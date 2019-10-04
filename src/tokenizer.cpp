#include "tokenizer.h"

bool MQTTTokenizer::checkTopicMatchesFilter(MQTTTokenizer& filter, MQTTTokenizer& topic) {
  int i = 0;
  bool result = false;

  MQTTToken* filterPtr;
  MQTTToken* topicPtr;

  filterPtr = filter.first;
  topicPtr  = topic.first;

  while (filterPtr != nullptr) {
    if (i >= topic.count) {
      return (result && (filterPtr->kind == TokenKind_t::tkMultiLevel));
    }
    
    if (filterPtr->kind == tkInvalid) { 
      return false;
    } else if (filterPtr->kind == TokenKind_t::tkValid) {
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
  
  if (filter.count < topic.count) {
    // Retrieve the count - 1 token from the filter list
    i = 0;
    filterPtr = filter.first;
    while ((filterPtr != nullptr) && (i < filter.count - 1)) {
      ++i;
      filterPtr = filterPtr->next;
    }
    result = (result && (filterPtr != nullptr) && filterPtr->kind == tkMultiLevel);
  }

  return result;
}

/** @brief  Parse a topic name or topic filter string into a linked list of tokens
 *  @param  text  The topic or filter sting to Parse
 */
void MQTTTokenizer::tokenize(const String& text) {
  clear();
  MQTTToken* ptr = nullptr;
  if (text.length() > 0) 
    _tokenize(text, ptr);
}

/** @brief  Recursively tokenize text and add the tokens to the linked list.
 *  @remark Called by tokenize()
 */
void MQTTTokenizer::_tokenize(const String& text, MQTTToken* ptr) {
  MQTTToken node = new MQTTToken;

  //Serial.print("  text="); Serial.println(text);

  if (ptr = nullptr) {
    first = node;
  } else {
    ptr.next = node;
  }
  
  ++count;
  
  int pos = text.indexOf('/');
  if (pos < 0) {
    node.text = text;
  } else {
    node.text = text.substring(0,pos);
    if (pos < text.length() - 1) {
      _tokenize(text,text.substring(pos+1)); 
    } 
  }
}

String& MQTTTokenizer::getString(String& s) const {
  int i=0;
  int size=0;
  MQTTToken* ptr;

  s = "";

  ptr = first;
  while (ptr != nullptr) {
    switch (ptr->kind) {
      case tkSingleLevel: ++size; break;
      case tkMultiLevel: ++size; break;
      case tkValid: size += ptr->text.length(); break;
    }    
    //Serial.print(ptr->text); 
    ++size;
    ptr = ptr->next;
  }

  //Serial.print("size="); Serial.println(size-1);

  s.reserve(size-1);

  ptr = first;
  while (ptr != nullptr) {
  
    switch (ptr->kind) {
      case tkSingleLevel: s += '+'; break;
      case tkMultiLevel: s += '#'; break;
      case tkValid: s += ptr->text; break;
    }
    if (i < count - 1) {
      s += '/';
    }
    ++i;
    ptr = ptr->next;
  }
  //Serial.print("asString="); Serial.println(s);
  return s;
}

void MQTTTokenizer::clear() {
  MQTTToken* ptr;
  while (first != nullptr) {
    ptr = first->next;
    delete first;
    first = ptr; 
  }
  first = nullptr;
  count = 0;
}

virtual bool MQTTTopic::validate() override {
  MQTTToken* ptr;

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
 *  @param    token   The token to validate
 *  @remark   Empty string is valid
 *  @remark   Any token containing a hash or a plus is invalid
 *  @returns  True if the token is valid 
 */ 
bool MQTTTopic::validateToken(MQTTToken& token) {
  if ((token.text.length() == 0) || ((token.text.indexOf('#') == -1) && (token.text.indexOf('+') == -1))) {
    token.kind = tkValid;
    return true;
  } else {
    token.kind = tkInvalid;
    return false;
  }
}

virtual bool MQTTFilter::validate() override {
  MQTTToken* ptr;

  // An empty topic string is invalid
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
bool MQTTFilter::validateToken(MQTTToken& token) {
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
  if ((hashPos > 0) || (plusPos > 0) || ((hashPos == 0) && (token.next == nullptr))) {
    token.kind = tkInvalid;
    return false;
  } 
  
  // Token is valid but set the token kind enum for special chars
  if (hashPos == 0) {
    token.kind = tkMultiLevel;
  } else if (plusPos == 0) {
    token.kind = tkSingleLevel;
  }
  
  return true;
}
