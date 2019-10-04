#include "tokenizer.h"

bool MQTTFilter::match(const MQTTTopic& topic) const {
  int i = 0;
  bool result = false;

  MQTTToken* filterPtr;
  MQTTToken* topicPtr;
  String s;

  filterPtr = first();
  topicPtr  = topic.first();

  //Serial.print("  filter="); Serial.println(getString(s));
  //Serial.print("  topic="); Serial.println(topic.getString(s));
  
  while (filterPtr != nullptr) {
    if (i >= topic.count()) {
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
  
  if (count() < topic.count()) {
    // Retrieve the count - 1 token from the filter list
    i = 0;
    filterPtr = first();
    while ((filterPtr != nullptr) && (i < count() - 1)) {
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

  String s;
  //Serial.print("  getString="); Serial.println(getString(s));
}

/** @brief  Recursively tokenize text and add the tokens to the linked list.
 *  @remark Called by tokenize()
 */
void MQTTTokenizer::_tokenize(String text, MQTTToken* ptr) {
  MQTTToken* node = new MQTTToken;

  //Serial.print("  text="); Serial.println(text);

  if (ptr == nullptr) {
    //Serial.println("Add first node");
    first_ = node;
  } else {
    //Serial.println("Add subsequent node");
    ptr->next = node;
  }
      
  ++count_;
  
  int pos = text.indexOf('/');
  //Serial.print("  pos="); Serial.println(pos);
  if (pos < 0) {
    node->text = text;
    //Serial.print("  token="); Serial.println(node->text);
  } else {
    node->text = text.substring(0,pos);
    //Serial.print("  token="); Serial.println(node->text);
    if (pos < text.length() - 1) {
      //Serial.print(" _tokenize() "); Serial.println(text.substring(pos+1));
      _tokenize(text.substring(pos+1),node); 
    }
  }

  //Serial.print("  count="); Serial.println(count_);
}

String& MQTTTokenizer::getString(String& s) const {
  int i=0;
  int size=0;
  MQTTToken* ptr;

  s = "";
  //Serial.println("getString");
  ptr = first_;
  while (ptr != nullptr) {
    //Serial.print("  kind: "); Serial.println(ptr->kind);
    switch (ptr->kind) {
      case tkSingleLevel: ++size; break;
      case tkMultiLevel: ++size; break;
      case tkValid: size += ptr->text.length(); break;
    }    
    //Serial.print("  text: "); Serial.println(ptr->text); 
    ++size;
    ptr = ptr->next;
  }

  //Serial.print("  size: "); Serial.println(size-1);

  s.reserve(size-1);

  ptr = first_;
  while (ptr != nullptr) {
  
    switch (ptr->kind) {
      case tkSingleLevel: s += '+'; break;
      case tkMultiLevel: s += '#'; break;
      case tkValid: s += ptr->text; break;
    }
    if (i < count_ - 1) {
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
  while (first_ != nullptr) {
    ptr = first_->next;
    delete first_;
    first_ = ptr; 
  }
  first_ = nullptr;
  count_ = 0;
}

bool MQTTTopic::validate() {
  MQTTToken* ptr;

  //Serial.println("Topic Validate");
  
  // An empty topic string is invalid
  if (count() == 0) return false;
  
  ptr = first();
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

bool MQTTFilter::validate() {
  MQTTToken* ptr;

  //Serial.println("Filter Validate");
  
  // An empty topic string is invalid
  if (first() == nullptr) {
    return false;
  }

  ptr = first();
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

  //Serial.print("  token="); Serial.println(token.text);
  //Serial.print("  hashPos="); Serial.println(hashPos);
  //Serial.print("  plusPos="); Serial.println(plusPos);
  
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
  
  //Serial.print("  kind="); Serial.println(token.kind);

  return true;
}
