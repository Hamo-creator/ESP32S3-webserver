
#include <EthernetWebServer.h>
#include <Arduino_JSON.h>
#include <cjson/cJSON.h>
#include "LittleFS.h"
#include <Preferences.h>
#include <string>
#include <stdint.h>

#define SCK       12  //12  //37
#define SS        10  //10  //38
#define MOSI      11  //11  //35
#define MISO      13  //13  //36
#define SPI_FREQ  32000000

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(225, 225, 225, 225);

typedef struct{
  uint8_t m_IpAddress[4];       // IP-Adresse der Steuerung
  uint8_t m_Subnet[4];          // Subnetzmaske;
  uint8_t m_Gateway[4];         // Gateway
} ethernet_settings_type;

typedef struct{
  char m_Benutzername[24];
  char m_Passwort[24];
  int8_t userLevel;
} user_settings_type;

user_settings_type users[12];

network_settings_type nst;
user_settings_type ust;

String htmlContent;

// Some prototypes
void registerUser(const char* newUsername, const char* newPassword, int8_t newUserLevel);
void deleteUser(const char* usernameToDelete);
int8_t getUserLevel(const char* user, const char* password);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8E1, 35, 42);  //Raudrate, databits parity stopbits, RX, TX
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
  Ethernet.begin(mac, nst.m_ethernetSettings.m_IpAddress);

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
  
  ethernetServer.on("/user", HTTP_POST,handleNewUsername );
  ethernetServer.on("/index", HTTP_POST, handleindex);
  ethernetServer.on("/login", HTTP_GET, handleanmelden);
  ethernetServer.on("/user", HTTP_GET, handleuser);
  ethernetServer.on("/userdelete", HTTP_GET,handleDeleteUser );
  
  ethernetServer.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  // Handle the ethernet connection and firmware update
  ethernetServer.handleClient();
}

