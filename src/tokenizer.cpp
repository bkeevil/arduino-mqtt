#include "tokenizer.h"

bool MQTTTokenizer::checkTopicMatchesFilter(MQTTTokenizer& topic, MQTTTokenizer& filter) {
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

/** @brief  Parse a topic name or topic filter into tokens and validate those tokens
 *  @param  text  The topic or filter sting to Parse
 *  @param  isFilter  Set to true to parse as a topic filter, or false to parse as a topic name
 *  @remark A copy of text has to be made because it is modified by the tokenizer
 *  @return True if the topic name or filter string was parsed and was valid
 */
bool MQTTTokenizer::tokenize(String text, bool isFilter) {
  MQTTToken* ptr = nullptr;
  
  clear();
  
  //Serial.println("before _tokenize");
  
  // Remove any trailing / char
  if (text.lastIndexOf('/') == text.length() - 1) {
      text = text.substring(0,text.length() - 1);
  }

  if (text.length() > 0) {
    _tokenize(text, ptr);
  } else {
    return false;
  }
  //Serial.println("before validate");
  if (isFilter) {
    return validateTopicFilter();
  } else {
    return validateTopicName();
  }
}

/** @brief  Recursively tokenize text and add the tokens to the linked list.
 *  @remark Called by tokenize()
 */
void MQTTTokenizer::_tokenize(String& text, MQTTToken* ptr) {
  String token;

  //Serial.print("  Entry text="); Serial.println(text);
  int pos = text.indexOf('/');
  if (pos < 0) {
    token = text;
    text = "";
  } else {
    token = text.substring(0,pos);
    text = text.substring(pos+1);
  }
  //Serial.print("  Next text="); Serial.println(text);
  //Serial.print("  token="); Serial.println(token);

  if (ptr == nullptr) {
    //Serial.println("malloc first node");
    first = new MQTTToken;
    ptr = first;
    ptr->next = nullptr;
  } else {
    //Serial.println("malloc subsequent node");
    ptr->next = new MQTTToken;
    ptr = ptr->next;
    ptr->next = nullptr;
  }  
  
  ptr->text = token;
  ptr->next = nullptr;
  ++count;
  //Serial.print("  count="); Serial.println(count);
  //Serial.print("  pos="); Serial.println(pos);
  //Serial.print("  length="); Serial.println(text.length());
  if ((pos > 0) || (text.length() > 0)) 
    _tokenize(text,ptr);  
}

/** @brief    Validates a token as a topic name. 
 *            Sets the token kind to tkValid or tkInvalid.
 *  @param    token   The token to validate
 *  @remark   Empty string is valid
 *  @remark   Any token containing a hash or a plus is invalid
 *  @returns  True if the token is valid 
 */ 
bool MQTTToken::validateTopicName() {
  if ((text.length() == 0) || ((text.indexOf('#') == -1) && (text.indexOf('+') == -1))) {
    kind = tkValid;
    return true;
  } else {
    kind = tkInvalid;
    return false;
  }
}

/** @brief    Validates a token as a topic name. 
 *            Sets the token kind to tkValid or tkInvalid.
 *  @param    token   The token to validate
 *  @param    isLast  Set to True if this is the last token in the filter string
 *  @remark   An empty string is always valid
 *  @remark   Any token not containing a special char is valid
 *  @remark   The hash character must only appear on its own
 *  @remark   The hash character must only be in the last in the list of tokens
 *  @remark   The plus character must only appear on its own
 *  @returns  True if the token is valid 
 */ 
bool MQTTToken::validateTopicFilter(bool isLast) {
  size_t len;
  int hashPos;
  int plusPos;
  
  // An empty string is always valid
  len = text.length();
  if (len == 0) {
    kind = tkValid;
    return true;
  }

  hashPos = text.indexOf('#');
  plusPos = text.indexOf('+');

  // Any token not containing a special char is valid
  if ((hashPos == -1) && (plusPos == -1)) {
    kind = tkValid;
    return true;
  }

  // The hash and plus character must only appear on their own
  // The hash character must only be in the last in the list of tokens
  if ((hashPos > 0) || (plusPos > 0) || ((hashPos == 0) && !isLast)) {
    kind = tkInvalid;
    return false;
  } 
  
  // Token is valid but set the token kind enum for special chars
  if (hashPos == 0) {
    kind = tkMultiLevel;
  } else if (plusPos == 0) {
    kind = tkSingleLevel;
  }
  
  return true;
}

bool MQTTTokenizer::validateTopicName() {
  MQTTToken* ptr;

  // An empty topic string is invalid
  if (count == 0) return false;
  
  ptr = first;
  while (ptr != nullptr) {
    if (!ptr->validateTopicName()) {
      clear();
      return false;
    }
    ptr = ptr->next;
  }

  return true;
}

bool MQTTTokenizer::validateTopicFilter() {
  int i=0;
  MQTTToken* ptr;

  // An empty topic string is invalid
  if (count == 0) {
    return false;
  }

  ptr = first;
  while (ptr != nullptr) {
    if (!ptr->validateTopicFilter(i==count-1)) {
      clear();
      return false;
    }
    ++i;
    ptr = ptr->next;
  }

  return true;
}

String& MQTTTokenizer::asString(String& s) const {
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
