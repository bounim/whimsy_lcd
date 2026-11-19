#include "SPIFFS.h"
#include "classicneue.h"
#include <FS.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

//const char* ssid = "FibreBox_X6-510357";
//const char* password = "C34YYWKHWAACNCFPFC";

const char* ssid = "Freebox-3BEFF4";
const char* password = "exciri-specifice!5-cacaram8?-anfractum";
const char* city = "Paris";

const char* weatherAPI = "https://api.openweathermap.org/data/2.5/weather?lat={lat}&lon={lon}&appid={API key}";
const char* geolocationAPI = "http://api.openweathermap.org/geo/1.0/direct?q=Paris,FR&limit=1&appid=d053630738a201b6146d86799c0d3a4c";
const char* weatherAPIid = "d053630738a201b6146d86799c0d3a4c";
const char* timeAPI = "https://time.now/developer/api/timezone/Europe/Zurich";





TFT_eSPI tft = TFT_eSPI();

void setupWifiConnection() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  tft.drawString("Connecting to ", 30, 30);
  tft.drawString(ssid, 30, 30);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);

  // Wait for the connection to establish
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {

  Serial.println();
  Serial.println("SPIFFS files found:");

  Serial.printf("Listing directory: %s\n", "/");
  String  line = "=====================================";

  Serial.println(line);
  Serial.println(" Name               Size");
  Serial.println(line);

  fs::File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  } else {
    Serial.println("Directory opened");
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  fs::File file = root.openNextFile();
  while (file) {

    if (file.isDirectory()) {
      Serial.print("DIR : ");
      String fileName = file.name();
      Serial.print(fileName);
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      String fileName = file.name();
      Serial.print("  " + fileName);
      int spaces = 20 - fileName.length(); // Tabulate nicely
      while (spaces--) Serial.print(" ");
      String fileSize = (String) file.size();
      spaces = 10 - fileSize.length(); // Tabulate nicely
      while (spaces--) Serial.print(" ");
      Serial.println(fileSize + " bytes");
    }

    file = root.openNextFile();
  }

  Serial.println(line);
  Serial.println();
  delay(1000);
}

void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void listFiles(void) {
  listDir(SPIFFS, "/", 0);
}


// SPIFFS address is 0x00290000

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(10);

  Serial.println("Initializing...");

  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(0xFFF0F5);
  tft.loadFont("clacon2");
  tft.drawString("setting up...", 30, 30);
//  tft.fillScreen(random(0xFFFF));
  if (!SPIFFS.begin(true)) {
    Serial.println("\nSPIFFS mount failed");
    return;
  } else {
    Serial.println("\nSPIFFS mount successful");
  }
  listFiles();
  delay(2000);
  setupWifiConnection();
  delay(2000);
}

void getCoordinatesLoop() {
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    // Configuration pour ignorer la vérification du certificat SSL (Idéal pour le développement)
    client->setInsecure();
    HTTPClient http;
    Serial.println("\n[HTTPS] Initialisation de la connexion...");
    // Initialiser la requête avec le client sécurisé et l'URL
    
    if (http.begin(*client, timeAPI)) {
      // Envoi de la requête GET
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        Serial.printf("[HTTPS] Code de réponse : %d\n", httpResponseCode);

        // Récupération de la chaîne de caractères (Payload)
        String payload = http.getString();

        // 2. Désérialisation du flux JSON
        // JsonDocument remplace DynamicJsonDocument / StaticJsonDocument en version v7
        JsonDocument doc;

        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          // Extraction des données du format JSON vers des variables C++
          const char* lat = doc["lat"];
          const char* lon = doc["lon"];
          const char* country = doc["country"];

          // Affichage des valeurs extraites
          Serial.println("--- Données JSON Extraites ---");
          Serial.printf("Country     : %s\n", country);
          Serial.printf("Location : %s, %s\n", lat, lon);
        }
        else {
          Serial.print("Échec de la désérialisation JSON : ");
          Serial.println(error.c_str());
        }
      }
      else {
        Serial.printf("[HTTPS] Échec de la requête, erreur : %s\n", http.errorToString(httpResponseCode).c_str());
      }

      // Libération des ressources de la requête
      http.end();
    }
    else {
      Serial.printf("[HTTPS] Impossible de se connecter au serveur\n");
    }
    // Libération de la mémoire du client sécurisé
    delete client;
  } else {
    Serial.println("Wi-Fi déconnecté");
  }
}

void getTimeLoop() {
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client) {
    // Configuration pour ignorer la vérification du certificat SSL (Idéal pour le développement)
    client->setInsecure();
    HTTPClient http;
    Serial.println("\n[HTTPS] Initialisation de la connexion...");
    // Initialiser la requête avec le client sécurisé et l'URL
    if (http.begin(*client, timeAPI)) {
      // Envoi de la requête GET
      int httpResponseCode = http.GET();

      if (httpResponseCode > 0) {
        Serial.printf("[HTTPS] Code de réponse : %d\n", httpResponseCode);

        // Récupération de la chaîne de caractères (Payload)
        String payload = http.getString();

        // 2. Désérialisation du flux JSON
        // JsonDocument remplace DynamicJsonDocument / StaticJsonDocument en version v7
        JsonDocument doc;

        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
          // Extraction des données du format JSON vers des variables C++
          const char* timezone = doc["timezone"];
          const char* datetime = doc["datetime"];

          // Affichage des valeurs extraites
          Serial.println("--- Données JSON Extraites ---");
          Serial.printf("Time zone   : %s\n", timezone);
          Serial.printf("Datetime    : %s\n", datetime);
          tft.drawString(timezone, 30, 30);
          tft.drawString(datetime, 30, 30);
        }
        else {
          Serial.print("Échec de la désérialisation JSON : ");
          Serial.println(error.c_str());
        }
      }
      else {
        Serial.printf("[HTTPS] Échec de la requête, erreur : %s\n", http.errorToString(httpResponseCode).c_str());
      }

      // Libération des ressources de la requête
      http.end();
    }
    else {
      Serial.printf("[HTTPS] Impossible de se connecter au serveur\n");
    }
    // Libération de la mémoire du client sécurisé
    delete client;
  } else {
    Serial.println("Wi-Fi déconnecté");
  }

  // Attendre 15 secondes avant la prochaine requête
  delay(15000);
}

void loop() {
  // put your main code here, to run repeatedly:
//  tft.setRotation(0);
//  tft.fillScreen(random(0xFFFF));
  delay(1000);
//  getTimeLoop();
//  getCoordinatesLoop();
}
