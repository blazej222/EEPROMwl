# EEPROM wear leveling library
## Introduction
Each EEPROM cell has a limited amount of writes that can be performed until it becomes unrealiable.

This library allows to store multiple variables of any type (builtin or custom classes) in EEPROM memory while avoiding EEPROM wear.

It does so by distributing space each variable across a certain number of cells, instead of writing in the same cell/cells all the time.

## Usage
Make sure that the EEPROM area you want to allocate for the library is empty before usage.

First we need to define a structure we will be saving to memory and create an `EEPROMwl` object:
```cpp
#include <Arduino.h>
#include "EEPROMwl.h"

//Example object we will use for demonstration.
struct Object{
    uint8_t foo = 0;
    uint16_t bar = 0;
}
/*
The library will divide 1000 bytes (from address 0 to 999) 
across 3 instances of variable (of type Object).
*/
EEPROMwl<Object,3> eeprom_custom(0,1000,false); 

Object a,b,c,d;
a.foo = 1;
b.bar = 2;
```
We've created three instances of object with type `Object`.

Next we save those instances to EEPROM memory:

```cpp
eeprom_custom.put(0,a);     
eeprom_custom.put(1,b);
eeprom_custom.put(2,c);
```

And we can retrieve the data using:
```cpp
eeprom_custom.get(0,c);
eeprom_custom.get(1,d);

Serial.println(c.foo); //Result is 1.
Serial.println(d.bar); //Result is 2.
```

In addition, advanced debug info can be retrieved about underlying memory structures:
```cpp
//Declare additional helper structure (definied within the library).
Block_data info;
//Retrieve data
eeprom_custom.getBlockInfo(0,info);
Serial.println(info.getDebugData()); 
/*
Result is information about underlying data structure:
Data begin:xxx
Data end:xxx
Status begin:xxx
Status end:xxx
Next Write in status partition:xxx
Next Read in data partition:xxx
*/
```

## Principle of operation

#TODO: Write this

### PLACEHOLDER
