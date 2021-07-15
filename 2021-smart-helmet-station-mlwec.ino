#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

#define BLYNK_PRINT Serial
#include <Blynk.h>
#include <BlynkSimpleEsp32.h>
char auth[] = "1Owcp70dw-_AnApYMTJMjMBnr_zfPPiD";             
constexpr char WIFI_SSID[] = "Madhu P";

int button=25;
int buzzer=33;

// REPLACE WITH THE MAC Address of your receiver 
uint8_t broadcastAddress[] = {0xAC, 0x67, 0xB2, 0x38, 0xB4, 0x20};
// Define variables to send message
String message;
// Define variables to store incoming message
String incomingMessage;
// Variable to store if sending data was successful
String success;
//Must match the receiver structure
typedef struct struct_message {
    String s;
} struct_message;

// Create a struct_message called BME280Readings to hold sensor readings
struct_message send_msg;

// Create a struct_message to hold incoming sensor readings
struct_message incomingMsg;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingMsg, incomingData, sizeof(incomingMessage));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingMessage = incomingMsg.s;
  digitalWrite(buzzer,1);
  delay(1000);
  digitalWrite(buzzer,0);
  Blynk.notify(incomingMessage);
  delay(5000);
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
void setup() 
{
  pinMode(button,INPUT);
  pinMode(buzzer,OUTPUT);
  Serial.begin(9600);
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin("Madhu P","madhu2022");
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println("Connected to Wifi Network");
  int32_t channel = getWiFiChannel(WIFI_SSID);
  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) 
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  // Register peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
  Blynk.begin(auth,"Madhu P","madhu2022");
}
 
void loop() 
{
  Blynk.run();
  if(digitalRead(button)==0) {
  // Set values to send
  send_msg.s = "emergency";
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &send_msg, sizeof(send_msg));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  }
  updateDisplay();
  delay(10000);
  
}

void updateDisplay()
{
  // Display Readings in Serial Monitor
  Serial.println("INCOMING Message:");
  Serial.print("mESSAGE: ");
  Serial.print(incomingMsg.s);
  Serial.println(" ");
}
