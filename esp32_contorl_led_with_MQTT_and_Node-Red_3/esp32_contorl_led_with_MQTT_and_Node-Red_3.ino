#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// Update these with values suitable for your network.
const char* ssid = "Idea";
const char* password = "12345678";

// Config MQTT Server
#define mqtt_server "192.168.43.54"
#define mqtt_port 1883
#define mqtt_user "TEST"
#define mqtt_password "12345"

#define LED_PIN 2
const int LED_PIN1 = 14;

#define DHTTYPE DHT22
const int DHTPin = 4;
DHT dht(DHTPin, DHTTYPE);

static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long currentTime = millis();
const long delayTime = 3000;

//const int ledPin = 16;  // 16 corresponds to GPIO16

// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

void setup() {
  dht.begin();
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_PIN1, OUTPUT);

  
  ledcSetup(ledChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(LED_PIN1, ledChannel);

  
  Serial.begin(115200);
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("/ESP/LED");
      client.subscribe("/ESP/LED1");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      return;
    }
  }
  client.loop();
  dhtread();
}

boolean isNumeric(String str) {
    unsigned int stringLength = str.length();
 
    if (stringLength == 0) {
        return false;
    }
 
    boolean seenDecimal = false;
 
    for(unsigned int i = 0; i < stringLength; ++i) {
        if (isDigit(str.charAt(i))) {
            continue;
        }
 
        if (str.charAt(i) == '.') {
            if (seenDecimal) {
                return false;
            }
            seenDecimal = true;
            continue;
        }
        return false;
    }
    return true;
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String msg = "";
  int i=0;
  while (i<length) msg += (char)payload[i++];
  if (msg == "GET") {
    client.publish("/ESP/LED", (digitalRead(LED_PIN) ? "LEDON" : "LEDOFF"));
    Serial.println("Send !");
    return;
  }
  if (msg == "on") {
    client.publish("/ESP/LED", "state-on");
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Send on !");
    return;
  }
  if (msg == "off") {
    client.publish("/ESP/LED", "state-off");
    digitalWrite(LED_PIN, LOW);
    Serial.println("Send off !");
    return;
  }

//ledcWrite(ledChannel, dutyCycle); 
Serial.println(topic);
  if (isNumeric(msg)  ) {
//  if (topic == "/ESP/LED1" ) {

//    client.publish("/ESP/LED", "state-off");
    ledcWrite(ledChannel, msg.toInt());
    Serial.println("LED = "+msg);
    return;
  }
  
  //digitalWrite(LED_PIN, (msg == "LEDON" ? HIGH : LOW));
 // Serial.println(msg);
}
void dhtread() {

  if (millis() - currentTime > delayTime) {
    currentTime = millis();
   
    Serial.print("Millis time =  ");
    Serial.println(millis());
    Serial.print("currenttime =  ");
    Serial.println(currentTime);

    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      strcpy(celsiusTemp, "Failed");
      strcpy(fahrenheitTemp, "Failed");
      strcpy(humidityTemp, "Failed");
    }
    else {
      // Computes temperature values in Celsius + Fahrenheit and Humidity
      float hic = dht.computeHeatIndex(t, h, false);
      dtostrf(hic, 6, 2, celsiusTemp);
      float hif = dht.computeHeatIndex(f, h);
      dtostrf(hif, 6, 2, fahrenheitTemp);
      dtostrf(h, 6, 2, humidityTemp);
      // You can delete the following Serial.prints, it s just for debugging purposes
    }


    client.publish("room/temperature", celsiusTemp);
    client.publish("room/fahrenheit", fahrenheitTemp);
    client.publish("room/humidity", humidityTemp);

  }
}
