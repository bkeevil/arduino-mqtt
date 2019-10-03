#include "Arduino.h"
#include "tokenizer.h"

void runTest(bool b) {
  if (b) Serial.println("PASS"); else Serial.println("FAIL");
}

void setup() {
  Serial.begin(115200);

  MQTTTokenizer tokenizer;

  runTest(!tokenizer.tokenize("",false));
  runTest(tokenizer.count==1);
  runTest(!tokenizer.tokenize("",true));
  runTest(tokenizer.count==1);
  runTest(tokenizer.tokenize("/",true));
  runTest(tokenizer.count==2);
  runTest(tokenizer.tokenize("/",false));
  runTest(tokenizer.count==2);
  runTest(tokenizer.tokenize("#",true));
  runTest(tokenizer.tokenize("+",true));
  runTest(!tokenizer.tokenize("#/",true));
  runTest(tokenizer.tokenize("/#",true));
  runTest(tokenizer.tokenize("+/",true));
  runTest(tokenizer.tokenize("/+",true));
  runTest(!tokenizer.tokenize("#",false));
  runTest(!tokenizer.tokenize("+",false));
  runTest(!tokenizer.tokenize("#/",false));
  runTest(!tokenizer.tokenize("/#",false));
  runTest(!tokenizer.tokenize("+/",false));
  runTest(!tokenizer.tokenize("/+",false));
  runTest(tokenizer.tokenize("Test1/TestA/TestB",true));
  runTest(tokenizer.asString().equals("Test1/TestA/TestB"));
  runTest(tokenizer.tokenize("Test1/TestA/TestB",false));
  runTest(tokenizer.tokenize("Test1/+/TestB",true));
  runTest(!tokenizer.tokenize("Test1/+/TestB",false));
  runTest(!tokenizer.tokenize("Test1/TestA+/TestB",true));
  runTest(!tokenizer.tokenize("Test1/TestA+/TestB",false));
  runTest(!tokenizer.tokenize("Test1/#/TestB",true));
  runTest(!tokenizer.tokenize("Test1/#/TestB",false));
  runTest(!tokenizer.tokenize("Test1/TestA/#",true));
  runTest(tokenizer.tokenize("Test1/TestA/#",false));
}

void loop() {

}