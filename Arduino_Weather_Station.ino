// To use ArduinoGraphics APIs, please include BEFORE Arduino_LED_Matrix
#include "ArduinoGraphics.h"
#include "Arduino_LED_Matrix.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "RTC.h"
#include <Wire.h>
#include <NTPClient.h>

ArduinoLEDMatrix matrix;

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#include "icons.h"

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 1584)
const int epd_bitmap_allArray_LEN = 11;
const unsigned char* epd_bitmap_allArray[11] = {
  epd_bitmap_cloud_drizzle,              // 09d && 09n
  epd_bitmap_cloud_fill,                 // 03d && 03n
  epd_bitmap_cloud_haze2_fill,           // 50d && 50n
  epd_bitmap_cloud_lightning_rain_fill,  // 11d && 11n
  epd_bitmap_cloud_rain,                 // 10d && 10n
  epd_bitmap_cloud_sun_fill,             // 02d
  epd_bitmap_cloud_moon_fill,            // 02n
  epd_bitmap_clouds_fill,                // 04d && 04n
  epd_bitmap_moon_fill,                  // 01n
  epd_bitmap_snow,                       // 13d && 13n
  epd_bitmap_sun_fill                    // 01d

};

const unsigned char* getIcon(const char* icon) {
  if (strcmp(icon, "09d") == 0 || strcmp(icon, "09n") == 0) {
    return epd_bitmap_allArray[0];
  } else if (strcmp(icon, "03d") == 0 || strcmp(icon, "03n") == 0) {
    return epd_bitmap_allArray[1];
  } else if (strcmp(icon, "50d") == 0 || strcmp(icon, "50n") == 0) {
    return epd_bitmap_allArray[2];
  } else if (strcmp(icon, "11d") == 0 || strcmp(icon, "11n") == 0) {
    return epd_bitmap_allArray[3];
  } else if (strcmp(icon, "10d") == 0 || strcmp(icon, "10n") == 0) {
    return epd_bitmap_allArray[4];
  } else if (strcmp(icon, "02d") == 0) {
    return epd_bitmap_allArray[5];
  } else if (strcmp(icon, "02n") == 0) {
    return epd_bitmap_allArray[6];
  } else if (strcmp(icon, "04d") == 0 || strcmp(icon, "04n") == 0) {
    return epd_bitmap_allArray[7];
  } else if (strcmp(icon, "01n") == 0) {
    return epd_bitmap_allArray[8];
  } else if (strcmp(icon, "13d") == 0 || strcmp(icon, "13n") == 0) {
    return epd_bitmap_allArray[9];
  } else if (strcmp(icon, "01d") == 0) {
    return epd_bitmap_allArray[10];
  }
}
#include "arduino_secrets.h"

#include "WiFiS3.h"
#include <WiFiUdp.h>
#include <Arduino_JSON.h>
#include <assert.h>
#include <math.h>
const char* apiKey = SECRET_API_KEY;
const float lat = SECRET_lat;
const float lon = SECRET_lon;
const char* host = "api.openweathermap.org";


int wifiStatus = WL_IDLE_STATUS;
WiFiUDP Udp;  // A UDP instance to let us send and receive packets over UDP
NTPClient timeClient(Udp);

unsigned long lastConnectionTime = 0;          // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 120000;  // delay between updates, in milliseconds
JSONVar myObject;
JSONVar jsonResult;

unsigned char frame[8][12];
int temperature = 0;
int winddeg = 0;
int humi = 0;
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;  // your network SSID (name)
char pass[] = SECRET_PASS;  // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;           // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;

WiFiClient client;

/* -------------------------------------------------------------------------- */
void setup() {

  RTC.begin();
  /* -------------------------------------------------------------------------- */
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Stazione Meteo");
  display.drawBitmap(0, 7, getIcon("01d"), 23, 23, WHITE);
  display.display();

  delay(5000);
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Communication with WiFi module failed!");
    display.display();
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Please upgrade the firmware");
    display.display();
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Attempting to connect to SSID:");
    display.setCursor(0, 1);
    display.print(ssid);
    display.display();
    status = WiFi.begin(ssid, pass);
  }

  printWifiStatus();
  delay(5000);
  Serial.println("\nStarting connection to server...");
  timeClient.begin();
  timeClient.update();

  auto timeZoneOffsetHours = 2;
  auto unixTime = timeClient.getEpochTime() + (timeZoneOffsetHours * 3600);
  Serial.print("Unix time = ");
  Serial.println(unixTime);
  RTCTime timeToSet = RTCTime(unixTime);
  RTC.setTime(timeToSet);

  RTCTime currentTime;
  RTC.getTime(currentTime);
  Serial.println("The RTC was just set to: " + String(currentTime));

  http_request();
}

