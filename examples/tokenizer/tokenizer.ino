/** @file     tokenizer.ino
 *  @brief    A test driver for MQTTTokenizer, MQTTTopic, and MQTTFilter classes of my mqtt package.
 *  @author   Bond Keevil
 *  @date     2019
 *  @remarks  Should run on any ESP32 module
 */

#include "Arduino.h"
#include "mqtt.h"
#include "esp_heap_caps.h"


void runTest(bool b) {
  if (b) Serial.println("PASS"); else Serial.println("FAIL");
}

void testTopic() {
  MQTTTopic topic;
  String s;

  Serial.print("Blank Topic: ");
  runTest(!topic.setString(""));
  Serial.print("Blank Topic Count: ");
  runTest(topic.count==0);
  Serial.print("Single Slash Topic: ");
  runTest(!topic.setString("/"));
  Serial.print("Single Slash Count: ");
  runTest(topic.count==0);
  Serial.print("Single Multi Topic: ");
  runTest(!topic.setString("#"));
  Serial.print("Single Wildcard Topic: ");
  runTest(!topic.setString("+"));
  Serial.print("Multi Slash Topic: ");
  runTest(!topic.setString("#/"));
  Serial.print("Slash Multi Topic: ");
  runTest(!topic.setString("/#"));
  Serial.print("Wildcard Slash Topic: ");
  runTest(!topic.setString("+/"));
  Serial.print("Slash Wildcard Topic: ");
  runTest(!topic.setString("/+"));  
  Serial.print("Three Token Topic: ");
  runTest(topic.setString("Test1/TestA/TestB"));
  Serial.print("Check asString matches: ");
  runTest(topic.getString(s).equals("Test1/TestA/TestB"));
  Serial.print("Three Token Topic Count: ");
  runTest(topic.count == 3);
  Serial.print("Wildcard in topic: ");
  runTest(!topic.setString("Test1/+/TestB"));
  Serial.print("Wildcard as part of topic token: ");
  runTest(!topic.setString("Test1/TestA+/TestB"));
  Serial.print("Multi as part of topic token: ");
  runTest(!topic.setString("Test1/TestA#/TestB"));  
  Serial.print("Multi in wrong place in topic: ");
  runTest(!topic.setString("Test1/#/TestB"));
  Serial.print("Multi in right place in topic: ");
  runTest(!topic.setString("Test1/TestA/#"));
  Serial.print("Wildcard at end in topic: ");
  runTest(!topic.setString("Test1/TestA/+"));
}

void testFilter() {
  MQTTFilter filter;
  String s;
  
  Serial.print("Blank Filter: ");
  runTest(!filter.setString(""));
  Serial.print("Blank Filter Count: ");
  runTest(filter.count==0);
  Serial.print("Single Slash Filter: ");
  runTest(!filter.setString("/"));
  Serial.print("Single Slash Filter Count: ");
  runTest(filter.count==0);
  Serial.print("Single Multi Filter: ");
  runTest(filter.setString("#"));
  Serial.print("Single Multi Filter Count: ");
  runTest(filter.count==1);
  Serial.print("Single Wildcard Filter: ");
  runTest(filter.setString("+"));
  Serial.print("Single Wildcard Filter Count: ");
  runTest(filter.count==1);
  Serial.print("Multi Slash Filter: ");
  runTest(filter.setString("#/"));
  Serial.print("Multi Slash Filter Count: ");
  runTest(filter.count==1);
  Serial.print("Slash Multi Filter: ");
  runTest(filter.setString("/#"));
  Serial.print("Slash Multi Filter Count: ");
  runTest(filter.count==2);
  Serial.print("Wildcard Slash Filter: ");
  runTest(filter.setString("+/"));
  Serial.print("Wildcard Slash Filter: ");
  runTest(filter.count==1);
  Serial.print("Slash Wildcard Filter: ");
  runTest(filter.setString("/+"));
  Serial.print("Slash Wildcard Filter Count: ");
  runTest(filter.count==2);
  Serial.print("Three Token Filter: ");
  runTest(filter.setString("Test1/TestA/TestB"));
  Serial.print("Check asString matches: ");
  runTest(filter.getString(s).equals("Test1/TestA/TestB"));
  Serial.print("Three Token Filter Count: ");
  runTest(filter.count == 3);
  Serial.print("Wildcard in filter: ");
  runTest(filter.setString("Test1/+/TestB"));
  Serial.print("Wildcard in Filter Count: ");
  runTest(filter.count == 3);
  Serial.print("Wildcard as part of filter token: ");
  runTest(!filter.setString("Test1/TestA+/TestB"));
  Serial.print("Multi as part of filter token: ");
  runTest(!filter.setString("Test1/TestA#/TestB"));
  Serial.print("Multi in wrong place in filter: ");
  runTest(!filter.setString("Test1/#/TestB"));
  Serial.print("Multi in right place in filter: ");
  runTest(filter.setString("Test1/TestA/#"));
  Serial.print("Multi in right place token count: ");
  runTest(filter.count == 3);
  Serial.print("Wildcard at end in filter: ");
  runTest(filter.setString("Test1/TestA/+"));
  Serial.print("Wildcard Multi at end in topic: ");
  runTest(filter.setString("Test1/TestA/+/#"));
}

void testGroup3() {
  Serial.println("A subscription to A/# is a subscription to the topic A and all topics beneath A");
  MQTTFilter filter("A/#");
  MQTTTopic topic;

  topic.setString("A");
  runTest(filter.match(topic));
  topic.setString("A/");
  runTest(filter.match(topic));
  topic.setString("A/B");
  runTest(filter.match(topic));
  topic.setString("B");
  runTest(!filter.match(topic));
  topic.setString("A/Test1/Test2");
  runTest(filter.match(topic));
}

void testGroup4() {
  Serial.println("A subscription to A/+ is a subscription to the topics directly beneath, but not A itself");
  MQTTFilter filter("A/+");
  MQTTTopic topic;

  topic.setString("A");
  runTest(!filter.match(topic));
  topic.setString("A/");
  runTest(!filter.match(topic));
  topic.setString("A/B");
  runTest(filter.match(topic));
  topic.setString("B");
  runTest(!filter.match(topic));
  topic.setString("A/Test1/Test2");
  runTest(!filter.match(topic));

}

void testGroup5() {
  Serial.println("subscription to A/+/# is a subscription to all topics beneath A, but not A itself");
  MQTTFilter filter("A/+/#");
  MQTTTopic topic;

  topic.setString("A");
  runTest(!filter.match(topic));
  topic.setString("A/");
  runTest(!filter.match(topic));
  topic.setString("A/B");
  runTest(filter.match(topic));
  topic.setString("B");
  runTest(!filter.match(topic));
  topic.setString("A/Test1/Test2");
  runTest(filter.match(topic));
}

void setup() {
  Serial.begin(115200);

  Serial.print("Free Heap Memory: "); Serial.println(heap_caps_get_free_size(MALLOC_CAP_8BIT)); 
  testTopic();
  testFilter();
  testGroup3();
  testGroup4();
  testGroup5();
  Serial.print("Free Heap Memory: "); Serial.println(heap_caps_get_free_size(MALLOC_CAP_8BIT)); 

}

void loop() {

}