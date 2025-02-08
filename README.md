# EEPROManager

EEPROM management library designed for Arduino, Teensy, Raspberry Pico and ESP32 boards using the Arduino Framework. **Compatible with FreeRTOS**.

## Navitagation

- [EEPROManager](#eepromanager)
  - [Navitagation](#navitagation)
  - [Description](#description)
    - [How it Works](#how-it-works)
    - [Memory Structure](#memory-structure)
  - [Installation](#installation)
    - [Arduino](#arduino)
      - [Dependencies](#dependencies)
    - [PlatformIO](#platformio)
  - [Usage](#usage)
    - [General Usage](#general-usage)
    - [Functions](#functions)
      - [EEPROManager()](#eepromanager-1)
      - [uint32\_t update()](#uint32_t-update)
      - [void synchronise()](#void-synchronise)
      - [void reset()](#void-reset)
      - [void wipe()](#void-wipe)
      - [void print(Stream\* stream)](#void-printstream-stream)
      - [void force()](#void-force)
  - [License](#license)

## Description

The purpose of this library is to simplify storing a variety of data on non-volatile memory while ensuring the memory does not get written to one area excessively without a means of *backing up* the data to a lesser used area (prolonging life of storage).

### How it Works

This library creates a **memory chunk** which is designed to store the desired data with some meta data to ensure validity and no corruption.

This memory chunk has a unique identifier to allow multiple chunks to work on a single target. This identifier (key) is cyclic reduncency checked to make sure it is not some erroneous byte.

Next the write count is stored to ensure no excessive writing to the chunk. If writing limits are exceeded a chunk further up in memory is used.

The desired data that needs to be stored can be just about anything. This could be useful for tracking a particular set of metrics, or storing settings for when the device is power cycled.

A routine that is called will check live running values of the data structure and will automatically store and handle safety of the stored data.

### Memory Structure

Below is the structure for a chunk of memory storing data. Multiple chunks can live on a single target.

| key | crc8 | write_count | length | storage | crc32 |
| - | - | - | - | - | - |
| uint16_t | uint8_t | uint32_t | uint16_t | < any > | uint32_t |

- key: Unique key to distinguish different storage areas
- crc8: CRC of key to check valid memory chunk
- write_count: Count of writes to memory chunk, if exceeds maximum memory chunk is invalid
- length: Length of storage area in bytes
- storage: Desired stored data, can be any size/design
- crc32: CRC of memory chunk to check validity

## Installation

### Arduino

To install this library follow the standard method of installing libraries, either using the library manager or a downloaded zip file of this repository.

**Make sure to install the below listed dependencies as this library depends upon them.**

For more information on how to install libraries please visit [Installing Additional Arduino Libraries](https://www.arduino.cc/en/guide/libraries "arduino.cc").

#### Dependencies

| Name | Git Link | ZIP file |
| - | - | - |
|CRC (Rob Tillaart) | https://github.com/RobTillaart/CRC.git | [Download zip file](https://github.com/RobTillaart/CRC/archive/refs/heads/master.zip) |

### PlatformIO

To include this library and its dependencies simply add the following to the "platformio.ini" file:
```
[env:my_build_env]
framework = arduino
lib_deps = 
  https://github.com/PCLabTools/EEPROManager.git
```

## Usage

### General Usage

1. Include the EEPROManager library:

``` cpp
#include <EEPROManager.h>
```

2. Define a struct that will contain the values that need to be stored:

``` cpp
// Example:
struct Memory_t
{
  bool important_flag = false;
  int some_number = 0;
  ...
}
```

3. Create an instance of that struct and create an EEPROManager object assigned to manage the struct:

``` cpp
Memory_t Memory

EEPROManager<Memory_t> manageMemory(&Memory, 0x0001);
```

4. Regularly run the update command to compare last known values in non volatile memory to the current values in memory. If these are different it will update the non volatile memory with the latest values.

``` cpp
loop()
{
  // Other user code
  manageMemory.update();
}
```

### Functions

#### EEPROManager()

Constructor used to create the management object responsible for updating the memory. This constructor is a template and can be assigned the type of the struct that the manager is responsible for storing.

Return: none

#### uint32_t update()

Running this function will compare the current values in the managed struct to the values stored in non-volatile memory and if different will update the stored values.

Return: 1 = successful , 0 = error

#### void synchronise()

Some boards use flash memory instead of an actual EEPROM so must initialise the flash during setup (example Rasberry Pico and ESP32).

Return: none

#### void reset()

Resets the values stored in non-volatile memory to the default values the firmware would provide on first use.

Return: none

#### void wipe()

Completely wipes and resets the EEPROM back to original state before storing the default values.

Return: none

#### void print(Stream* stream)

This function will print the memory in human-readable HEX to the desired Stream.

stream: Pointer to stream object for printing (example Serial or WiFi)

Return: none

#### void force()

This function forcibly writes the current values to non-volatile memory without performing the comparison step.

Return: none

## License

[MIT License](https://github.com/PCLabTools/EEPROManager/blob/main/LICENSE)
