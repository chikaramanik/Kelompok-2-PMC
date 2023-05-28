
#include <WiFi.h>
#include <PubSubClient.h>

// tambahan header untuk sensor MPE6050
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

// additional header for cryptography
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef unsigned char byte;

void swap(byte* a, byte* b) {
    byte temp = *a;
    *a = *b;
    *b = temp;
}

// Key schedule
void key_schedule(byte* key, byte* round_keys) {
    int i, j;
    byte S[256];

    // Initialize S-box
    for (i = 0; i < 256; i++) {
        S[i] = i;
    }

    // Permute S-box using key
    j = 0;
    for (i = 0; i < 256; i++) {
        j = (j + key[i % 16] + S[i]) % 256;
        swap(&S[i], &S[j]);
    }

    // Generate round keys
    j = 0;
    for (i = 0; i < 40; i++) {
        round_keys[i] = S[(i + 1) % 256];
        j = (j + round_keys[i]) % 256;
        swap(&S[i], &S[j]);
    }
}

// Encryption
void encrypt(char* data, byte* key, byte* iv, byte* ciphertext, int data_len) {
    byte round_keys[40];
    byte keystream[16];
    byte block[16];
    int i, j, k;

    // Generate round keys
    key_schedule(key, round_keys);

    // Encrypt each block of data
    for (i = 0; i < data_len; i += 16) {
        // Generate keystream for this block
        memcpy(block, iv, 16);
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 16; k++) {
                keystream[j * 4 + k] = block[k];
            }
            block[15]++;
            if (block[15] == 0) {
                block[14]++;
            }
        }
        for (j = 0; j < 16; j++) {
            keystream[j] ^= round_keys[j + 24];
        }

        // Encrypt the block
        for (j = 0; j < 16; j++) {
            ciphertext[i + j] = data[i + j] ^ keystream[j];
        }

        // Update the IV
        memcpy(iv, ciphertext + i, 16);
    }
}

// Update these with values suitable for your network.
const char* ssid = "chikara";
const char* password = "chikaramanik";
const char* mqtt_server = "broker.mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(1024)
#define MAX_ENCRYPTED_DATA_SIZE 1024
char msg[MSG_BUFFER_SIZE];
char msg1[MSG_BUFFER_SIZE];
char msg2[MSG_BUFFER_SIZE];
char msg3[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("1sampai10", "hello world");
      // ... and resubscribe
      //client.subscribe("1sampai10");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
  delay(100);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  
  client.loop();

  char data[1024];

  // taking sensor value
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float acc, velo;
  acc = sqrt(pow(a.acceleration.x,2) + pow(a.acceleration.y,2) + pow(a.acceleration.z,2));
  velo = sqrt(pow(g.gyro.x,2) + pow(g.gyro.y,2) + pow(g.gyro.z,2));

  // making conditional
  float sbx, sby, sbz;
  char vector;
  sbx = a.acceleration.x;
  sby = a.acceleration.y;
  sbz = a.acceleration.z;
  if (sbx > 0 && sby > 0 && sbz && 0){
    
  }
  sprintf(data, "%.4f,%.4f", acc,velo);


  // sprintf(data, "%f",acc);
  char test[] = "kanan";

  byte key[] = "1234567890ABCDEF";
  byte iv[] = "0000000000000000";
  byte ciphertext[MAX_ENCRYPTED_DATA_SIZE];
  byte plaintext[MAX_ENCRYPTED_DATA_SIZE];
  int data_len = strlen(data);
  int original_data_len = data_len;

  // padding
  while (data_len % 16 != 0){
    data[data_len] = 0;
    data_len++;
  }
  encrypt(data, key, iv, ciphertext, data_len);

  unsigned long now = millis();
  if (now - lastMsg > 20000) {
    lastMsg = now;
    ++value;
    snprintf (msg3, MSG_BUFFER_SIZE, "%02X",ciphertext);

    Serial.print("Publish message: \n");

    //Serial.println(msg);
    //Serial.println(msg1);
    //Serial.println(msg2);
    for (int i = 0; i < data_len; i++){
      Serial.printf("%02X", ciphertext[i]);
    }
    Serial.printf("\n");
    // Serial.println(msg3);
    //client.publish("1sampai10", msg);
    //client.publish("1sampai10", msg1);
    //client.publish("1sampai10", msg2);
    client.publish("1sampai10", ciphertext,data_len);

  }
}
