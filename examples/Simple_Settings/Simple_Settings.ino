/**
 * @file SimpleSettings.ino
 * @author Larry Colvin (pclabtools@projectcolvin.com)
 * @brief EEPROM manager for simplifying management of settings and memory
 * @version 0.1
 * @date 2025-02-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <Arduino.h>

//#define BOARD_ESP       // Uncomment for ESP32 boards
//#define BOARD_RP2040    // Uncomment for Raspberry Pico boards

#include <EEPROManager.h>

struct Settings_t
{
  char ID[32] = "Default ID";
  uint16_t Version = 0x0001;
} Settings;

EEPROManager<Settings_t> manageSettings(&Settings, 0x0001);

/**
 * @brief Initial Setup
 * 
 * @details When the device is booted the following actions are performed:
 *  -# Serial communication is begun at baud rate 9600
 *  -# Execution is halted to wait for a serial connection
 *  -# Settings are loaded from memory (EEPROM/flash)
 *  -# Device ID and Version are printed to the serial monitor
 * 
 */
void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    // Do nothing and wait for serial connection
    delay(1000);
  }
  Serial.println("Initialising");
  manageSettings.synchronise();     // This is done for flash memory based boards
  Serial.println("Loaded Settings from EEPROM");
  Serial.print("Device ID: ");
  Serial.println(Settings.ID);
  Serial.print("Version: ");
  Serial.println(Settings.Version, HEX);
}

/**
 * @brief Main Application Loop
 * 
 * @details When the device has been booted and is running normally:
 *  -# If Serial incoming bytes are detected read string and update "Settings.ID"
 *  -# If changes are detected to "Settings" they are written to memory
 * 
 */
void loop()
{
  // Read incoming strings
  char inChar;
  char newID[32] = "";
  while (Serial.available())
  {
    inChar = Serial.read();
    if (inChar == '\n')
    {
      strcpy(Settings.ID, newID);
      break;
    }
    strcat(newID, inChar);
  }

  // Update memory if changed
  manageSettings.update();
}