void handleNewUsername(){
  if(!ethernetServer.authenticate(ust.m_Benutzername, ust.m_Passwort))
    return ethernetServer.requestAuthentication();

  Serial.println("handleNewUserName get triggered!");

  String response;

  if(ethernetServer.args() == 0){
    response = "<h1>Empty username field</h1>";
    response += "<META http-equiv= 'refresh' content='5;URL=/'>Back to homepage...\n";
    ethernetServer.send(200, "text/html", response);
    return;
  }

  String newUsername = ethernetServer.arg("newUsername");
  Serial.print("As argument, newUsername: ");
  Serial.println(newUsername);
  String newPassword = ethernetServer.arg("newPassword");
  Serial.print("As argument, newPassword: ");
  Serial.println(newPassword);
  String confirmPassword = ethernetServer.arg("confirmPassword");
  Serial.print("As argument, confirmPassword: ");
  Serial.println(confirmPassword);
  String newuserlevel = ethernetServer.arg("userLevel");
  Serial.print("As argument, newuserlevel: ");
  Serial.println(newuserlevel);

  int8_t newUserLevel = (int8_t)(newuserlevel.toInt());
  Serial.print("After assignment, newUserLevel: ");
  Serial.println(newUserLevel);

  if(newUsername == ""){
    response = "<h1>Invalid Username format! Try something like: username</h1>";
    response += "<META http-equiv='refresh' content='5;URL=/'>Back to homepage...\n";
    // Send the response to the client
    ethernetServer.send(200, "text/html", response);
    return;
  }

  if(newPassword == confirmPassword){
    // Register the new user
    registerUser(newUsername.c_str(), newPassword.c_str(), newUserLevel);
    // New user is creadted
    ethernetServer.send(200);
  } else {
    // Passwords do not match, handle the error (e.g., return an error message)
    response = "<h1>Password and confirmPassword do not match</h1>";
    response += "<META http-equiv='refresh' content='5;URL=/'>Back to homepage...\n";
    ethernetServer.send(200, "text/html", response);
    return;
  }
}

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

  int level = getUserLevel(s_username.c_str(), s_password.c_str());
  Serial.print("level: ");
  Serial.println(level);

  if(level){
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

void handleuser(){

  if (!ethernetServer.authenticate(ust.m_Benutzername, ust.m_Passwort)){
    String response;
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

void handleUserDelete() {
  // Check if the request has the username parameter
  Serial.println("handleUserDelete get triggered");
  if (ethernetServer.hasArg("newUsername")) {
    // Get the value of the username parameter
    String username = ethernetServer.arg("newUsername");
    Serial.print("This user is going to be deleted: ");
    Serial.println(ethernetServer.arg("newUsername"));
    Serial.println(username);
    
    // Call the deleteUser function to delete the user
    deleteUser(username.c_str());
    
    // Send a response indicating success or failure
    ethernetServer.send(200, "text/plain", "User deleted successfully");
  } else {
    // If the username parameter is missing, send a response indicating a bad request
    ethernetServer.send(400, "text/plain", "Bad request: username parameter missing");
    Serial.println("username is missing");
  }
}

void registerUser(const char* newUsername, const char* newPassword, int8_t newUserLevel){

  // Validate input data (e.g. check username and password)
  size_t exn, exp, exl;

  if(numUsers > 11){
    return;
  }

  memcpy(users[numUsers].m_Benutzername, newUsername, sizeof(newUsername) + 1);
  pref.begin("Username", false);
  exn = pref.putString("Username_", newUsername);
  pref.end();

  memcpy(users[numUsers].m_Passwort, newPassword, sizeof(newPassword));
  pref.begin("Password", false);
  exp = pref.putString("Password_", newPassword);
  pref.end();

  users[numUsers].userLevel = newUserLevel;
  //memcpy(&users[numUsers].userLevel, &newUserLevel, sizeof(newUserLevel));
  pref.begin("Userlevel", false);
  exl = pref.putChar("Userlevel_", newUserLevel);
  pref.end();

  Serial.print("New username to be stored: ");
  Serial.println(users[numUsers].m_Benutzername);

  Serial.print("password for the new user: ");
  Serial.println(users[numUsers].m_Passwort);

  Serial.print("level for the new user: ");
  Serial.println(users[numUsers].userLevel);
  Serial.print("newUserLevel: ");
  Serial.println(newUserLevel);


  Serial.print("Saving newUsername in NVM was (x: success, 0: failed): ");
  Serial.println(exn);
  Serial.print("Saving newPassword in NVM was (x: success, 0: failed): ");
  Serial.println(exp);
  Serial.print("Saving newUserLevel in NVM was (1: success, 0: failed): ");
  Serial.println(exl);

  // Increment the number of users
  numUsers++;
  Serial.print("numUsers after increment: ");
  Serial.println(numUsers);
}

void deleteUser(const char* usernameToDelete) {
    // Find the index of the user to delete
    int indexToDelete = -1;
    for (int i = 0; i < numUsers; ++i) {
        if (strcmp(users[i].m_Benutzername, usernameToDelete) == 0) {
            indexToDelete = i;
            break;
        }
    }

    // If the user does not exist, return without performing deletion
    if (indexToDelete == -1) {
      Serial.println("User does not exist. No action taken.");
      return;
    }

    // If the user was found, Remove the user from non-volatile memory
    pref.begin("UserSettings", false);
    pref.remove(("Username_" + String(indexToDelete)).c_str());
    pref.remove(("Password_" + String(indexToDelete)).c_str());
    pref.remove(("UserLevel_" + String(indexToDelete)).c_str());
    pref.end();
    // shift all elements after the user one position to the left
    for (int i = indexToDelete; i < numUsers - 1; ++i) {
      memmove(users[i].m_Benutzername, users[i + 1].m_Benutzername, sizeof(users[i].m_Benutzername));
      memmove(users[i].m_Passwort, users[i + 1].m_Passwort, sizeof(users[i].m_Passwort));
      //memmove(&users[i].userLevel, &users[i + 1].userLevel, sizeof(users[i].userLevel));
      users[i].userLevel = users[i + 1].userLevel;
      Serial.println("---------AFTER MEMMOVE--------");
      Serial.print("user[");
      Serial.print(i);
      Serial.print("].m_Benutzername : ");
      Serial.println(users[i].m_Benutzername);
      Serial.print("users[");
      Serial.print(i);
      Serial.print("].m_Passwort : ");
      Serial.println(users[i].m_Passwort);
      Serial.print("users[");
      Serial.print(i);
      Serial.print("].userLevel : ");
      Serial.println(users[i].userLevel);
    }


    // Decrement the number of users
    numUsers--;

    Serial.print("User ");
    Serial.print(usernameToDelete);
    Serial.println(" deleted successfully.");
    Serial.print("numUsers afer deleting a user: ");
    Serial.println(numUsers);
}

int8_t getUserLevel(const char* user, const char* password){
  // Find the index of the user
  int userIndex = -1;
  int8_t userlevel = 0;

  String useRName;
  String pasSWord;

  for(int i = 0; i < numUsers; i++){
    if(strcmp(users[i].m_Benutzername, user) == 0){
      userIndex = i;
      break;
    }
  }

  Serial.print("userIndex: ");
  Serial.println(userIndex);

  if(userIndex == -1){
    Serial.println("User does not exist!!");
    return 0;
  }

  if(strcmp(users[userIndex].m_Passwort, password) == 0){
    // Get user level
    pref.begin("UserLevel", false);
    userlevel = pref.getChar("UserLevel_", users[userIndex].userLevel);
    pref.end();

    pref.begin("Username", false);
    useRName = pref.getString("Username_", users[userIndex].m_Benutzername, sizeof(users[userIndex].m_Benutzername) + 1);
    pref.end();

    pref.begin("Password", false);
    pasSWord = pref.getString("Password_", users[userIndex].m_Passwort, sizeof(users[userIndex].m_Passwort));
    pref.end();


    Serial.print("From pref. useRName: ");
    Serial.println(useRName);
    Serial.println(users[userIndex].m_Benutzername);
    Serial.print("From pref. pasSWord: ");
    Serial.println(pasSWord);
    Serial.println(users[userIndex].m_Passwort);
    Serial.print("From pref. userlevel: ");
    Serial.println(userlevel);
    Serial.println(users[userIndex].userLevel);
  }

  return userlevel;
}

