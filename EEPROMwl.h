#pragma once
#include "Arduino.h"
#include <EEPROM.h>
struct Block_data
{
    uint32_t data_begin;
    uint32_t data_end;
    uint32_t status_begin;
    uint32_t status_end;
    uint32_t status_length;
    uint32_t next_write;
    uint32_t next_read;
};

template <class T>
class __EEPROM_block
{
    uint32_t data_begin;
    uint32_t data_end;
    uint32_t status_begin;
    uint32_t status_end;
    uint32_t status_length;
    uint32_t getNextWritePosition();
    uint32_t getNextReadPosition();
    uint32_t getNextStatusValue(uint32_t nextWrite);

public:
    void begin(uint32_t _data_begin, uint32_t _data_end, uint32_t _status_begin, uint32_t _status_end);
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
    EEPROMwl(uint32_t beginAddress, uint32_t totalSpaceToAllocate, bool eraseExistingMemory);
    void get(uint16_t idx,T &_data);
    void put(uint16_t idx, T _data);
    void getBlockInfo(uint16_t idx, Block_data &info);
};

// DEFINITIONS

#include "EEPROMwl-impl.h"