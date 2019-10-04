#include "tokenizer.h"

bool MQTTTokenizer::checkTopicMatchesFilter(MQTTTokenizer& topic, MQTTTokenizer& filter) {
  int i = 0;
  bool result = false;

  MQTTToken_t* filterPtr;
  MQTTToken_t* topicPtr;

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
  MQTTToken_t* ptr = nullptr;
  
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
void MQTTTokenizer::_tokenize(String& text, MQTTToken_t* ptr) {
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
    first = (MQTTToken_t*)malloc(sizeof(MQTTToken_t));
    ptr = first;
  } else {
    //Serial.println("malloc subsequent node");
    ptr->next = (MQTTToken_t*)malloc(sizeof(MQTTToken_t));
    ptr = ptr->next;
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
bool MQTTTokenizer::validateTopicName(MQTTToken_t* token) {
  if ((token->text.length() == 0) || ((token->text.indexOf('#') == 0) && (token->text.indexOf('+') == 0))) {
    token->kind = tkValid;
    return true;
  } else {
    token->kind = tkInvalid;
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
bool MQTTTokenizer::validateTopicFilter(MQTTToken_t* token, bool isLast) {
  size_t len;
  size_t hashPos;
  size_t plusPos;

  // If token is nullptr then the token is invalid
  if (token == nullptr) return false;

  token->kind = tkValid;  // Assume the token is valid

  len = token->text.length();

  // An empty string is always valid
  if (len == 0) return true;
  
  hashPos = token->text.indexOf('#');
  plusPos = token->text.indexOf('+');

  // Any token not containing a special char is valid
  if ((hashPos == 0) && (plusPos == 0)) return true;
  
  token->kind = tkInvalid;

  // The hash character must only appear on its own
  if ((hashPos > 0) && (len != 1)) return false;
  // The hash character must only be in the last in the list of tokens
  if ((hashPos > 0) && !isLast) return false;
  // The plus character must only appear on its own
  if ((plusPos > 0) && (len != 1)) return false;

  // Token is valid but set the token kind enum for special chars
  if (hashPos == 1) {
    token->kind = tkMultiLevel;
  } else if (plusPos == 1) {
      token->kind = tkSingleLevel;
  }
  return true;
}

bool MQTTTokenizer::validateTopicName() {
  MQTTToken_t* ptr;

  // An empty topic string is invalid
  if (count == 0) return false;
  
  ptr = first;
  while (ptr != nullptr) {
    if (!validateTopicName(ptr)) return false;
    ptr = ptr->next;
  }

  return true;
}

bool MQTTTokenizer::validateTopicFilter() {
  int i=0;
  MQTTToken_t* ptr;

  // An empty topic string is invalid
  if (count == 0) return false;
  
  ptr = first;
  while (ptr != nullptr) {
    if (!validateTopicFilter(ptr,i==count-1)) return false;
    ++i;
    ptr = ptr->next;
  }

  return true;
}

String MQTTTokenizer::asString() const {
  int i=0;
  MQTTToken_t* ptr;
  String s;

  ptr = first;
  while (ptr != nullptr) {
    switch (ptr->kind) {
      case tkSingleLevel: s.concat('+'); break;
      case tkMultiLevel: s.concat('#'); break;
      case tkValid: s.concat(ptr->text); break;
    }
    if (i < count - 1) 
      s.concat('/');
    ++i;
    ptr = ptr->next;
  }
  return s;
}

void MQTTTokenizer::clear() {
  MQTTToken_t* ptr;
  //Serial.print("first="); Serial.println(int(first),HEX);
  while (first != nullptr) {
    ptr = first->next;
    //Serial.println("Free node");
    free(first);
    first = ptr; 
  }

  first = nullptr;
  count = 0;
}