void loop() {
  //Bars
  RTCTime currentTime;

  RTC.getTime(currentTime);
  display.clearDisplay();
  printWifiBar();
  display.setCursor(0, 0);

  String hours = String(currentTime.getHour());
  String minutes = String(currentTime.getMinutes());
  String seconds = String(currentTime.getSeconds());
  display.print(hours.length() == 1 ? "0" + hours : hours);
  display.print(":");
  display.print(minutes.length() == 1 ? "0" + minutes : minutes);
  display.print(":");
  display.print(seconds.length() == 1 ? "0" + seconds : seconds);
  //Body

  if (jsonResult.hasOwnProperty("main")) {
    JSONVar main = jsonResult["main"];
    temperature = main["temp"];
    String description = jsonResult["weather"][0]["description"];
    winddeg = jsonResult["wind"]["deg"];
    humi = main["humidity"];
    //Icon
    display.drawBitmap(0, 7, getIcon(jsonResult["weather"][0]["icon"]), 23, 23, WHITE);


    display.setCursor(30, 10);
    display.print(temperature);
    display.print("C");
    display.setCursor(70, 10);
    display.print(humi);
    display.print("%");
    display.setCursor(30, 20);
    display.print(description);
  }

  display.display();


  read_response();
  if (millis() - lastConnectionTime > postingInterval) {
    http_request();
  }
}

void read_response() {
  if (client.available()) {
    Serial.println("reading response");
    String line = client.readStringUntil('\r');

    Serial.print(line);
    if (line == "\n") {
      Serial.println("headers received");
      String payload = client.readString();
      myObject = JSON.parse(payload);
      jsonResult = myObject;
    }
  }
}

void http_request() {

  client.stop();

  Serial.println("\nStarting connection to server...");

  if (client.connect(host, 80)) {
    Serial.println("connected to server");
    String request = "/data/2.5/weather?lat=" + String(lat) + "&lon=" + String(lon) + "&lang=it&units=metric&appid=" + apiKey;
    client.println("GET " + request + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("Connection: close");
    client.println();
    lastConnectionTime = millis();
  } else {
    Serial.println("connection failed");
  }
}

void printWifiBar() {
  long rssi = WiFi.RSSI();

  if (rssi > -55) {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.fillRect(112, 4, 4, 4, WHITE);
    display.fillRect(117, 2, 4, 6, WHITE);
    display.fillRect(122, 0, 4, 8, WHITE);
  } else if (rssi<-55 & rssi> - 65) {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.fillRect(112, 4, 4, 4, WHITE);
    display.fillRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  } else if (rssi<-65 & rssi> - 75) {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.fillRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  } else if (rssi<-75 & rssi> - 85) {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.drawRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  } else if (rssi<-85 & rssi> - 96) {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.drawRect(107, 6, 4, 2, WHITE);
    display.drawRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  } else {
    display.drawRect(102, 7, 4, 1, WHITE);
    display.drawRect(107, 6, 4, 2, WHITE);
    display.drawRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  }
}

void printWifiStatus() {

  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(String(WiFi.SSID()));

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  display.setCursor(0, 20);
  display.println(ip);

  long rssi = WiFi.RSSI();

  if (rssi > -55) {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.fillRect(112, 4, 4, 4, WHITE);
    display.fillRect(117, 2, 4, 6, WHITE);
    display.fillRect(122, 0, 4, 8, WHITE);
  } else if (rssi<-55 & rssi> - 65) {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.fillRect(112, 4, 4, 4, WHITE);
    display.fillRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  } else if (rssi<-65 & rssi> - 75) {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.fillRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  } else if (rssi<-75 & rssi> - 85) {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.fillRect(107, 6, 4, 2, WHITE);
    display.drawRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  } else if (rssi<-85 & rssi> - 96) {
    display.fillRect(102, 7, 4, 1, WHITE);
    display.drawRect(107, 6, 4, 2, WHITE);
    display.drawRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  } else {
    display.drawRect(102, 7, 4, 1, WHITE);
    display.drawRect(107, 6, 4, 2, WHITE);
    display.drawRect(112, 4, 4, 4, WHITE);
    display.drawRect(117, 2, 4, 6, WHITE);
    display.drawRect(122, 0, 4, 8, WHITE);
  }

  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  display.setCursor(0, 10);
  display.print(String(rssi) + " dBm");
  display.display();
}
