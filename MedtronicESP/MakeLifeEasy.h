// A collection of handy functions and code snippets
// Might make your life easier, I donno ¯\_(ツ)_/¯

// ensure this library description is only included once
#ifndef MakeLifeEasy_h
#define MakeLifeEasy_h

#include <Arduino.h>

// Isolate value from string, as int (32bit)
int32_t getIntfromStr(String inputString, String leadingString, 
                        int sizeOfVariable);
// Isolate value from string, as float
float getFloatfromStr(String inputString, String leadingString, 
                        int sizeOfVariable);
// Convert int to ASCII encoded char
char ASCIIintToChar(uint8_t input);
#endif