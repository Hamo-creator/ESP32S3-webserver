#ifndef UPDATEFIRMWARE_H
#define UPDATEFIRMWARE_H

#include <StreamString.h>
#include "network.h"
#include "EthernetWebServer.h"
#include <Ethernet_Generic.h>
#include "Update.h"
#include "LittleFS.h"


EthernetWebServer ethernetServer(80);


network_settings_type nst;
user_settings_type ust;
String htmlContent;




//----------------------------------------- Firmware Update Pages -----------------------------------------------------

void handleUpdate() {


  if (!ethernetServer.authenticate(ust.m_Benutzername, ust.m_Passwort))
    return ethernetServer.requestAuthentication();

  readIPAddress(&nst);
  readSubnetMask(&nst);
  readGateway(&nst);
  readUsername(&ust);
  readPassword(&ust);

  _ip_add = String(nst.m_ethernetSettings.m_IpAddress[0]) + "." + String(nst.m_ethernetSettings.m_IpAddress[1]) + "." + String(nst.m_ethernetSettings.m_IpAddress[2]) + "." + String(nst.m_ethernetSettings.m_IpAddress[3]);
  _subnet = String(nst.m_ethernetSettings.m_Subnet[0]) + "." + String(nst.m_ethernetSettings.m_Subnet[1]) + "." + String(nst.m_ethernetSettings.m_Subnet[2]) + "." + String(nst.m_ethernetSettings.m_Subnet[3]);
  _gateway = String(nst.m_ethernetSettings.m_Gateway[0]) + "." + String(nst.m_ethernetSettings.m_Gateway[1]) + "." + String(nst.m_ethernetSettings.m_Gateway[2]) + "." + String(nst.m_ethernetSettings.m_Gateway[3]);
  _username = ust.m_Benutzername;
  _password = ust.m_Passwort;

  String s = "";
   
  File file2 = LittleFS.open("/index.html", "r");
  if (file2) {
    s += file2.readString();
    file2.close();
  }
  ethernetServer.send(200, "text/html", s);
}
  


void handlePostUpdate() {
  if (!ethernetServer.authenticate(ust.m_Benutzername, ust.m_Passwort))
    return ethernetServer.requestAuthentication();

  if (Update.hasError()) {
    StreamString str;
    Update.printError(str);
    str;
    String s = "";
    s += "<h1>UPDATE ERROR! </h1>";
    s += str;
    // s += FPSTR(htmlclose);    //Problem
    ethernetServer.send(200, "text / html", s);

    Serial.println("Update Error!!!");
  }
  else {
    String s = "<h1>UPDATE SUCCESS</h1>";
    s += "<META http-equiv='refresh' content='10;URL=/'>Back to homepage...\n";
    // s += (htmlclose);    //Problem
    //ethernetServer.client().setNoDelay(true);
    ethernetServer.send(200, "text/html", s);

    Serial.println("Update Success!!!");
;
    delay(3000);

    ethernetServer.client().stop();
    ESP.restart();
  }
  
}

void handleFileUpload() {
  if (!ethernetServer.authenticate(ust.m_Benutzername, ust.m_Passwort))
    return ethernetServer.requestAuthentication();

  ethernetHTTPUpload& upload = ethernetServer.upload();
  String updateerror = "";
  if (upload.status == UPLOAD_FILE_START) {

    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace)) { //start with max available size
      StreamString str;
      Update.printError(str);
      updateerror = str;
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      StreamString str;
      Update.printError(str);
      updateerror = str;
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { //true to set the size to the current progress
      StreamString str;
      Update.printError(str);
      updateerror = str;
    }

    else if (upload.status == UPLOAD_FILE_ABORTED) {
      Update.end();;
    }
    yield();
  }
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


void handleDeleteUser(){
  if(!ethernetServer.authenticate(ust.m_Benutzername, ust.m_Passwort))
    return ethernetServer.requestAuthentication();
  
  String response;

  if(ethernetServer.hasArg("newUsername")){
    String username = ethernetServer.arg("newUsername");

    // Call the deleteUser function to delete the user
    deleteUser(username.c_str());

    // Send a response indicating success of deletion
    response = "<h1>User has been deleted</h1>";
    //response += "<META http-equiv='refresh' content='5;URL=/'>Back to homepage...\n";
    ethernetServer.send(200, "text/html", response);
  }
  else{
    // If the username parameter is missing, send a response indicating a bad request
    // response = "<h1>username parameter missing</h1>";
    // response += "<META http-equiv='refresh' content='5;URL=/'>Back to homepage...\n";
    ethernetServer.send(200);
    return;
  }
}


#endif
