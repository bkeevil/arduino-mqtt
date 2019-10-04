#include "Arduino.h"
#include "tokenizer.h"

void runTest(bool b) {
  if (b) Serial.println("PASS"); else Serial.println("FAIL");
}

void setup() {
  Serial.begin(115200);

  MQTTTokenizer tokenizer;
  
  String s("Test1/Test2/Test3");
  tokenizer.tokenizeTopic(s);

/*
    a subscription to A/# is a subscription to the topic A and all topics beneath A
    a subscription to A/+ is a subscription to the topics directly beneath, but not A itself
    a subscription to A/+/# is a subscription to all topics beneath A, but not A itself
*/

  runTest(s.equals("Test1/Test2/Test3"));
  Serial.print("Blank Topic: ");
  runTest(!tokenizer.tokenizeTopic(""));
  Serial.print("Blank Topic Count: ");
  runTest(tokenizer.count==0);
  Serial.print("Blank Filter: ");
  runTest(!tokenizer.tokenizeFilter(""));
  Serial.print("Blank Filter Count: ");
  runTest(tokenizer.count==0);
  Serial.print("Single Slash Topic: ");
  runTest(!tokenizer.tokenizeTopic("/"));
  Serial.print("Single Slash Count: ");
  runTest(tokenizer.count==0);

  Serial.print("Single Slash Filter: ");
  runTest(!tokenizer.tokenizeFilter("/"));
  Serial.print("Single Slash Filter Count: ");
  runTest(tokenizer.count==0);
  Serial.print("Single Multi Filter: ");
  runTest(tokenizer.tokenizeFilter("#"));
  Serial.print("Single Multi Filter Count: ");
  runTest(tokenizer.count==1);
  Serial.print("Single Wildcard Filter: ");
  runTest(tokenizer.tokenizeFilter("+"));
  Serial.print("Single Wildcard Filter Count: ");
  runTest(tokenizer.count==1);
  Serial.print("Multi Slash Filter: ");
  runTest(tokenizer.tokenizeFilter("#/"));
  Serial.print("Multi Slash Filter Count: ");
  runTest(tokenizer.count==1);
  Serial.print("Slash Multi Filter: ");
  runTest(tokenizer.tokenizeFilter("/#"));
  Serial.print("Slash Multi Filter Count: ");
  runTest(tokenizer.count==2);
  Serial.print("Wildcard Slash Filter: ");
  runTest(tokenizer.tokenizeFilter("+/"));
  Serial.print("Wildcard Slash Filter: ");
  runTest(tokenizer.count==1);
  Serial.print("Slash Wildcard Filter: ");
  runTest(tokenizer.tokenizeFilter("/+"));
  Serial.print("Slash Wildcard Filter Count: ");
  runTest(tokenizer.count==2);
  
  Serial.print("Single Multi Topic: ");
  runTest(!tokenizer.tokenizeTopic("#"));
  Serial.print("Single Wildcard Topic: ");
  runTest(!tokenizer.tokenizeTopic("+"));
  Serial.print("Multi Slash Topic: ");
  runTest(!tokenizer.tokenizeTopic("#/"));
  Serial.print("Slash Multi Topic: ");
  runTest(!tokenizer.tokenizeTopic("/#"));
  Serial.print("Wildcard Slash Topic: ");
  runTest(!tokenizer.tokenizeTopic("+/"));
  Serial.print("Slash Wildcard Topic: ");
  runTest(!tokenizer.tokenizeTopic("/+"));
  
  /*runTest(tokenizer.tokenize("Test1/TestA/TestB",true));
  runTest(tokenizer.asString().equals("Test1/TestA/TestB"));
  runTest(tokenizer.tokenize("Test1/TestA/TestB",false));
  runTest(tokenizer.tokenize("Test1/+/TestB",true));
  runTest(!tokenizer.tokenize("Test1/+/TestB",false));
  runTest(!tokenizer.tokenize("Test1/TestA+/TestB",true));
  runTest(!tokenizer.tokenize("Test1/TestA+/TestB",false));
  runTest(!tokenizer.tokenize("Test1/#/TestB",true));
  runTest(!tokenizer.tokenize("Test1/#/TestB",false));
  runTest(!tokenizer.tokenize("Test1/TestA/#",true));
  runTest(tokenizer.tokenize("Test1/TestA/#",false));*/
}

void loop() {

}