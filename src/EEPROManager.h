/**
 * @file EEPROManager.h
 * @author Larry Colvin (pclabtools@colvin.app)
 * @brief Library header file for the EEPROManager Arduino library
 * @version 0.1
 * @date 2025-02-08
 * 
 * @copyright Copyright PCLabTools(c) 2025
 * 
 */

#ifndef EEPROManager_h
#define EEPROManager_h

#include <EEPROM.h>
#include <CRC.h>

#ifndef EEPROM_MAX_WRITES
#define EEPROM_MAX_WRITES 100000
#endif

#ifndef BOARD_RP2040_FLASH_SIZE
#define BOARD_RP2040_FLASH_SIZE 4096
#endif

#ifndef BOARD_ESP_FLASH_SIZE
#define BOARD_ESP_FLASH_SIZE 512
#endif

/**
 * @class EEPROManager
 * 
 * @brief Class library to facilitate management of EEPROM entries through client defined structs
 * 
 */
template <class T> class EEPROManager 
{
  public:
    EEPROManager(T *MEMORY, uint16_t KEY = 0x0001);   // Constructor which sets the EEPROM ENTRY unique KEY and binds the MEMORY
    uint32_t update();                                // Updates the EEPROM ENTRY if the MEMORY has changed since last check
    void synchronise();                               // Scynhronises the EEPROM similar to the constructor in case the constructor method is not supported
    void reset();                                     // Resets the memory to default firmware values
    void wipe();                                      // Resets the entire EEPROM back to default data (0xFF, 0xFF...)
    void print(Stream* stream);                       // Dumps the memory to the assigned stream for use with printing and debugging
    void force();                                     // Forces an update of the EEPROM
            
  private:
    void begin();                                     // Function used to initialise the EEPROM
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

/*******************
 * PUBLIC METHODS  *
 *******************/

/**
 * @brief Construct a new EEPROManager<T>::EEPROManager object
 * 
 * @tparam MEMORY Pointer to object (struct) to manager
 * @param KEY Unique identifier key for entry location in EEPROM
 */
template <class T> EEPROManager<T>::EEPROManager(T *MEMORY, uint16_t KEY)
{
  _MEMORY = MEMORY;
  _ENTRY_KEY = KEY;
  #if !defined(BOARD_RP2040) || !defined(BOARD_ESP)
  begin();
  #endif
}

/**
 * @brief Synchronise settings for flash based EEPROMs
 * 
 */
template <class T> void EEPROManager<T>::synchronise()
{
  #if defined BOARD_RP2040
  EEPROM.begin(BOARD_RP2040_FLASH_SIZE);
  begin();
  #elif defined BOARD_ESP
  EEPROM.begin(BOARD_ESP_FLASH_SIZE);
  begin();
  #endif
}

/**
 * @brief Resets the memory stored values to the default firmware values
 * 
 */
template <class T> void EEPROManager<T>::reset()
{
  T default_values = new T;
  _MEMORY = default_values;
  write();
}

/**
 * @brief Resets the EEPROM by overwriting the values with 0xFF
 * 
 */
template <class T> void EEPROManager<T>::wipe()
{
  for (uint16_t i = 0 ; i < EEPROM.length() ; i++)
  {
    #ifdef BOARD_ESP
    byte currentByte = EEPROM.read(i);
    if (currentByte != 0xFF)
    {
      EEPROM.write(i, 0xFF);
    }
    #else
    EEPROM.update(i, 0xFF);
    #endif
  }
  #if defined(BOARD_RP2040) || defined(BOARD_ESP)
  EEPROM.commit();
  #endif
  begin();
}

/**
 * @brief Prints the EEPROM dump to the assigned stream for printing and debugging
 * 
 * @param stream Stream object pointer
 */
template <class T> void EEPROManager<T>::print(Stream* stream)
{
  for (uint16_t i=0; i<EEPROM.length(); i++)
  {
    stream->printf("%02X ", EEPROM.read(i));
  }
  stream->printf("\n");
}

/**
 * @brief Forces the EEPROM to be updated with the currently held values
 * 
 */
template <class T> void EEPROManager<T>::force()
{
  write();
}

/*******************
 * PRIVATE METHODS *
 *******************/

/**
 * @brief Used during construction to locate and initialise the EEPROM
 * 
 */
template <class T> void EEPROManager<T>::begin()
{
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
 * @brief Initialisation routine for EEPROM
 * 
 */
template <class T> void EEPROManager<T>::initialise()
{
  _ENTRY_CRC8 = crc8(static_cast<uint8_t*>(static_cast<void*>(&_ENTRY_KEY)),sizeof(uint8_t));
  _ENTRY_WRITE_COUNT = 1;
  _ENTRY_LENGTH = sizeof(T);
  _ENTRY_CRC32 = crc32(static_cast<uint8_t*>(static_cast<void*>(_MEMORY)),sizeof(T));
}

/**
 * @brief Used to locate current entry in EEPROM
 * 
 * @return uint8_t Address location of EEPROM entry
 */
template <class T> uint8_t EEPROManager<T>::locate()
{
  // Set ADDRESS and return if space is valid
  uint8_t validSpace = 0;
  while (_ADDRESS < EEPROM.length())
  {
    // Look for EEPROMEntry: check if KEY valid in EEPROM
    uint16_t EEPROMKey = 0;
    uint8_t EEPROMCRC8 = 0;
    EEPROM.get(_ADDRESS, EEPROMKey);
    EEPROM.get(_ADDRESS + sizeof(_ENTRY_KEY), EEPROMCRC8);
    if (crc8(static_cast<uint8_t*>(static_cast<void*>(&EEPROMKey)),sizeof(uint8_t)) == EEPROMCRC8)
    {
      // Valid EEPROMEntry: read header EEPROMEntry information and check if matching KEY
      uint32_t EEPROMCount = 0;
      uint16_t EEPROMLength = 0;
      EEPROM.get(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8), EEPROMCount);
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
 * @brief If values differ between the EEPROM and registered object (struct) the changed values are written to the EEPROM
 * 
 * @return uint32_t Entry write count
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
    #if defined(BOARD_RP2040) || defined(BOARD_ESP)
    EEPROM.commit();
    #endif
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
 * @brief Writes the EEPROM entry
 * 
 */
template <class T> void EEPROManager<T>::write()
{
  EEPROM.put(_ADDRESS, _ENTRY_KEY);
  EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY), _ENTRY_CRC8);
  EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8), _ENTRY_WRITE_COUNT);
  EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT), _ENTRY_LENGTH);
  EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT) + sizeof(_ENTRY_LENGTH), *_MEMORY);
  EEPROM.put(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT) + sizeof(_ENTRY_LENGTH) + sizeof(T), _ENTRY_CRC32);
  #if defined(BOARD_RP2040) || defined(BOARD_ESP)
  EEPROM.commit();
  #endif
}

/**
 * @brief Reads the EEPROM entry
 * 
 */
template <class T> void EEPROManager<T>::read()
{
  EEPROM.get(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8), _ENTRY_WRITE_COUNT);
  EEPROM.get(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT) + sizeof(_ENTRY_LENGTH), *_MEMORY);
  EEPROM.get(_ADDRESS + sizeof(_ENTRY_KEY) + sizeof(_ENTRY_CRC8) + sizeof (_ENTRY_WRITE_COUNT) + sizeof(_ENTRY_LENGTH) + sizeof(T), _ENTRY_CRC32);
}

#endif // EEPROManager_h