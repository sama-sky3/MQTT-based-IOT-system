#include "Secrets.h"
#include <ESP32Servo.h>
#include <LiquidCrystal.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// lcd and ir and servo setup
LiquidCrystal lcd(13, 12, 14, 27, 26, 25);
Servo myservo;

#define ir_sensor 33
int ir_sensor_value = 0;

#define AWS_IOT_PUBLISH_TOPIC "sensor/ir_data"
#define AWS_IOT_SUBSCRIBE_TOPIC  "actuator/servo_control"
#define WIFI_SSID "********"
#define WIFI_PASSWORD "********"

// Secure WiFi client for TLS
WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);

void connectAWS() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WIFI Connected");
  
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  espClient.setCACert(AWS_CERT_CA);
  espClient.setCertificate(AWS_CERT_CRT);
  espClient.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  mqttClient.setServer(AWS_IOT_ENDPOINT, 8883);

  // Create a message handler
  mqttClient.setCallback(messageHandler);

  Serial.println("Connecting to AWS IOT");

  while (!mqttClient.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if (!mqttClient.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  mqttClient.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

void publishMessage() {
  StaticJsonDocument<200> doc;
  doc["ir"] = ir_sensor_value;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);  // print to client

  mqttClient.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  if (doc.containsKey("angle")) {
    int angle = doc["angle"];
    
    Serial.print("Moving servo to: ");
    Serial.println(angle);
    
    lcd.setCursor(0, 1);
    lcd.print("Angle: ");
    lcd.print(angle);
    lcd.print((char)223);  // Degree symbol
    
    myservo.write(angle);
  }
}



void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.print("Starting...");
  pinMode(ir_sensor, INPUT);
  myservo.attach(32);
  delay(1000);
  lcd.clear();

  // Allow insecure TLS (for testing only)
  espClient.setInsecure();

  //connect to AWS
  connectAWS();
  
}
void pub_irSensor() {
  ir_sensor_value = digitalRead(ir_sensor);
  Serial.print(ir_sensor_value);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  if (ir_sensor_value == LOW) {
    lcd.print("Object Detected");
  } else {
    lcd.print("No Object");
  }
  
  publishMessage();
  mqttClient.loop();
  delay(1000);
}

void loop() {
pub_irSensor();
mqttClient.loop(); // Required to keep receiving MQTT messages

}

