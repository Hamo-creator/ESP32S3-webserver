#include <stdint.h>
#ifndef NETWORK_H
#define NETWORK_H

#include <string>

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

extern user_settings_type users[12];



// Function to change the IP address
void changeIPAddress(network_settings_type *settings, uint8_t *newIPAddress);

// Function to read the changed IP address
void readIPAddress(network_settings_type *settings);

// Function to change the subnet mask
void changeSubnetMask(network_settings_type *settings, uint8_t *newSubnetMask);

// Function to read the changed IP address
void readSubnetMask(network_settings_type *settings);

// Function to change the gateway settings
void changeGateway(network_settings_type *settings, uint8_t *newGateway);

// Function to read the changed IP address
void readGateway(network_settings_type *settings);


// Function to create new user
void registerUser(const char* newUsername, const char* newPassword, int8_t newUserLevel);

// Function to delete a user
void deleteUser(const char* usernameToDelete);

int8_t getUserLevel(const char* user, const char* password);




#endif
