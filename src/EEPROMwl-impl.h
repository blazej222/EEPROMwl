#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "EEPROMwl.h"
#include <string.h>

//TODO: Move this file to separate .cpp

/**
 * @brief Returns an address in status partition of where the next write should occur.
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @return eesize_t Address in status partition of where the next write should occur.
 */
template <class T>
eesize_t __EEPROM_block<T>::getNextWritePosition()
{
    if ((uint8_t)EEPROM.read(statusBegin) != (uint8_t)(EEPROM.read(statusEnd) + 1))   // special case: moving thorugh border
    {
        return statusBegin;
    }

    for (eesize_t i = statusBegin + 1; i <= statusEnd; i++) 
    {
        if ((uint8_t)EEPROM.read(i) != (uint8_t)(EEPROM.read(i - 1) + 1))               // normal case, if current not equal to previous +1 this is where we should write
            return i;                                                                   // if current cell does not equal to previous + 1 return index
    }
    //FIXME:Undefined behavior possible, return first byte by default or raise exception?
}

/**
 * @brief Returns an index in data partition that should be read next
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @return eesize_t Index in data partition that should be read now
 */
template <class T>
eesize_t __EEPROM_block<T>::getNextReadPosition()
{
    eesize_t nextWrite = getNextWritePosition();
    if (nextWrite == statusBegin)
        return (statusBegin - sizeof(T));                                  // if next write would occur on the first cell read T last bytes
    else
        return ((nextWrite - statusBegin - 1) * sizeof(T)) + dataBegin;   // normal scenario
}

/**
 * @brief Returns next status value that should be written.
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @param[in] nextWrite Address of next write in status partition
 * @return eesize_t Next status value that should be written.
 */
template <class T>
eesize_t __EEPROM_block<T>::getNextStatusValue(eesize_t nextWrite)
{
    if (nextWrite == statusBegin)
        return (uint8_t)(EEPROM.read(statusEnd) + 1);
    else
        return (uint8_t)(EEPROM.read(nextWrite - 1) + 1);
}

/**
 * @brief Initializes internal block partition borders with values from the constructor.
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @param[in] _dataBegin Address of first byte of data partition.
 * @param[in] _dataEnd Address of last byte of data partition.
 * @param[in] _statusBegin Address of first byte in status partition.
 * @param[in] _statusEnd Address of last byte in status partition.
 */
template <class T>
void __EEPROM_block<T>::begin(eesize_t _dataBegin, eesize_t _dataEnd, eesize_t _statusBegin, eesize_t _statusEnd)
{
    statusLength = _statusEnd - _statusBegin;
    dataBegin = _dataBegin;
    dataEnd = _dataEnd;
    statusBegin = _statusBegin;
    statusEnd = _statusEnd;
}

/**
 * @brief Internal function for putting data in a single block.
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @param[in] data Value of variable to store in EEPROM memory.
 */
template <class T>
void __EEPROM_block<T>::put(T data)
{
    eesize_t writePos = getNextWritePosition();
    // Serial.println(write_pos);
    EEPROM.put(((writePos - statusBegin) * sizeof(T)) + dataBegin, data);    // write data segment
    // Serial.println((write_pos - status_begin)*sizeof(T));
    EEPROM.write(writePos, getNextStatusValue(writePos));                     // write status segment
}

/**
 * @brief Retrieve saved data from EEPROM memory.
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @param[out] data This variable will be filled with data read from EEPROM memory.
 */
template <class T>
void __EEPROM_block<T>::get(T &data)
{
    // Serial.println(getNextReadPosition());
    EEPROM.get(getNextReadPosition(), data); // we are using this pointer because otherwise compiler throws "There are no arguments that depend on a template parameter"
    // Serial.println(data);
}

/**
 * @brief Retrieve internal block debug information.
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @param[out] info Internal block debug information
 */
