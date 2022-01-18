/**
 * @file EEPROManager.h
 * @author Larry Colvin (pclabtools@projectcolvin.com)
 * @brief Library header file for the EEPROManager Arduino library
 * @version 0.1
 * @date 2022-01-08
 * 
 * @copyright Copyright PCLabTools(c) 2022
 * 
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <CRC.h>

#ifndef EEPROM_MAX_WRITES
  #define EEPROM_MAX_WRITES 100000
#endif

#ifndef EEPROManager_h

  #define EEPROManager_h

  template <class T> class EEPROManager 
  {
    public:
      EEPROManager(T *MEMORY, uint16_t KEY = 0x0001);
      uint32_t update();
             
     private:
      void initialise();
      uint8_t locate();
      T *_MEMORY;
      struct EEPROMEntry
      {
        uint16_t KEY;
        uint8_t CRC8;
        uint32_t WRITE_COUNT;
        uint16_t LENGTH;
        T DATA;
        uint32_t CRC32;
      } _ENTRY;
      uint16_t _ADDRESS = 0;
  };

#endif

  template <class T> EEPROManager<T>::EEPROManager(T *MEMORY, uint16_t KEY)
  {
    _MEMORY = MEMORY;
    _ENTRY.KEY = KEY;
    _ENTRY.DATA = *MEMORY;
    initialise();
    if (locate())
    {
      // Entry found: read EEPROMEntry from EEPROM
      EEPROM.get(_ADDRESS, _ENTRY);
      *MEMORY = _ENTRY.DATA;
    }
    else
    {
      // Uninitialised space: write EEPROMEntry to EEPROM
      EEPROM.put(_ADDRESS, _ENTRY);
    }
  }

  template <class T> void EEPROManager<T>::initialise()
  {
    _ENTRY.CRC8 = crc8(static_cast<uint8_t*>(static_cast<void*>(&_ENTRY.KEY)),sizeof(uint8_t));
    _ENTRY.WRITE_COUNT = 1;
    _ENTRY.LENGTH = sizeof(T);
    _ENTRY.CRC32 = crc32(static_cast<uint8_t*>(static_cast<void*>(&_ENTRY.DATA)),sizeof(T));
  }

  template <class T> uint8_t EEPROManager<T>::locate()
  {
    // Set ADDRESS and return if space is valid
    uint8_t validSpace;
    while (_ADDRESS < EEPROM.length())
    {
      // Look for EEPROMEntry: check if KEY valid in EEPROM
      uint16_t EEPROMKey;
      EEPROM.get(_ADDRESS, EEPROMKey);
      uint8_t EEPROMCRC8;
      EEPROM.get(_ADDRESS + 2, EEPROMCRC8);
      if (crc8(static_cast<uint8_t*>(static_cast<void*>(&EEPROMKey)),sizeof(uint8_t)) == EEPROMCRC8)
      {
        // Valid EEPROMEntry: read header EEPROMEntry information and check if matching KEY
        uint32_t EEPROMCount;
        EEPROM.get(_ADDRESS + 3, EEPROMCount);
        uint16_t EEPROMLength;
        EEPROM.get(_ADDRESS + 7, EEPROMLength);
        if (EEPROMKey == _ENTRY.KEY && EEPROMCount < EEPROM_MAX_WRITES)
        {
          // Matching KEY and WRITE_COUNT within limits:return 1
          validSpace = 1;
          break;          
        }
        else
        {
          // KEY valid but not matching or WRITE_COUNT exceeds maximum: move to next EEPROMEntry address
          _ADDRESS += EEPROMLength + 13;
        }
      }
      else
      {
        // Invliad space: return 0
        validSpace = 0;
        break;
      }
    }
    return validSpace;
  }

  template <class T> uint32_t EEPROManager<T>::update()
  {
    // Compare MEMORY CRC32 to ENTRY CRC32
    if (crc32(static_cast<uint8_t*>(static_cast<void*>(_MEMORY)),sizeof(T)) == _ENTRY.CRC32)
    {
      // Data matches: do nothing
      return 0;
    }
    else
    {
      // Data has changed: write new data to EEPROM
      _ENTRY.WRITE_COUNT++;
      _ENTRY.DATA = *_MEMORY;
      _ENTRY.CRC32 = crc32(static_cast<uint8_t*>(static_cast<void*>(&_ENTRY.DATA)),sizeof(T));
      EEPROM.put(_ADDRESS, _ENTRY);
      if (_ENTRY.WRITE_COUNT >= EEPROM_MAX_WRITES)
      {
        // Write count has been exceeded: locate uninitialised space for new EEPROMEntry
        locate();
        if (_ADDRESS < (EEPROM.length() - 13 - sizeof(T)))
        {
          // Space left in EEPROM: write data to EEPROM
          _ENTRY.WRITE_COUNT = 1;
          EEPROM.put(_ADDRESS, _ENTRY);
          return _ENTRY.WRITE_COUNT;
        }
        else
        {
          // No space left in EEPROM: throw exception
          return 0xFFFFFFFF;
        }
      }
      else
      {
        // Write count is within limits: write new data to EEPROMEntry
        EEPROM.put(_ADDRESS, _ENTRY);
        return _ENTRY.WRITE_COUNT;
      }
    }
  }