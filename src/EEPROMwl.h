#pragma once
#include "Arduino.h"
#include <EEPROM.h>
#include <string.h>

typedef uint16_t eesize_t; //Modify this if you want to store data on more than 64KB of eeprom (uint16_t can address max 64kB)

struct Block_data
{
    eesize_t dataBegin;
    eesize_t dataEnd;
    eesize_t statusBegin;
    eesize_t statusEnd;
    eesize_t statusLength;
    eesize_t nextWrite;
    eesize_t nextRead;
    String getDebugData();
};

template <class T>
class __EEPROM_block
{
    eesize_t dataBegin;
    eesize_t dataEnd;
    eesize_t statusBegin;
    eesize_t statusEnd;
    eesize_t statusLength;
    eesize_t getNextWritePosition();
    eesize_t getNextReadPosition();
    eesize_t getNextStatusValue(eesize_t nextWrite);

public:
    void begin(eesize_t _dataBegin, eesize_t _dataEnd, eesize_t _statusBegin, eesize_t _statusEnd);
    void put(T data);
    void get(T &data);
    void getBlockInfo(Block_data &info);
};

template <class T, uint16_t amountOfVariables>
class EEPROMwl
{
    __EEPROM_block<T> data[amountOfVariables];
    void distributeUniformly(eesize_t beginAddress, eesize_t totalSpaceToAllocate);

public:
    EEPROMwl(eesize_t beginAddress, eesize_t totalSpaceToAllocate);
    void get(uint16_t idx, T &_data); // TODO: Implement simple assignment as below
    // T &get(uint16_t idx);
    void put(uint16_t idx, T _data);
    void getBlockInfo(uint16_t idx, Block_data &info);
};

// DEFINITIONS

#include "EEPROMwl-impl.h"