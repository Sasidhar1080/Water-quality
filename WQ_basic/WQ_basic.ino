#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h> // Include the Wi-Fi library
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Time.h>
#include <HTTPClient.h>
#include <WiFiManager.h> 


// const char* ssid = "IIIT-Guest";
// const char* password = "I%GR#*S@!";

#define HTTPS     false

#define CSE_IP      "onem2m.iiit.ac.in"
// #define OM2M_ORGIN    "AirPoll@20:22uHt@Sas" //AQ
#define OM2M_ORGIN    "wdmon@20:gpod@llk4"  //WQ
// #define CSE_IP      "dev-onem2m.iiit.ac.in"
// #define OM2M_ORGIN    "Tue_20_12_22:Tue_20_12_22"
#define CSE_PORT    443
#define FINGER_PRINT  "10 3D D5 4E B1 47 DB 4B 5C B0 89 08 41 A7 A4 14 87 10 7F E8"
#define OM2M_MN     "/~/in-cse/in-name/AE-WM/"
#define OM2M_AE     "WM-WD"
#define OM2M_DATA_CONT  "WM-WD-VN00-00/Data"
#define OM2M_DATA_LBL   "[\"AE-AQ\", \"V3.0.02\", \"WM-WD-VN00-00\", \"AQ-V3.0.02\"]"
#define OM2M_NODE_ID   "WD-VN00-00"



#define ONE_WIRE_BUS 27 // temp pin
#define tdssensorPin 35 // tds pin

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

int tdssensorValue = 0;
float tdsValue = 0;
float Voltage = 0;
float Temp = 0;

bool wifiConnected = false;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org"); // NTP server to use

ESP32Time rtc(0);  // offset in seconds GMT+5:30 (India Standard Time)

#define MIN_VALID_TIME 1672531200  // Unix timestamp for 1st Jan 2023 00:00:00
#define MAX_VALID_TIME 2082844799  // Unix timestamp for 1st Jan 2036 00:00:00

static uint64_t ntp_epoch_time = 0;


HTTPClient http;


void setup() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  Serial.begin(115200);
  tempSensor.begin();

  // Connect to Wi-Fi
    WiFiManager wm;
    // wm.resetSettings();
    bool res;
    res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
    if(!res) {
        Serial.println("Failed to connect");
         ESP.restart();
    } 
    else {
        Serial.println("Connected to Wi-Fi!");
        wifiConnected = true; // Update the wifiConnected variable
    }

  timeClient.begin(); // Initialize NTP client
}


void sync_time() {
  static bool first_time = true;

  if (WiFi.status() == WL_CONNECTED) {
    if (first_time){
      timeClient.begin();
      timeClient.setTimeOffset(0);
      timeClient.update();
      first_time = false;
    }

    // Check if the obtained NTP time is valid (non-zero and within a reasonable range) // 03-08-2023
    uint64_t ntp_time = timeClient.getEpochTime(); // 0;
   

    int num_tries=4;
    while (ntp_time == 0 || ntp_time < MIN_VALID_TIME || ntp_time > MAX_VALID_TIME) {
      // Invalid NTP time, perform a reset
      Serial.println("not working ");
      delay(1000);
      // ESP.restart();

      timeClient.begin();
      timeClient.setTimeOffset(0);
      timeClient.update();

      ntp_time = timeClient.getEpochTime();

      if (num_tries <= 0){
        Serial.println("Invalid NTP time, performing reset...");
        ESP.restart();
      }
        num_tries--;
    }

    rtc.offset = 0; // Set the RTC offset    
    rtc.setTime(ntp_time);

    struct tm timeinfo;
  if (getLocalTime(&timeinfo)){
       rtc.setTimeStruct(timeinfo); 
   }

    Serial.print("Universal time: " + rtc.getDateTime(true) + "\t");
    ntp_epoch_time =rtc.getEpoch();
    Serial.println(ntp_epoch_time,DEC); // For UTC, take timeoffset as 0
    Serial.println("");     
  }
}

void loop() {
  if (wifiConnected) {
    sync_time();
  //Time Sync
  Serial.print("Universal time: " + rtc.getDateTime(true) + "\t");
  ntp_epoch_time =rtc.getEpoch();
  Serial.println(ntp_epoch_time,DEC); // For UTC, take timeoffset as 0
  Serial.println("");

  // TDS calibration
  tdssensorValue = analogRead(tdssensorPin);
  Voltage = tdssensorValue * 3.3 / 1024.0; // Convert analog reading to Voltage
  Serial.print("Voltage: ");
  Serial.println(Voltage);

  // Temperature reading
  Temp = tempSensor.requestTemperaturesByIndex(0);
  Serial.print("Temperature: ");
  Serial.print(Temp);
  Serial.println(" °C");

  // Temperature compensation
  float compensationCoefficient = 1.0 + 0.02 * (Temp - 25.0);
  float compensationVoltage = Voltage / compensationCoefficient;

  // TDS with Temperature Compensation
  tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
    - 255.86 * compensationVoltage * compensationVoltage
    + 857.39 * compensationVoltage) * 0.5;

  Serial.print("TDS Value (with Temp Compensation) = ");
  Serial.print(tdsValue);
  Serial.println(" ppm");

  // TDS without Temperature Compensation
  float compensationCoefficient_without_temp = 1.0 + 0.02 * (25.0 - 25.0);
  float compensationVoltage_without_temp = Voltage / compensationCoefficient_without_temp;
  float tdsValue_without_temp = (133.42 * compensationVoltage_without_temp * compensationVoltage_without_temp
    - 255.86 * compensationVoltage_without_temp * compensationVoltage_without_temp
    + 857.39 * compensationVoltage_without_temp) * 0.5;

  Serial.print("TDS Value (without Temp Compensation) = ");
  Serial.print(tdsValue_without_temp);
  Serial.println(" ppm");
 // posting onem2m

  delay(1000);
  }

  else {
    Serial.println("Not connected to WiFi. Waiting...");
    delay(5000); // Wait for 5 seconds before attempting Wi-Fi connection again
    WiFiManager wm;
    bool res;
    res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
       Serial.println("Connected to Wi-Fi!");
        wifiConnected = true; // Update the wifiConnected variable
    }
  }
  Serial.println("completed loop");
 delay(10000);//testing
//  delay(360000);
}