#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "EEPROMwl.h"
#include <string.h>

/**
 * @brief Returns an address in status partition of where the next write should occur.
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @return uint32_t Address in status partition of where the next write should occur.
 */
template <class T>
uint32_t __EEPROM_block<T>::getNextWritePosition()
{
    if ((uint8_t)EEPROM.read(status_begin) != (uint8_t)(EEPROM.read(status_end) + 1)) // special case: moving thorugh border
    {
        return status_begin;
    }

    for (uint32_t i = status_begin + 1; i <= status_end; i++) 
    {
        if ((uint8_t)EEPROM.read(i) != (uint8_t)(EEPROM.read(i - 1) + 1))   // normal case, if current not equal to previous +1 this is where we should write
            return i;                                                       // if current cell does not equal to previous + 1 return index
    }
    //FIXME:Undefined behavior possible, return first byte by default or raise exception?
}

/**
 * @brief Returns an index in data partition that should be read next
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @return uint32_t Index in data partition that should be read now
 */
template <class T>
uint32_t __EEPROM_block<T>::getNextReadPosition()
{
    uint32_t nextWrite = getNextWritePosition();
    if (nextWrite == status_begin)
        return (status_begin - sizeof(T)); // if next write would occur on the first cell read T last bytes
    else
        return ((nextWrite - status_begin - 1) * sizeof(T)) + data_begin; // normal scenario
}

/**
 * @brief Returns next status value that should be written.
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @param nextWrite 
 * @return uint32_t Next status value that should be written.
 */
template <class T>
uint32_t __EEPROM_block<T>::getNextStatusValue(uint32_t nextWrite)
{
    if (nextWrite == status_begin)
        return (uint8_t)(EEPROM.read(status_end) + 1);
    else
        return (uint8_t)(EEPROM.read(nextWrite - 1) + 1);
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
void __EEPROM_block<T>::put(T data) //this function is used internally to put data in single block
{
    uint32_t write_pos = getNextWritePosition();
    // Serial.println(write_pos);
    EEPROM.put(((write_pos - status_begin) * sizeof(T)) + data_begin, data); // write data segment
    // Serial.println((write_pos - status_begin)*sizeof(T));
    EEPROM.write(write_pos, getNextStatusValue(write_pos)); // write status segment
}

template <class T>
void __EEPROM_block<T>::get(T &data)
{
    // Serial.println(getNextReadPosition());
    EEPROM.get(getNextReadPosition(), data); // we are using this pointer because otherwise compiler throws "There are no arguments that depend on a template parameter"
    // Serial.println(data);
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

/**
 * @brief Divide `totalSpaceToAllocate` bytes across `amountOfVariables` partitions. 
 *  
 * Each partition consists of `status` and `data` partitions, where data contains actual data 
 * and status contains metadata required to guarantee we read/write in correct places.
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @tparam amountOfVariables Amount of variables of type `T` that we will store inside EEPROM.
 * @param beginAddress First byte of object.
 * @param totalSpaceToAllocate Total amount of bytes available for all variables.
 */
template <class T, uint16_t amountOfVariables>
void EEPROMwl<T, amountOfVariables>::distributeUniformly(uint32_t beginAddress, uint32_t totalSpaceToAllocate)
{
    //Whole eeprom memory assignedfor us should be divided into amountOfVariables blocks, each divided into 2 partitions - status and data partition
    uint32_t onePartitionSize = totalSpaceToAllocate / amountOfVariables; //Total size of one block equals total space assigned divided by amount of variables // 256
    uint32_t statusBufferSize = onePartitionSize / (sizeof(T) + 1);       //Size of one block is divided by (size of variable stored + 1). This is length of status partition// 51,2
    uint32_t dataBufferSize = statusBufferSize * sizeof(T);               //204,8
    //Generally, we divide one block of data (assigned to one variable) in proportion 1:n, where n is sizeof(variable)
    //This leaves us with an example for uint32_t as variable:
    //Status length = (block length) / (sizeof(uint32_t) +1 ) = (block_length) / 5
    //Data length = (status length) * sizeof(uint32_t) = (status_length) * 4

    for (uint16_t i = 0; i < amountOfVariables; i++) //Let's calculate border values for partitions in each block
    //Comments below contain example values of first block of data when 1024B eeprom is being assigned for 4 x 4 bytes values.
    {
        uint32_t data_begin = beginAddress + i * onePartitionSize; // dataBegin address     0
        uint32_t data_end = data_begin + dataBufferSize - 1;       // dataEnd address       203
        uint32_t status_begin = data_end + 1;                      // statusBegin address;  204
        uint32_t status_end = status_begin + statusBufferSize - 1; // statusEnd address;    254
        data[i].begin(data_begin, data_end, status_begin, status_end); //Initialize internal _EEPROM_block structure that keeps the data
    }
}

/**
 * @brief Construct a new EEPROMwl<T, amountOfVariables>::EEPROMwl object
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @tparam amountOfVariables Amount of variables of type `T` that we will store inside EEPROM.
 * @param beginAddress First byte of object.
 * @param totalSpaceToAllocate Total amount of bytes available for all variables.
 */
template <class T, uint16_t amountOfVariables>
EEPROMwl<T, amountOfVariables>::EEPROMwl(uint32_t beginAddress, uint32_t totalSpaceToAllocate)
{
    distributeUniformly(beginAddress, totalSpaceToAllocate);
}

/**
 * @brief 
 * 
 * @tparam T 
 * @tparam amountOfVariables 
 * @param idx 
 * @param _data 
 */
template <class T, uint16_t amountOfVariables>
void EEPROMwl<T, amountOfVariables>::get(uint16_t idx, T &_data)
{
    data[idx].get(_data);
}

/**
 * @brief saves `_data` custom object to EEPROM.
*/
template <class T, uint16_t amountOfVariables>
void EEPROMwl<T, amountOfVariables>::put(uint16_t idx, T _data)
{
    data[idx].put(_data);
}

/**
 * @brief Fills `info` with underlying block info.
*/
template <class T, uint16_t amountOfVariables>
void EEPROMwl<T, amountOfVariables>::getBlockInfo(uint16_t idx, Block_data &info)
{
    data[idx].getBlockInfo(info);
}

//-----------------------------------------------
// String Block_data::printResult(){
//     String result = "Data begin:" + (String)data_begin +'\n' + \
//                     "Data end:" + (String)data_end + '\n' + \
//                     "Status begin:" + (String)status_begin + '\n' +\
//                     "Status end:" + (String)status_end + '\n' +\
//                     "Next Write:" + (String)next_write + '\n' +\
//                     "Next Read:"+ (String)next_read;
//     return result;
// }