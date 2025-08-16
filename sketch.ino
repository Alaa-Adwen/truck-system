#include "DHT.h"
#include "WiFi.h"
#include "FirebaseESP32.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "NMEA.h"

#define OLED_RESET -1 
#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 19 //Digital pin connected to the sensor

#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

#include <addons/RTDBHelper.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""

#define API_KEY "AIzaSyCOJTJ1etcpZVhVA-VzoiRCRwjhLBF3uCE"


#define DATABASE_URL "https://track-truck-system-default-rtdb.europe-west1.firebasedatabase.app/"


const char* items[] = {"Fruits", "Meat", "Medicine"};
int selectedIndex = 0;


#define LEN(arr) ((int)(sizeof(arr) / sizeof(arr)[0]))

union {
  char bytes[4];
  float valor;
} velocidadeGPS;

float latitude;
float longitude;

NMEA gps(GPRMC); 


FirebaseData fbdo;

FirebaseConfig config;
FirebaseAuth auth;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600); 

  pinMode(15, INPUT);  // Button to cycle

  dht.begin();

    // initialize OLED display with I2C address 0x3C
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("failed to start SSD1306 OLED"));
    while (1);
  }

  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);

  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.database_url = DATABASE_URL;
  config.api_key = API_KEY;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Firebase.setFloatDigits(2);

  while (!Firebase.signUp(&config, &auth, "" , "")){
    Serial.println("Sign-up failed");
    delay(3000);
  }

  Serial.println("Firebase signed up");

  Firebase.setString(fbdo,"/selectedItem", "......");

  delay(2000);         // wait two seconds for initializing
  oled.clearDisplay(); // clear display

  oled.setTextSize(1);         // set text size
  oled.setTextColor(WHITE);    // set text color
  oled.setCursor(30, 20);       // set position to display
}

void loop() {

  while (Serial2.available()) {             // Waits for serial port data

    char serialData = Serial2.read();       // Receives data from GPS serial port
    Serial.print(serialData);


    if (gps.decode(serialData)) {           // Checks if the GPS sentence is valid
      if (gps.gprmc_status() == 'A') {      // Checks if GPS status is 'A'

        velocidadeGPS.valor = gps.gprmc_speed(KMPH);        // Receives GPS speed in km/h

      } else {
        velocidadeGPS.valor = 0;
      }

      latitude  = gps.gprmc_latitude();
      longitude = gps.gprmc_longitude();

      // Add line break
      Serial.println();
      Serial.println();

      // Show Latitude
      Serial.print(" Latitude: ");
      Serial.println(latitude, 8);

      // Show Longitude
      Serial.print("Longitude: ");
      Serial.println(longitude, 8);

      // Show Speed ​​in km/h
      Serial.print("    Speed: ");
      Serial.print(velocidadeGPS.valor);
      Serial.println(" Km/h");

      // Converts Geographic Coordinates to Cartesian Plane
      convertCoordinatesToCartesian(latitude, longitude);
    }
  }
if (Firebase.getString(fbdo, "/selectedItem")) {
  String item = fbdo.stringData();

  // Optional: make it case-insensitive
  item.toLowerCase();

  if (item == "fruits") {
    selectedIndex = 0;
  } else if (item == "meat") {
    selectedIndex = 1;
  } else if (item == "medicine") {
    selectedIndex = 2;
  }

  Serial.print("Selected item from Firebase: ");
  Serial.println(item);
} else {
  Serial.println("Failed to read /selectedItem from Firebase");
}


  float t = dht.readTemperature();


  if(isnan(t)){
    Serial.println(F("failed to read from DHT sensor"));
    return;
  }



  Firebase.setFloat(fbdo,"/sensor/temperature", t);
  Firebase.setString(fbdo,"/Warnings", "......");

  if (t > 7 && strcmp(items[selectedIndex], "Fruits") == 0 ) {
  Firebase.setString(fbdo,"/Warnings", "⚠ WARNING: Temperature too high, product may spoil!");
  tone(5, 262, 250);
  }
  if (t > -15 && strcmp(items[selectedIndex], "Meat") == 0 ) {
  Firebase.setString(fbdo,"/Warnings", "⚠ WARNING: Temperature too high, product may spoil!");
  tone(5, 262, 250);
  }
  if (t > 30 && strcmp(items[selectedIndex], "Medicine") == 0 ) {
  Firebase.setString(fbdo,"/Warnings", "⚠ WARNING: Temperature too high, product may spoil!");
  tone(5, 262, 250);
  }

  Serial.print(F("Temperature: "));
  Serial.print(t);
  Serial.print(F("C"));
  Serial.println();
}

void convertCoordinatesToCartesian(float latitude, float longitude) {

  float latRadius = latitude  * (PI) / 180;  // Convert from Degrees to Radians
  float lonRadius = longitude * (PI) / 180;

  int earthRadius = 6371; // Radius in km

  float posX = earthRadius * cos(latRadius) * cos(lonRadius);
  float posY = earthRadius * cos(latRadius) * sin(lonRadius);

  Serial.print("        X: ");  Serial.println(posX);
  Serial.print("        Y: ");  Serial.println(posY);
}
   