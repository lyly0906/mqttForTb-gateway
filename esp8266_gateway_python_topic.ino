#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#define WIFI_AP "ybh_001" //"张小欣的iphone"
#define WIFI_PASSWORD "y06100610" //".1234567"

#define TOKEN "ESP8266_DEMO_TOKEN"

#define GPIO0 0
#define GPIO2 2

#define GPIO0_PIN 3
#define GPIO2_PIN 5
int Status = 12;  // Digital pin D6 ring
int port = A0;//连接火焰传感器
int sensor = 13;  // Digital pin D7  LED
int sum = 0;

int timeOut = 0;
int flag = 0;
int value = 0;//测量的模拟数值
bool switch_state = false;
int stats = 0;
int loopNum = 0;
int flagNum = 0;

char thingsboardServer[] = "39.105.185.205";

WiFiClient wifiClient;

PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;

// We assume that all GPIOs are LOW
boolean gpioState[] = {false, false};

void setup() {
Serial.begin(115200);
// Set output mode for all GPIO pins
pinMode(GPIO0, OUTPUT);
pinMode(GPIO2, OUTPUT);
pinMode(Status, OUTPUT);  // declare LED as output
pinMode(sensor, OUTPUT);   // LED status
digitalWrite(sensor, HIGH);
delay(10);
InitWiFi();
client.setServer( thingsboardServer, 1883 );
client.setCallback(on_message);
}

void loop() {
if(sum >= 20000){
  Serial.println(sum);

  value = analogRead(port);
  Serial.println(value);

  if(value < 100){
    flag = 1;
    timeOut=0;
    
    digitalWrite(sensor, HIGH);
    digitalWrite (Status, HIGH);
    if(flagNum<1){
      client.publish("/sensor/data/esp8266_fire", get_gpio_status(1).c_str());
    }
    if(loopNum>20){
      client.publish("/sensor/data/esp8266_fire", get_gpio_status(1).c_str());
      loopNum=0;
    }    
    flagNum++;
  }else{
    if(flag==1){
      flagNum = 0;
      Serial.println(timeOut);
      if(timeOut > 5){
        switch_state = 0;
        Serial.println("------------------");
        digitalWrite(sensor, LOW);
        digitalWrite (Status, LOW);
        client.publish("/sensor/data/esp8266_fire", get_gpio_status(0).c_str());
        flag=0;
        timeOut=0;
        flagNum=0;
        }else{
          digitalWrite(sensor, HIGH);
          digitalWrite (Status, HIGH);
          if(flagNum<1){
            client.publish("/sensor/data/esp8266_fire", get_gpio_status(1).c_str()); 
          }
          flagNum++; 
        }
              
        timeOut++;
    }else{
        //digitalWrite(sensor, LOW);
        digitalWrite (Status, LOW);
        flagNum=0;
        if(loopNum>10){
          client.publish("/sensor/data/esp8266_fire", get_gpio_status(0).c_str());
          loopNum=0;
        }
    }

    if(stats){
      digitalWrite(sensor,HIGH);
      //digitalWrite (Status, HIGH);
      delay(100);
      digitalWrite(sensor,LOW);
      //digitalWrite (Status, LOW);
    }else{
      digitalWrite(sensor,HIGH);
    }
  sum=0;
    
  }
  loopNum++;
}
//Serial.println(sum);
sum++;
if ( !client.connected() ) {
reconnect();
}

client.loop();
}

