#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX

void sendATCommand(const char *cmd) {
  Serial.print("Sending: ");
  Serial.println(cmd);
  mySerial.println(cmd);  // Send AT command to Bluetooth module
  delay(500);  // Wait for the module to process

  // Read and print the response
  while (mySerial.available()) {
    Serial.write(mySerial.read());
  }
  Serial.println();  // Newline for readability
}

void setup() {
  Serial.begin(9600);
  delay(4000);  // Wait for the Serial Monitor to open

  mySerial.begin(9600);
  delay(1000);  // Short delay to ensure stability

  // Send AT commands in the required order
  sendATCommand("AT+RENEW");
  sendATCommand("AT+IMME1");
  sendATCommand("AT+SHOW1");
  sendATCommand("AT+COMP1");
  sendATCommand("AT+NOTI1");
  sendATCommand("AT+128B0");
  sendATCommand("AT+UUID0xFFE0");
  sendATCommand("AT+CHAR0xFFE1");
  sendATCommand("AT+ROLE1");
  sendATCommand("AT+DISC?");
  sendATCommand("AT+CO0780473BFE656");
}

void loop() {
  if (mySerial.available()) {
    Serial.write(mySerial.read());
  }
  if (Serial.available()) {
    mySerial.write(Serial.read());
  }
}