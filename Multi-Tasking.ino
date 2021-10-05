#include <WiFi.h>
#include <PubSubClient.h>


TaskHandle_t t0;
TaskHandle_t t1;

const char* ssid = "CPE61";
const char* password = "61037029";
const char* mqtt_server = "---";//can't connecting

#define mqtt_port 1883

WiFiClient espClient;
PubSubClient *client = NULL; 

long lastMsg = 0;
char msg[50];
int value = 0;

long lastReconnectAttempt = 0;
String STATE = "None";

void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  
  
  xTaskCreatePinnedToCore(
    tLedAndSerialFunc,  
    "LedAndSerial",     
    10000,              
    NULL,               
    1,                  
    &t1,                
    0);                 
  delay(1000);
  
  
  xTaskCreatePinnedToCore(
    tNetworkFunc,       
    "Network",          
    10000,              
    NULL,               
    1,                  
    &t0,                
    1);                 
  delay(1000);
}
boolean reconnect() {
  if (client->connect("arduinoClient")) {
    Serial.println("MQTT connected");
    client->publish("device/outTopic", "Hi");
    client->subscribe("device/inTopic");
  }
  return client->connected();
}

void loop() {
  
}

void tNetworkFunc(void *params)
 {
  Serial.print("tNetworkFunc running on core ");
  Serial.println(xPortGetCoreID());
  setup_wifi();
  
  
  while (true) {
    if (WiFi.status() != WL_CONNECTED) { 
      STATE = "None";
    } else if (WiFi.status() == WL_CONNECTED && STATE == "None") {\
     
      Serial.println("WiFi connected");
      STATE = "WiFi-Connect";
      client = new PubSubClient(espClient);
      client->setServer(mqtt_server, mqtt_port);
      client->setCallback(callback);
    } else if (WiFi.status() != WL_CONNECTED && STATE == "MQTT-Connect") {
     
      delete client;
      STATE = "None";
    } else { 
      if (!client->connected()) {
        long now = millis();
        if (now - lastReconnectAttempt > 5000) {
          lastReconnectAttempt = now;
         
          if (reconnect()) {
            lastReconnectAttempt = 0;
          }
        }
        STATE = "WiFi-Connect";
      } else {
       
        STATE = "MQTT-Connect";
        client->loop();

        long now = millis();
        if (now - lastMsg > 2000) {
          lastMsg = now;
          ++value;
          snprintf (msg, 50, "hello world #%ld", value);
          Serial.print("Publish message: ");
          Serial.println(msg);
          client->publish("device/outTopic", msg);
        }
      }
    }
    delay(10);
  }
}

void tLedAndSerialFunc(void *params) 
{
 
  Serial.print("tLedAndSerialFunc running on core ");
  Serial.println(xPortGetCoreID());
  Serial.println((String)"Counter is reset(" + Counter + ")");
  unsigned int Counter = 0;
  uint32_t _nextSerial = millis() + 1000;
  uint32_t _nextLED = millis() + 500;

  
  while (true) {
    uint32_t cur = millis();
    if (cur >= _nextSerial) {
      Serial.println((String)"Counter is changed to " + Counter++);
      _nextSerial = cur + 1000;
    }

    if (cur >= _nextLED) {
      if (STATE == "WiFi-Connect") {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      } else if (STATE == "MQTT-Connect") {
        digitalWrite(LED_BUILTIN, LOW);
      } else if (STATE == "None") {
        digitalWrite(LED_BUILTIN, HIGH); 
      }
      _nextLED = cur + 500;
    }
    delay(1000);
  }
}