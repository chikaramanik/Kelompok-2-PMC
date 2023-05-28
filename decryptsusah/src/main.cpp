#include <Arduino.h>

#include <WiFi.h>
#include <PubSubClient.h>

// additional header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
// Decryption
void decrypt(byte* ciphertext, byte* key, byte* iv, byte* text, int data_len) {
    byte round_keys[40];
    byte keystream[16];
    byte block[16];
    int i, j, k;

    // Generate round keys
    key_schedule(key, round_keys);

    // Decrypt each block of data
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

        // Decrypt the block
        for (j = 0; j < 16; j++) {
            text[i + j] = ciphertext[i + j] ^ keystream[j];
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
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
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
  unsigned char recieved[length];
  unsigned char decrypted[length];
  byte key[] = "1234567890ABCDEF";
  byte iv[] = "0000000000000000";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    recieved[i] = (char)payload[i];
    //Serial.print((char)payload[i]);
  }
  for (int i = 0; i < 16; i ++){
    Serial.printf("%02X", recieved[i]);
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
  decrypt(recieved, key, iv, decrypted,4);
  Serial.print("Decrypted message : ");
  Serial.printf("%s", decrypted);

  // conditional
  Serial.println();

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
     // ... and resubscribe
     client.subscribe("1sampai10");
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
}

void loop() {

 if (!client.connected()) {
   reconnect();
 }
  client.loop();

  byte key[] = "0123456789ABCDEF";
  byte iv[] = "0000000000000000";
  byte test[16];
  int data_len = 9;
  int original_data_len = data_len;;

  // while (data_len % 16 != 0){
  //   data
  // }
  memcpy(iv, "0000000000000000", 16);

  
}