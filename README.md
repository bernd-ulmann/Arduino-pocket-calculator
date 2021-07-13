# Arduino-pocket-calculator
This directory contains the source code for a simple stack oriented pocket 
calculator based on an Arduino Mega 2650 with a 3.5 inch touch TFT display.

![OverallImg](side_view.jpg)

This is a very basic pocket calculator which uses single precision floating
point numbers to do all arithmetic. Its main purpose is to show how to 
implement simple touch screen interfaces. Neither can it compete with a real
pocket calculator nor is it intended to do so. :-) 

## Operation
Pocket calculators are a long term obsession of mine (dating back to my much
younger self, being about 7 years old, when my parents bought a pocket
calculator which deeply influenced me :-) ). This little device does not even
try to replicate any commercially available calculator (at least none I know
of).

First of all, it is a stack based calculator (RPN). To enter a number, just 
touch the digits, the sign of a number entered may be changed during number
entry by the +- key in the bottom row. In contrast to other RPN calculators
a number entered must be PUSHed explicitly onto the stack by tipping the 
PUSH "button".

Most keys have two functions with the INV key controlling which function is 
to be executed. If INV is pressed, a tiny "I" is displayed in the status 
line on top of the display. Any following key press will result in the inverse
function of that key to be executed. In the following table, TOS denotes the
value on the "top of stack" while TOS-1 represents the value in the second
element from the top of the stack:

|Key |Normal mode|INV mode|
|----|-----------|--------|
|RAD |Switch to radians mode|Switch to degrees|
|FIX |Set number of decimal places to TOS||
|CLR |Resets the calculator||
|INV |Switch to INV mode|Switch to normal mode|
|PI/E|Push pi to the stack|Push e to the stack|
|I/FP|Return the integer part of the TOS|Return the fractional part of TOS|
|SQRT|Compute the square root|Compute the square|
|DEL |Delete a number input||
|SIN |sin(TOS)|asin(TOS)|
|COS |cos(TOS)|acos(TOS)|
|TAN |tan(TOS)|atan(TOS)|
|P/R |polar -> rectangular|rectangular -> polar|
|STO |Stare TOS-1 into store location TOS|Recall from storage location TOS|
|X/Y |Swap TOS and TOS-1||
|LOG |log(TOS)|10 ** TOS|
|LN  |ln(TOS)|exp(TOS)|
|1/X |1/TOS||
|* / |TOS-1 * TOS|TOS-1 / TOS|
|+ - |TOS-1 + TOS|TOS-1 - TOS|
|PUSH|Push a number to the stack or duplicate TOS|Delete TOS|
|0..9|Enter digit||
|.   |Decimal point||
|+-  |Change sign of number being entered||
