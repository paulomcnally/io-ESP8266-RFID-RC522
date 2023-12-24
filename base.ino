#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>

#include <SPI.h>     // incluye libreria bus SPI
#include <MFRC522.h> // incluye libreria especifica para MFRC522

// Wifi Data
const char *ssid = "WIFI_SSID_CHANGE_ME";
const char *password = "WIFI_PASSWORD_CHANGE_ME";

// Host Data
String protocol = "https";
String host = "www.attendances.app";
String path = "/api/identifiers/read";

// POST PARAMS
String api_token = "API_TOKEN_CHANGE_ME";
String uuidHex;

#define RST_PIN 4
#define SS_PIN 2
#define BUZZER_PIN 5

MFRC522 mfrc522(SS_PIN, RST_PIN);

/**
 * Buzzer
 */
void callBuzzer(int pin, int duration, int times = 1, bool useSecondDelay = true) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(duration);
    digitalWrite(pin, LOW);
    if (useSecondDelay) {
      delay(duration);
    }
  }
}

/**
 * HTTP Request
 */
void httpRequest(String uuid) {
  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect(host, 443)) {
    Serial.println("connection failed");
  } else {
    Serial.println("HTTP Request | Init");
    HTTPClient http;
    String data = "api_token=" + api_token + "&key=" + uuid;
    String url = protocol + "://" + host + path;
    Serial.print("HTTP Request | URL  ►  ");
    Serial.println(url);
    Serial.print("HTTP Request | Params  ►  ");
    Serial.println(data);
    http.begin(client, url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int response_code = http.POST(data);

    if (response_code > 0) {
      Serial.println("HTTP Request | Status Code ► " + String(response_code));

      if (response_code == 200) {
        String body = http.getString();
        Serial.println("HTTP Request | Response  ►  ");
        Serial.println(body);

        if (body.startsWith("{\"company\":")) {
          callBuzzer(BUZZER_PIN, 100, 1, false);
        } else {
          callBuzzer(BUZZER_PIN, 1000, 2);
        }
      } else {
        callBuzzer(BUZZER_PIN, 1000, 2);
      }
    } else {
      Serial.print("HTTP Request | Error Code  ►  ");
      Serial.println(response_code);
      callBuzzer(BUZZER_PIN, 1000, 3);
    }
    http.end();
  }
}


void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  delay(10);
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  // start wifi connection
  WiFi.begin(ssid, password);

  Serial.print("WiFi | Connecting  ►  ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
    callBuzzer(BUZZER_PIN, 500, 1, false);
  }

  Serial.print("WiFi | IP  ►  ");
  Serial.println(WiFi.localIP());
}

void loop() {
  uuidHex = "";

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uint8_t currentByte = mfrc522.uid.uidByte[mfrc522.uid.size - (i + 1)];
      if (currentByte < 16) {
        uuidHex += "0";
      }
      uuidHex += String(currentByte, HEX);
    }

    Serial.print("RFID | Hexadecimal  ►  ");
    Serial.println(uuidHex);

    String uuidHexSeven =  uuidHex.substring(uuidHex.length() - 8);
    Serial.print("RFID | Hexadecimal 8  ►  ");
    Serial.println(uuidHexSeven);


    String uuidDec = uuidHexSeven;
    long unsigned B = strtoul(uuidDec.c_str(),NULL,16);
    uuidDec = String(B);
    while (uuidDec.length() < 10) {
      uuidDec = "0" + uuidDec;
    }

   
    Serial.print("RFID | Decimal  ►  ");
    Serial.println(uuidDec);
    mfrc522.PICC_HaltA();

    if (WiFi.status() == WL_CONNECTED) {
      httpRequest(uuidDec);
    } else {
      Serial.println("Wifi error."); // mensaje si se desconecto del wifi el modulo
      callBuzzer(BUZZER_PIN, 1000, 4);
    }
  } 
  delay(1000);
}

