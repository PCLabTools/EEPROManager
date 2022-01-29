/**
 * @file SimpleExample.ino
 * @author Larry Colvin (pclabtools@projectcolvin.com)
 * @brief EEPROM manager for simplifying management of settings and memory
 * @version 0.1
 * @date 2022-01-15
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <Arduino.h>
#include <EEPROManager.h>

struct Settings
{
  String ID = "Default ID";
  uint16_t Version = 0x0001;
} DeviceSettings;

EEPROManager<Settings> manageDeviceSettings(&DeviceSettings, 0x0001);

/**
 * @brief Initial Setup
 * 
 * @details When the device is booted the following actions are performed:
 *  -# Serial communication is begun at desired baud rate 9600
 * 
 */
void setup()
{
  Serial.being(9600);
}

/**
 * @brief Main Application Loop
 * 
 * @details description
 * 
 */
void loop()
{
  manageDeviceSettings.update();
  // read incoming command
  // change setting struct
}