#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

#include "FS.h"
#include "SD.h"
#include <SPI.h>

#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
constexpr char WIFI_SSID[] = "Madhu P";
uint8_t broadcastAddress[] = {0xAC, 0x67, 0xB2, 0x38, 0xDB, 0xA0};

String success;
typedef struct struct_message
{
    String s;
} struct_message;
struct_message statusHelmet;
struct_message incomingMsg;
String incomingMessage;
#define SD_CS 5

String dataMessage;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) 
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
} 

int32_t getWiFiChannel(const char *ssid)
{
  if (int32_t n = WiFi.scanNetworks())
  {
      for (uint8_t i=0; i<n; i++)
      {  
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) 
          {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingMsg, incomingData, sizeof(incomingMessage));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingMessage = incomingMsg.s;
  if(incomingMessage=="emergency") {
    buzzer_control(1);
  }
}

/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

// Hardware
int limit=4;
int g1=34;
int g2=35;
int g3=32;
int buzzer=33;
int button=25;
int logid=0;

void bsp_init(void) {
  pinMode(limit,INPUT);
  pinMode(button,INPUT);
  pinMode(g1,INPUT);
  pinMode(g2,INPUT);
  pinMode(g3,INPUT);
  pinMode(buzzer,OUTPUT);
  Serial.begin(9600);

  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while(1);
  }

  /* Set the range to whatever is appropriate for your project */
  accel.setRange(ADXL345_RANGE_16_G);

  // Initialize SD card
  SD.begin(SD_CS);  
  if(!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  }

  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
  File file = SD.open("/data.txt");
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    writeFile(SD, "/data.txt", "Reading ID, Date, Hour, Temperature \r\n");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();

  WiFi.mode(WIFI_STA);
  int32_t channel = getWiFiChannel(WIFI_SSID);
  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after
  if (esp_now_init() != ESP_OK) 
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

int read_button() {
  if(digitalRead(button)==0) 
    return(1);
  else 
    return(0);
}
void buzzer_control(int m) {
  digitalWrite(buzzer,m);
}

int read_g1(void) {
  if(digitalRead(g1)==0)
    return(1);
  else
    return(0);
}

int read_g2(void) {
  if(digitalRead(g2)==0)
    return(1);
  else
    return(0);
}

int read_g3(void) {
  if(digitalRead(g3)==0)
    return(1);
  else
    return(0);
}


int read_limit(void) {
  if(digitalRead(limit)==0)
    return(1); // helmet is available
  else
    return(0);  // helmet is not available
}

int read_gyro() {
  sensors_event_t event; 
  accel.getEvent(&event);
 
  /* Display the results (acceleration is measured in m/s^2) */
  //Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
  //Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
  //Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");Serial.println("m/s^2 ");
  
  if(event.acceleration.y>6)
    return(1);
  else
    return(0);
}

// Write to the SD card (DON'T MODIFY THIS FUNCTION)
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Write the sensor readings on the SD card
void logSDCard(String m) {
  dataMessage = m + "\r\n";
  Serial.print("Save data: ");
  Serial.println(dataMessage);
  appendFile(SD, "/data.txt", dataMessage.c_str());
}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void send_to_station(String abc) {

  statusHelmet.s=abc;
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &statusHelmet, sizeof(statusHelmet));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}

void setup() {
  bsp_init();

}

void loop() {
  int limitValue=read_limit();
  int g1Value=read_g1();
  int g2Value=read_g2();
  int g3Value=read_g3();
  int gyroValue=read_gyro();
  int buttonValue=read_button();
  
  if (limitValue==0) {
    Serial.println("Helmet Alert from ID: 123");
    send_to_station("Helmet Alert from ID: 123");
  } 
  if(g1Value==1) {
    Serial.println("SO2 Gas Alert from ID: 123");
    send_to_station("SO2 Gas Detected from ID: 123");
  }
  if(g2Value==1) {
    Serial.println("CO Gas Alert from ID: 123");
    send_to_station("CO Gas Detected from ID: 123");
  }
  if(g3Value==1) {
    Serial.println("NO2 Gas Alert from ID: 123");
    send_to_station("NO2 Gas Detected from ID: 123");
  }
  if(gyroValue==1) {
    Serial.println("Fall Alert from ID: 123");
    send_to_station("Fall Alert from ID: 123");
  }
  if(buttonValue==1) {
    Serial.println("Emergency Alert from ID: 123");
    send_to_station("Emergency Alert from ID: 123");
  }
  
  if(limitValue==0 || g1Value==1 || g2Value==1 || g3Value==1 || gyroValue==1 || buttonValue==1) 
    buzzer_control(1);
  else
    buzzer_control(0);
  logid+=1;
  String dummy=String(logid)+"," + String(limitValue)+","+String(g1Value)+","+String(g2Value)+","+String(g3Value)+","+String(gyroValue)+","+String(buttonValue);
  logSDCard(dummy);
  Serial.print(limitValue);
  Serial.print(",");
  Serial.print(g1Value);
  Serial.print(",");
  Serial.print(g2Value);
  Serial.print(",");
  Serial.print(g3Value);
  Serial.print(",");
  Serial.print(buttonValue);
  Serial.print(",");
  Serial.println(gyroValue);
}