template <class T>
void __EEPROM_block<T>::getBlockInfo(Block_data &info)
{
    info.dataBegin = dataBegin;
    info.dataEnd = dataEnd;
    info.statusBegin = statusBegin;
    info.statusEnd = statusEnd;
    info.statusLength = statusLength;
    info.nextWrite = getNextWritePosition();
    info.nextRead = getNextReadPosition();
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
 * @param[in] beginAddress First byte of object.
 * @param[in] totalSpaceToAllocate Total amount of bytes available for all variables.
 */
template <class T, uint16_t amountOfVariables>
void EEPROMwl<T, amountOfVariables>::distributeUniformly(eesize_t beginAddress, eesize_t totalSpaceToAllocate)
{
    //Whole eeprom memory assigned for us should be divided into amountOfVariables blocks, each divided into 2 partitions - status and data partition
    eesize_t onePartitionSize = totalSpaceToAllocate / amountOfVariables; //Total size of one block equals total space assigned divided by amount of variables // 256
    eesize_t statusBufferSize = onePartitionSize / (sizeof(T) + 1);       //Size of one block is divided by (size of variable stored + 1). This is length of status partition// 51,2
    eesize_t dataBufferSize = statusBufferSize * sizeof(T);               //204,8
    //Generally, we divide one block of data (assigned to one variable) in proportion 1:n, where n is sizeof(variable)
    //This leaves us with an example for eesize_t as variable:
    //Status length = (block length) / (sizeof(eesize_t) +1 ) = (block length) / 5
    //Data length = (status length) * sizeof(eesize_t) = (status_length) * 4

    for (uint16_t i = 0; i < amountOfVariables; i++) //Let's calculate border values for partitions in each block
    //Comments below contain example values of first block of data when 1024B eeprom is being assigned for 4 x 4 bytes values.
    {
        eesize_t dataBegin = beginAddress + i * onePartitionSize;      // dataBegin address     0
        eesize_t dataEnd = dataBegin + dataBufferSize - 1;            // dataEnd address       203
        eesize_t statusBegin = dataEnd + 1;                           // statusBegin address;  204
        eesize_t statusEnd = statusBegin + statusBufferSize - 1;      // statusEnd address;    254
        data[i].begin(dataBegin, dataEnd, statusBegin, statusEnd);  //Initialize internal _EEPROM_block structure that keeps the data
    }
}

/**
 * @brief Construct a new EEPROMwl<T, amountOfVariables>::EEPROMwl object
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @tparam amountOfVariables Amount of variables of type `T` that we will store inside EEPROM.
 * @param[in] beginAddress Address of where first byte of object should be in eeprom memory.
 * @param[in] totalSpaceToAllocate Total amount of bytes available for all variables.
 */
template <class T, uint16_t amountOfVariables>
EEPROMwl<T, amountOfVariables>::EEPROMwl(eesize_t beginAddress, eesize_t totalSpaceToAllocate)
{
    distributeUniformly(beginAddress, totalSpaceToAllocate);
}

/**
 * @brief Retrieve data from EEPROM memory.
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @tparam amountOfVariables Amount of variables of type `T` that we will store inside EEPROM.
 * @param[in] idx Index of underlying data block. This should be value between `0` and `amountOfVariables`.
 * @param[out] _data Retrieved data will be stored inside this variable.
 */
template <class T, uint16_t amountOfVariables>
void EEPROMwl<T, amountOfVariables>::get(uint16_t idx, T &_data)
{
    data[idx].get(_data);
}

/**
 * @brief Save data to EEPROM memory.
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @tparam amountOfVariables Amount of variables of type `T` that we will store inside EEPROM.
 * @param[in] idx Index of underlying data block. This should be value between `0` and `amountOfVariables`.
 * @param[in] _data Variable that will be saved in EEPROM.
 */
template <class T, uint16_t amountOfVariables>
void EEPROMwl<T, amountOfVariables>::put(uint16_t idx, T _data)
{
    data[idx].put(_data);
}

/**
 * @brief 
 * 
 * @tparam T Type of variable to store in EEPROM memory.
 * @tparam amountOfVariables Amount of variables of type `T` that we will store inside EEPROM.
 * @param[in] idx Index of underlying data block. This should be value between `0` and `amountOfVariables`.
 * @param[out] info Structure that will be filled with block info.
 */
template <class T, uint16_t amountOfVariables>
void EEPROMwl<T, amountOfVariables>::getBlockInfo(uint16_t idx, Block_data &info)
{
    data[idx].getBlockInfo(info);
}

//-----------------------------------------------
/**
 * @brief Converts retrieved debug data to String.
 * 
 * @return String containing all debug data of underlying memory block.
 */
inline String Block_data::getDebugData(){
    String result = "Data begin:" + (String)dataBegin +'\n' + \
                    "Data end:" + (String)dataEnd + '\n' + \
                    "Status begin:" + (String)statusBegin + '\n' +\
                    "Status end:" + (String)statusEnd + '\n' +\
                    "Next Write in status partition:" + (String)nextWrite + '\n' +\
                    "Next Read in data partition:"+ (String)nextRead;
    return result;
}