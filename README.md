# EEPROM wear leveling library
## Introduction
This is a simple library that can be used for leveling EEPROM wear on Arduino devices (and basically any other device if a custom implementation of `EEPROM.get()`, `EEPROM.put()` and `EEPROM.write()` is provided).

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
EEPROMwl<Object,3> eeprom_custom(0,1000); 

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

Imagine you're trying to save a variable with a length of one byte, and the EEPROM memory you have available has a size of 10 bytes.

The library divides available 10 bytes into two partitions - `status` partition containing information about how many writes were performed and where and `data` containing the actual variable.

This is a representation of an EEPROM memory before any operations have been done ( `|` sign is a separator between data (on the left) and status (on the right)):

```
00 00 00 00 00 | 00 00 00 00 00
```

Let's assume you now use the library to write a byte with a value of `CA`

```
CA 00 00 00 00 | 01 00 00 00 00
```

Now you write a value of `FE`:

```
CA FE 00 00 00 | 01 02 00 00 00
```

And so on.

Here's an example on a 3 byte value. Let's assume we have 20 bytes of memory available.

The initial division will look like this:

```
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 | 00 00 00 00 00
```

You write a value of `BA DB EE`

```
BA DB EE 00 00 00 00 00 00 00 00 00 00 00 00 | 01 00 00 00 00
```

And then another write of `FA DE DD` commences:

```
BA DB EE FA DE DD 00 00 00 00 00 00 00 00 00 | 01 02 00 00 00
```


