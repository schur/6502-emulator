# 6502-emulator
A 6502 emulator (based on OneLoneCoder's NES Emulator)
## Description
This is a 6502 emulator written in C++. It is based on the 6502 emulator contained in [OneLoneCoder's NES emulator](https://github.com/OneLoneCoder), but can be used as a standalone 6502 emulator. I like this emulator, so decided to fork it and develop it further. 
The emulator comes with an emulated Bus and RAM.
## Build
To build, simply clone this repository, enter the directory and type
`make`
This will build the executable
## Usage
`./6502_demo <binary file>`

If no argument is given, a simple demo 6502 assembly program (addition loop) will automatically be loaded.
## Example
To run the example (Ben Eater's convert to decimal):

`./6502_demo examples/beneater_div.bin`
