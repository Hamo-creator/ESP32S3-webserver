#include <stddef.h>
#include <string.h>
#include "network.h"
#include <cstring>
#include <Arduino.h>
#include <Preferences.h>

network_settings_type networkSettings;

user_settings_type userSettings;

security_setting_type securitySettings;

Preferences pref;

String ipaddress, subnet, gateway;

String usernameString;
String passwordString;

user_settings_type users[12];

int numUsers = 0;


// Function to change the IP address
void changeIPAddress(network_settings_type *settings, uint8_t *newIPAddress) {                     
  // Copy the new IP address to the appropriate structure (Ethernet or WLAN)                      
  memcpy(settings->m_mode.bits.ethernet ? settings->m_ethernetSettings.m_IpAddress                
                                        : settings->m_wlanSettings.m_IpAddress,
          newIPAddress, 4);
    
  pref.begin("IP Address", false);                                                                // Start the preferences to save to the flash
  
  pref.putBytes("IP_Address_" , newIPAddress, 4);                                                 // Write 4 bytes to the flash

  pref.end();                                                                                     // End the preferences
}

// Function to read the changed IP address from the Flash
void readIPAddress(network_settings_type *settings){
  pref.begin("IP Address", false);
  
  pref.getBytes("IP_Address_",settings->m_ethernetSettings.m_IpAddress, 4);                       // Read 4 Bytes from the flash

  pref.end();
}

// Function to change the subnet mask
void changeSubnetMask(network_settings_type *settings, uint8_t *newSubnetMask) {
    // Copy the new subnet mask to the appropriate structure (Ethernet or WLAN)
    memcpy(settings->m_mode.bits.ethernet ? settings->m_ethernetSettings.m_Subnet
                                          : settings->m_wlanSettings.m_Subnet,
           newSubnetMask, 4);
    
  pref.begin("Subnet Mask", false);

  pref.putBytes("Subnet_Mask_" , newSubnetMask, 4);

  pref.end();
}

// Function to read the changed IP address
void readSubnetMask(network_settings_type *settings){
  pref.begin("Subnet Mask", false);
  
  pref.getBytes("Subnet_Mask_",settings->m_ethernetSettings.m_Subnet, 4);

  pref.end();
}

// Function to change the gateway settings
void changeGateway(network_settings_type *settings, uint8_t *newGateway) {
    // Copy the new gateway settings to the appropriate structure (Ethernet or WLAN)
    memcpy(settings->m_mode.bits.ethernet ? settings->m_ethernetSettings.m_Gateway
                                          : settings->m_wlanSettings.m_Gateway,
           newGateway, 4);
    
  pref.begin("Gateway", false);
  
  pref.putBytes("Gateway_" , newGateway, 4);

  pref.end();
}

// Function to read the changed IP address
void readGateway(network_settings_type *settings){
  pref.begin("Gateway", false);
  
  pref.getBytes("Gateway_",settings->m_ethernetSettings.m_Gateway, 4);

  pref.end();
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







