#include <WiFi.h> // Library untuk fungsi WiFi pada ESP32
#include <PubSubClient.h> // Library untuk fungsi MQTT

#define pinSensor 34 // Pin untuk Input Analog dari sensor
#define LED 2 // Pin Built In LED pada ESP32

//wifi configuration
const char* ssid = "carbonmono"; // Nama WiFi
const char* password = "carbonmono"; // Kata Sandi WiFi

//mqtt configuration
const char* mqttServerIP = "broker.hivemq.com"; // MQTT Broker yang digunakan
const int mqttPort = 1883;  // Port MQTT Broker

// Untuk memanggil fungsi pada MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Topic yang digunakan untuk MQTT
char* Topic = "airquality/data/co";

// Untuk koneksi ulang ke MQTT
void reconnect(){
  while(!client.connected()){
    Serial.println("Connecting to MQTT Server..");
    Serial.print("IP MQTT Server : "); Serial.println(mqttServerIP);
    bool hasConnection = client.connect("airq");
    if(hasConnection){
      Serial.println("Success connected to MQTT Broker");
    } else {
      Serial.print("Failed connected");
      Serial.println(client.state());
      delay(2000);
      Serial.println("Try to connect...");
    }
  }
  client.publish(Topic, "Reconnecting");
}

// Untuk melakukan perintah callback agar terjadi lifecycle pada MQTT
void callback(char* topic, byte* payload, unsigned int length){
  Serial.println("Message Arrived");
  Serial.print("Topic :"); Serial.println(topic);
  Serial.print("Message : ");
  String pesan = "";
  for(int i=0; i < length; i++){
    Serial.print((char)payload[i]);
    pesan += (char)payload[i];
  }
}


// Setup
void setup() {
  // Setup Baud Rate
  Serial.begin(9600);

  // Setup Pin Digital
  pinMode(LED,OUTPUT);

  // Setup Koneksi WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());

  // Setup MQTT
  client.setServer(mqttServerIP, mqttPort);
  client.setCallback(callback);
  
  delay(20);
}


// Prosedur Koneksi WiFi
void Connection(){
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop()){
    client.connect("Client");
  }
}


// Variabel untuk menghitung nilai Karbon Monoksida
float VRL;
float Rs;
float ppm;
long RL = 1000; // 1000 Ohm
long Ro = 830; // 830 ohm

// Prosedur untuk menghitung Karbon Monoksida
void calculateSensor(){
   int sensorvalue = analogRead(pinSensor); // membaca nilai ADC dari sensor
   VRL = sensorvalue*5.00/1024;  // mengubah nilai ADC ( 0 - 1023 ) menjadi nilai voltase ( 0 - 5.00 volt )
   Serial.print("SENSOR -> VRL : ");
   Serial.print(VRL);
   Serial.print(" volt | ");
  
   Rs = ( 5.00 * RL / VRL ) - RL;
   Serial.print("Rs : ");
   Serial.print(Rs);
   Serial.print(" Ohm | ");
   
   ppm = 100 * pow(Rs / Ro,-1.53); // ppm = 100 * ((rs/ro)^-1.53);
   Serial.print("CO : ");
   Serial.print(ppm);
   Serial.println(" ppm");
}

// Prosedur untuk Publish data pada MQTT
char dataPublish[50];
void publishMQTT(char* topics, String data){
   data.toCharArray(dataPublish, data.length() + 1);
   client.publish(topics, dataPublish);
}


// Variabel untuk Delay/Interval publish dan LED blink
unsigned long previousMillisStream, prevMillis;
const long intervalStream = 500, interval = 1000; 

void loop() {
  // Memanggil prosedur Koneksi WiFi
  Connection();

  // Memanggil Prosedur Menghitung sensor
  calculateSensor();

  // Publish data ppm ke MQTT Broker setiap 1 detik (1000 milisecond)
  unsigned long currentMillisStream = millis();
  if (currentMillisStream - previousMillisStream >= intervalStream) {
    previousMillisStream = currentMillisStream;
    digitalWrite(LED,HIGH);
    publishMQTT(Topic,(String(ppm)));
    Serial.println("CO Publish: " + String(ppm));
  }

  // LED Blink Indikator mati setiap 0.5 detik (500 Milisecond)
  unsigned long currentMillis = millis();
  if (currentMillis - prevMillis >= interval) {prevMillis = currentMillis;digitalWrite(LED,LOW);}
  
  delay(10);
}
