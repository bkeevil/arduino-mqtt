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
  
  Serial.print("Three Token Topic: ");
  runTest(tokenizer.tokenizeTopic("Test1/TestA/TestB"));
  Serial.print("Check asString matches: ");
  runTest(tokenizer.asString(s).equals("Test1/TestA/TestB"));
  Serial.print("Three Token Topic Count: ");
  runTest(tokenizer.count == 3);

  Serial.print("Three Token Filter: ");
  runTest(tokenizer.tokenizeFilter("Test1/TestA/TestB"));
  Serial.print("Check asString matches: ");
  runTest(tokenizer.asString(s).equals("Test1/TestA/TestB"));
  Serial.print("Three Token Filter Count: ");
  runTest(tokenizer.count == 3);
  
  Serial.print("Wildcard in topic: ");
  runTest(!tokenizer.tokenizeTopic("Test1/+/TestB"));
  Serial.print("Wildcard in filter: ");
  runTest(tokenizer.tokenizeFilter("Test1/+/TestB"));
  Serial.print("Wildcard in Filter Count: ");
  runTest(tokenizer.count == 3);

  Serial.print("Wildcard as part of topic token: ");
  runTest(!tokenizer.tokenizeTopic("Test1/TestA+/TestB"));
  Serial.print("Wildcard as part of filter token: ");
  runTest(!tokenizer.tokenizeFilter("Test1/TestA+/TestB"));
  
  Serial.print("Multi as part of topic token: ");
  runTest(!tokenizer.tokenizeTopic("Test1/TestA#/TestB"));
  Serial.print("Multi as part of filter token: ");
  runTest(!tokenizer.tokenizeFilter("Test1/TestA#/TestB"));
  
  Serial.print("Multi in wrong place in topic: ");
  runTest(!tokenizer.tokenizeTopic("Test1/#/TestB"));
  Serial.print("Multi in wrong place in filter: ");
  runTest(!tokenizer.tokenizeFilter("Test1/#/TestB"));

  Serial.print("Multi in right place in topic: ");
  runTest(!tokenizer.tokenizeTopic("Test1/TestA/#"));
  Serial.print("Multi in right place in filter: ");
  runTest(tokenizer.tokenizeFilter("Test1/TestA/#"));
  Serial.print("Multi in right place token count: ");
  runTest(tokenizer.count == 3);

  Serial.print("Wildcard at end in topic: ");
  runTest(!tokenizer.tokenizeTopic("Test1/TestA/+"));
  Serial.print("Wildcard at end in filter: ");
  runTest(tokenizer.tokenizeFilter("Test1/TestA/+"));

  Serial.print("Wildcard Multi at end in topic: ");
  runTest(tokenizer.tokenizeFilter("Test1/TestA/+/#"));
  
  /*
    a subscription to A/# is a subscription to the topic A and all topics beneath A
    a subscription to A/+ is a subscription to the topics directly beneath, but not A itself
    a subscription to A/+/# is a subscription to all topics beneath A, but not A itself
  */
}

void loop() {

}