// The callback for when a PUBLISH message is received from the server.
void on_message(const char* topic, byte* payload, unsigned int length) {

Serial.println("On message");
Serial.println(length);
char json[length+1];
strncpy (json, (char*)payload, length);
json[length] = '\0';

Serial.print("Topic: ");
Serial.println(topic);
Serial.print("Message: ");
Serial.println(String(json));

// Decode JSON request
StaticJsonDocument<200> jsonBuffer;
auto error = deserializeJson(jsonBuffer, (char*)json);
  
if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
}
// Check request method
String methodName = String((const char*)jsonBuffer["method"]);
int status1 = int(jsonBuffer["status"]);
Serial.println(status1);
if(status1 == 1){
  //digitalWrite (Status, HIGH);
  switch_state = 1;
  stats = 1;
}else{
  //digitalWrite (Status, LOW);
  switch_state = 0;
  stats = 0;
}
Serial.println("methodName");
Serial.println(methodName);
if (methodName.equals("getGpioStatus")) {
// Reply with GPIO status
String responseTopic = String(topic);
responseTopic.replace("request", "response");
client.publish(responseTopic.c_str(), get_gpio_status(0).c_str());
} else if (methodName.equals("setGpioStatus")) {
// Update GPIO status and reply
set_gpio_status(jsonBuffer["params"]["pin"], jsonBuffer["params"]["enabled"]);
String responseTopic = String(topic);
responseTopic.replace("request", "response");
client.publish(responseTopic.c_str(), get_gpio_status(0).c_str());
//client.publish("/sensor/data/esp8266_fire", get_gpio_status(0).c_str());
//client.publish("v1/devices/me/attributes", get_gpio_status(0).c_str());
}
}

String get_gpio_status(int stat) {
// Prepare gpios JSON payload string
StaticJsonDocument<200> jsonBuffer;

jsonBuffer["serialNumber"] = "esp8266_fire";
jsonBuffer["sensorType"] = "esp8266";
jsonBuffer["fire"] = stat;
jsonBuffer["status"] = stats;
char payload[256];
serializeJson(jsonBuffer, payload);
String strPayload = String(payload);
Serial.print("Get gpio status: ");
Serial.println(strPayload);
return strPayload;
}

String get_gpio_status1(int stat) {
// Prepare gpios JSON payload string
StaticJsonDocument<200> jsonBuffer;

jsonBuffer["serialNumber"] = "esp8266_fire";
jsonBuffer["sensorType"] = "esp8266";
jsonBuffer["fire"] = stat;
jsonBuffer["status"] = stats;
char payload[256];
serializeJson(jsonBuffer, payload);
String strPayload = String(payload);
Serial.print("Get gpio status: ");
Serial.println(strPayload);
return strPayload;
}

void set_gpio_status(int pin, boolean enabled) {
if (pin == GPIO0_PIN) {
// Output GPIOs state
digitalWrite(GPIO0, enabled ? HIGH : LOW);
// Update GPIOs state
gpioState[0] = enabled;
} else if (pin == GPIO2_PIN) {
// Output GPIOs state
digitalWrite(GPIO2, enabled ? HIGH : LOW);
// Update GPIOs state
gpioState[1] = enabled;
}
}

void InitWiFi() {
Serial.println("Connecting to AP ...");
// attempt to connect to WiFi network

WiFi.begin(WIFI_AP, WIFI_PASSWORD);
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}
Serial.println("Connected to AP");
}


void reconnect() {
// Loop until we're reconnected
while (!client.connected()) {
status = WiFi.status();
if ( status != WL_CONNECTED) {
WiFi.begin(WIFI_AP, WIFI_PASSWORD);
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}
Serial.println("Connected to AP");
}
Serial.print("Connecting to Thingsboard node ...");
// Attempt to connect (clientId, username, password)
if ( client.connect("ESP8266 Device", TOKEN, NULL) ) {
Serial.println( "[DONE]" );
// Subscribing to receive RPC requests
client.subscribe("sensor/esp8266_fire/request/echoEsp8266/+");
// Sending current GPIO status
Serial.println("Sending current GPIO status ...");
client.publish("/sensor/data/esp8266_fire", get_gpio_status(0).c_str());
} else {
Serial.print( "[FAILED] [ rc = " );
Serial.print( client.state() );
Serial.println( " : retrying in 5 seconds]" );
// Wait 5 seconds before retrying
delay( 5000 );
}
}
}
