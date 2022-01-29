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

/**
 * @class EEPROManager
 * 
 * @brief Class library to facilitate management of EEPROM entries through client defined structs
 * 
 */
#ifndef EEPROManager_h

  #define EEPROManager_h
  
  template <class T> class EEPROManager 
  {
    public:
      EEPROManager(T *MEMORY, uint16_t KEY = 0x0001);   // Constructor which sets the EEPROM ENTRY unique KEY and binds the MEMORY
      uint32_t update();                                // Updates the EEPROM ENTRY if the MEMORY has changed since last check
             
    private:
      void initialise();                                // Initialises the CRC8, WRITE_COUNT, LENGTH and CRC32
      uint8_t locate();                                 // Locates a valid EEPROM ENTRY matching MEMORY or uninitialised space ready for writing
      void write();                                     // Writes the current MEMORY into the EEPROM ENTRY at the current ADDRESS
      void read();                                      // Reads the current EEPROM ENTRY at the current ADDRESS into MEMORY
      
      uint16_t _ADDRESS = 0;                            // Current EEPROM ENTRY starting ADDRESS
      T *_MEMORY;                                       // Pointer to MEMORY struct which is monitored for changes
      uint16_t _ENTRY_KEY;                              // Unique KEY used for identifying EEPROM ENTRY
      uint8_t _ENTRY_CRC8;                              // CRC8 used to check EEPROM ENTRY validity
      uint32_t _ENTRY_WRITE_COUNT;                      // Current EEPROM ENTRY WRITE_COUNT
      uint16_t _ENTRY_LENGTH;                           // LENGTH of the EEPROM ENTRY data
      uint32_t _ENTRY_CRC32;                            // CRC32 used to check EEPROM ENTRY validity
  };

#endif

/**
 * @brief Construct a new EEPROManager<T>::EEPROManager object
 * 
 * @tparam T 
 * @param MEMORY 
 * @param KEY 
 */
template <class T> EEPROManager<T>::EEPROManager(T *MEMORY, uint16_t KEY)
{
  _MEMORY = MEMORY;
  _ENTRY_KEY = KEY;
  initialise();
  if (locate())
  {
    // Entry found: read EEPROMEntry from EEPROM
    read();
  }
  else
  {
    // Uninitialised space: write EEPROMEntry to EEPROM
    write();
  }
}

/**
 * @brief 
 * 
 * @tparam T 
 */
template <class T> void EEPROManager<T>::initialise()
{
  _ENTRY_CRC8 = crc8(static_cast<uint8_t*>(static_cast<void*>(&_ENTRY_KEY)),sizeof(uint8_t));
  _ENTRY_WRITE_COUNT = 1;
  _ENTRY_LENGTH = sizeof(T);
  _ENTRY_CRC32 = crc32(static_cast<uint8_t*>(static_cast<void*>(_MEMORY)),sizeof(T));
}

/**
 * @brief 
 * 
 * @tparam T 
 * @return uint8_t 
 */
template <class T> uint8_t EEPROManager<T>::locate()
{
  // Set ADDRESS and return if space is valid
  uint8_t validSpace = 0;
  while (_ADDRESS < EEPROM.length())
  {
    // Look for EEPROMEntry: check if KEY valid in EEPROM
    uint16_t EEPROMKey;
    EEPROM.get(_ADDRESS, EEPROMKey);
    uint8_t EEPROMCRC8;
    EEPROM.get(_ADDRESS + sizeof(_ENTRY_KEY), EEPROMCRC8);
    if (crc8(static_cast<uint8_t*>(static_cast<void*>(&EEPROMKey)),sizeof(uint8_t)) == EEPROMCRC8)
    {
      // Valid EEPROMEntry: read header EEPROMEntry information and check if matching KEY
      uint32_t EEPROMCount;
      EEPROM.get(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8), EEPROMCount);
      uint16_t EEPROMLength;
      EEPROM.get(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof(_ENTRY_WRITE_COUNT), EEPROMLength);
      if (EEPROMKey == _ENTRY_KEY && EEPROMCount < EEPROM_MAX_WRITES)
      {
        // Matching KEY and WRITE_COUNT within limits:return 1
        validSpace = 1;
        break;          
      }
      else
      {
        // KEY valid but not matching or WRITE_COUNT exceeds maximum: move to next EEPROMEntry address
        _ADDRESS += EEPROMLength + + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT) + sizeof(_ENTRY_LENGTH) + sizeof(_ENTRY_CRC32);
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

/**
 * @brief 
 * 
 * @tparam T 
 * @return uint32_t 
 */
template <class T> uint32_t EEPROManager<T>::update()
{
  // Compare MEMORY CRC32 to ENTRY CRC32
  if (crc32(static_cast<uint8_t*>(static_cast<void*>(_MEMORY)),sizeof(T)) == _ENTRY_CRC32)
  {
    // Data matches: do nothing
    return 0;
  }
  else
  {
    // Data has changed: write new data to EEPROM
    _ENTRY_WRITE_COUNT++;
    _ENTRY_CRC32 = crc32(static_cast<uint8_t*>(static_cast<void*>(_MEMORY)),sizeof(T));
    EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8), _ENTRY_WRITE_COUNT);
    EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT) + sizeof(_ENTRY_LENGTH), *_MEMORY);
    EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT) + sizeof(_ENTRY_LENGTH) + sizeof(T), _ENTRY_CRC32);
    if (_ENTRY_WRITE_COUNT >= EEPROM_MAX_WRITES)
    {
      // Write count has been exceeded: locate uninitialised space for new EEPROMEntry
      locate();
      if (_ADDRESS < (EEPROM.length() - (sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT) + sizeof(_ENTRY_LENGTH) + sizeof(T) + sizeof(_ENTRY_CRC32))))
      {
        // Space left in EEPROM: write data to EEPROM
        _ENTRY_WRITE_COUNT = 1;
        write();
        return _ENTRY_WRITE_COUNT;
      }
      else
      {
        // No space left in EEPROM: throw exception
        return 0xFFFFFFFF;
      }
    }
    else
    {
      // Write count is within limits: return write count
      return _ENTRY_WRITE_COUNT;
    }
  }
}

/**
 * @brief 
 * 
 * @tparam T 
 */
template <class T> void EEPROManager<T>::write()
{
  EEPROM.put(_ADDRESS, _ENTRY_KEY);
  EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY), _ENTRY_CRC8);
  EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8), _ENTRY_WRITE_COUNT);
  EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT), _ENTRY_LENGTH);
  EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT) + sizeof(_ENTRY_LENGTH), *_MEMORY);
  EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT) + sizeof(_ENTRY_LENGTH) + sizeof(T), _ENTRY_CRC32);
}

/**
 * @brief 
 * 
 * @tparam T 
 */
template <class T> void EEPROManager<T>::read()
{
  EEPROM.get(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8), _ENTRY_WRITE_COUNT);
  EEPROM.get(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT) + sizeof(_ENTRY_LENGTH), *_MEMORY);
  EEPROM.get(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT) + sizeof(_ENTRY_LENGTH) + sizeof(T), _ENTRY_CRC32);
}