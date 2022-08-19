#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "EEPROMwl.h"

template <class T>

uint32_t __EEPROM_block<T>::getNextWritePosition() //this should return an address in status partition of where the write should now occur
{
    if ((uint8_t)EEPROM.read(status_begin) != (uint8_t)(EEPROM.read(status_end)+1))
    {
        return status_begin;
    }

    for (uint32_t i = status_begin + 1; i <= status_end; i++)
    {
        if ((uint8_t)EEPROM.read(i) != (uint8_t)(EEPROM.read(i - 1) + 1))
            return i; // if current cell does not equal to previous + 1 return index
    }
}
template <class T>
uint32_t __EEPROM_block<T>::getNextReadPosition() //this returns an index in data partition that should be read now
{
    uint32_t nextWrite = getNextWritePosition();
    if (nextWrite == status_begin)
        return (status_begin-sizeof(T)); // if next write would occur on the first cell
    else
        return ((nextWrite - status_begin-1)*sizeof(T))+data_begin; // normal scenario
}
template <class T>
uint32_t __EEPROM_block<T>::getNextStatusValue(uint32_t nextWrite)
{
    if (nextWrite == status_begin)
        return (uint8_t)(EEPROM.read(status_end) + 1);
    else
        return (uint8_t)(EEPROM.read(nextWrite -1) + 1);
}

template <class T>
void __EEPROM_block<T>::begin(uint32_t _data_begin, uint32_t _data_end, uint32_t _status_begin, uint32_t _status_end)
{
    status_length = _status_end - _status_begin;
    data_begin = _data_begin;
    data_end = _data_end;
    status_begin = _status_begin;
    status_end = _status_end;
}
template <class T>
void __EEPROM_block<T>::put(T data)
{
    uint32_t write_pos = getNextWritePosition();
    //Serial.println(write_pos);
    EEPROM.put(((write_pos - status_begin)*sizeof(T))+data_begin, data);    // write data segment
    //Serial.println((write_pos - status_begin)*sizeof(T));
    EEPROM.write(write_pos, getNextStatusValue(write_pos)); // write status segment
}
template <class T>
void __EEPROM_block<T>::get(T &data)
{
    //Serial.println(getNextReadPosition());
    EEPROM.get(getNextReadPosition(), data); // we are using this pointer because otherwise compiler throws "There are no arguments that depend on a template parameter"
    //Serial.println(data);
}

template <class T>
void __EEPROM_block<T>::getBlockInfo(Block_data &info)
{
    info.data_begin = data_begin;
    info.data_end = data_end;
    info.status_begin = status_begin;
    info.status_end = status_end;
    info.status_length = status_length;
    info.next_write = getNextWritePosition();
    info.next_read = getNextReadPosition();
}


//------------------------------------------------------------



template <class T, uint16_t amountOfVariables>
void EEPROMwl<T, amountOfVariables>::distributeUniformly(uint32_t beginAddress, uint32_t totalSpaceToAllocate)
{
    uint32_t onePartitionSize = totalSpaceToAllocate / amountOfVariables; //256
    uint32_t statusBufferSize = onePartitionSize / (sizeof(T) + 1); //51,2
    uint32_t dataBufferSize = statusBufferSize * sizeof(T); // 204,8
    for (uint16_t i = 0; i < amountOfVariables; i++)
    {
        uint32_t data_begin = beginAddress + i * onePartitionSize; // dataBegin address     0
        uint32_t data_end = data_begin + dataBufferSize - 1;       // dataEnd address       203
        uint32_t status_begin = data_end + 1;                      // statusBegin address;  204
        uint32_t status_end = status_begin + statusBufferSize - 1; // statusEnd address;    254
        data[i].begin(data_begin, data_end, status_begin, status_end);
    }
}
template <class T, uint16_t amountOfVariables>
EEPROMwl<T, amountOfVariables>::EEPROMwl(uint32_t beginAddress, uint32_t totalSpaceToAllocate, bool eraseExistingMemory)
{
    if (eraseExistingMemory)
    {
        for (uint32_t i = beginAddress; i < beginAddress + totalSpaceToAllocate; i++)
        {
            EEPROM.update(i, 125);
        }
    }
    distributeUniformly(beginAddress, totalSpaceToAllocate);
}
template <class T, uint16_t amountOfVariables>
void EEPROMwl<T, amountOfVariables>::get(uint16_t idx,T &_data)
{
    data[idx].get(_data);
}
template <class T, uint16_t amountOfVariables>
void EEPROMwl<T, amountOfVariables>::put(uint16_t idx, T _data)
{
    data[idx].put(_data);
}

template <class T, uint16_t amountOfVariables>
void EEPROMwl<T, amountOfVariables>::getBlockInfo(uint16_t idx, Block_data &info)
{
    data[idx].getBlockInfo(info);
}