#pragma once
#include "Arduino.h"
#include <EEPROM.h>
#include <string.h>
struct Block_data
{
    uint32_t dataBegin;
    uint32_t dataEnd;
    uint32_t statusBegin;
    uint32_t statusEnd;
    uint32_t nextWrite;
    uint32_t statusLength;
    uint32_t nextRead;
    String getDebugData();
};

template <class T>
class __EEPROM_block
{
    uint32_t dataBegin;
    uint32_t dataEnd;
    uint32_t statusBegin;
    uint32_t statusEnd;
    uint32_t statusLength;
    uint32_t getNextWritePosition();
    uint32_t getNextReadPosition();
    uint32_t getNextStatusValue(uint32_t nextWrite);

public:
    void begin(uint32_t _dataBegin, uint32_t _dataEnd, uint32_t _statusBegin, uint32_t _statusEnd);
    void put(T data);
    void get(T &data);
    void getBlockInfo(Block_data &info);
};

template <class T, uint16_t amountOfVariables>
class EEPROMwl
{
    __EEPROM_block<T> data[amountOfVariables];
    void distributeUniformly(uint32_t beginAddress, uint32_t totalSpaceToAllocate);

public:
    EEPROMwl(uint32_t beginAddress, uint32_t totalSpaceToAllocate);
    void get(uint16_t idx,T &_data);
    void put(uint16_t idx, T _data);
    void getBlockInfo(uint16_t idx, Block_data &info);
};

// DEFINITIONS

#include "EEPROMwl-impl.h"