
#include <EthernetWebServer.h>
#include <Arduino_JSON.h>
#include <cjson/cJSON.h>
#include "LittleFS.h"
#include <string>
#include <stdint.h>
#include "network.h"
#include "updatefirmware.h"

// Define some pins for the Ethernet
#define SCK       12
#define SS        10
#define MOSI      11
#define MISO      13
#define SPI_FREQ  32000000

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(225, 225, 225, 225);


extern network_settings_type nst;
extern user_settings_type ust;

extern String htmlContent;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while (!Serial); 

  SPI.begin(SCK, MISO, MOSI, SS);
  SPI.setFrequency(SPI_FREQ);
  Ethernet.init(SS);

  readIPAddress(&nst);      // reading the on flash saved network parameters
  readSubnetMask(&nst);
  readGateway(&nst);
  readUsername(&ust);
  readPassword(&ust);
  delay(200);

  // Start the ethernet connection and the server:
  // Ethernet.begin(mac);
  // Ethernet.begin(mac, ip);
  Ethernet.begin(mac, nst.m_ethernetSettings.m_IpAddress); // Start with the saved param on the NVM

  delay(200);
  
  if (Ethernet.hardwareStatus() == EthernetNoHardware)
  {
    Serial.println("No Ethernet found");
    while (true)
    {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }

  if (Ethernet.linkStatus() == 2)
  {
    Serial.println("No Ethernet cable");
  }
  
  if(!LittleFS.begin()){
    // Do something
    Serial.println("LittleFS has failed!");
  }
  else {
    // Do sometihng else
    Serial.println("LittleFS successfully worked!");
    delay(500);
  }

  ethernetServer.on("/", HTTP_GET, handleUpdate);
  ethernetServer.on("/update", HTTP_POST, handlePostUpdate, handleFileUpload); 
  ethernetServer.on("/user", HTTP_POST,handleNewUsername );
  ethernetServer.on("/index", HTTP_POST, handleindex);
  ethernetServer.on("/settings", HTTP_GET, handlesettings);
  ethernetServer.on("/logout", HTTP_GET, handleabmelden);
  ethernetServer.on("/login", HTTP_GET, handleanmelden);
  ethernetServer.on("/user", HTTP_GET, handleuser);
  ethernetServer.on("/userdelete", HTTP_GET,handleDeleteUser );
  ethernetServer.on("/aktualisiere", HTTP_GET, handleaktualisierung);
  
  ethernetServer.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  // Handle the ethernet connection and firmware update
  ethernetServer.handleClient();
}


// Opens the homepage after successfully login and authentication
void handleindex(){
  Serial.println("inside handleindex");

  String response;
  String s_username, s_password;
  
  File file1 = LittleFS.open("/index.html", "r");
  if (!file1) {
    Serial.println("Failed to open index.html file");
    return;
  }
    
  response = file1.readString();
  file1.close();

  Serial.println("Inside handleindex before cookies!");

  s_username = ethernetServer.arg("username");
  s_password = ethernetServer.arg("password");

  Serial.print("username: ");
  Serial.println(s_username);
  Serial.print("password: ");
  Serial.println(s_password);

  // Checks the password for the given username and retrieve the userlevel
  int level = getUserLevel(s_username.c_str(), s_password.c_str());
  Serial.print("level: ");
  Serial.println(level);

  if(level){  // If the user exist then sending the cookie to the client
    String cookie = "username=" + s_username + "," + String(level) + "; Path=/";
    ethernetServer.sendHeader("Set-Cookie", cookie);
    ethernetServer.send(200, "text/html", response);
    Serial.println("Authentication successful. Cookies sent.");
  }
  else {
    ethernetServer.send(401, "text/plain", "Wrong username or password");
    Serial.println("Authentication failed. Cookies not sent!.");
  }
}

// Logout
void handleabmelden(){
  // Send the response to the client
  ethernetServer.send(401);
}

// Login and authentication
void handleanmelden(){
  // Send the response to the client
  Serial.println("inside handleanmelden!");
  
  if (!ethernetServer.authenticate(ust.m_Benutzername, ust.m_Passwort))
     return ethernetServer.requestAuthentication();
  
  String response;
  
  File file1 = LittleFS.open("/login.html", "r");
  if (file1) {
    response += file1.readString();
    file1.close();
  }

  ethernetServer.send(200, "text/html", response);
  Serial.println("inside handleanmelden and response has been sent!");
}

// Open the page responsible for user managment
void handleuser(){
  if (!ethernetServer.authenticate(ust.m_Benutzername, ust.m_Passwort)){
    String response;                                 //This art of authentication is not working properly, idk why
    File file = LittleFS.open("/login.html", "r");
    if(file){
      response += file.readString();
      file.close();
    }
    ethernetServer.send(200, "text/html", response);
    //return ethernetServer.requestAuthentication();
  }

  // Send the response to the client
  String response;
  
  File file2 = LittleFS.open("/user.html", "r");
  if (file2) {
    response += file2.readString();
    file2.close();
  }
  ethernetServer.send(200, "text/html", response);
}

// open the page to update the server with the firmware
void handleaktualisierung(){
  if (!ethernetServer.authenticate(ust.m_Benutzername, ust.m_Passwort)){
    String response;                                 //This art of authentication is not working properly, idk why
    File file = LittleFS.open("/login.html", "r");
    if(file){
      response += file.readString();
      file.close();
    }
    ethernetServer.send(200, "text/html", response);
    //return ethernetServer.requestAuthentication();
  }

  // Send the response to the client
  String response;
  
  File file2 = LittleFS.open("/aktualisiere.html", "r");
  if (file2) {
    response += file2.readString();
    file2.close();
  }

  // Send the modified HTML as the HTTP response
  ethernetServer.send(200, "text/html", response);

}

void handlesettings() {
  // Check if the client is authenticated
  if (!ethernetServer.authenticate(ust.m_Benutzername, ust.m_Passwort)) //{
        return ethernetServer.requestAuthentication();

  // If the client is authenticated, send the settings page with a 200 status code
  String response;
  Serial.println("Step 3: opening settings.html in settings");
  File file1 = LittleFS.open("/settings.html", "r");
  if (file1) {
    Serial.println("Step 4: opened settings.html in settings");
    response += file1.readString();
    file1.close();
  }

  ethernetServer.send(200, "text/html", response);
}

