# digitech-assembler

A simple "assembler" (actually outputs hex codes of the instructions in ASCII) for the minimal computer.

It supports R-, I- and J-Instructions.

## Installation

Compile the [`assembler.c`](./assembler.c)-File and run the executable.
For example in Linux:
```sh
gcc assembler.c -o assembler
```

This program was not tested on windows, it should theorically work though using Visual Studio (or MSVC).

## Usage

Run the compiled program with the path to the assembly file as it's first argument. You can optionally pass a filename
for an output file. If you don't pass an output file, the program just writes to the terminal's output.

## How it works

It reads a file specified as a command line argument line by line.
Then it tokenizes this line by splitting on spaces and commas.
Using a map it then looks for the type of the instruction (R, I and J) and
finally merges the arguments and the opcode into one instruction.

## Configuration

The assembler is written in a way that you can easily add more instructions of your own type.

To add a `bne` instruction you could first modify the
[`BITS_INSTRUCTION`](https://github.com/AntonPieper/digitech-assembler/blob/master/assembler.c#L13) and
[`BITS_OPCODE`](https://github.com/AntonPieper/digitech-assembler/blob/master/assembler.c#L14) to add room for the instruction.

Then you can modify the [`INSTRUCTIONS`](https://github.com/AntonPieper/digitech-assembler/blob/main/assembler.c#L37-L39)-Array and add the `bne`-instruction:

```c
const Instruction INSTRUCTIONS[] = {
    {"add", 'r', 0x0},
    {"sub", 'r', 0x1},
    {"mul", 'r', 0x2},
    {"and", 'r', 0x3},
    {"or", 'r', 0x4},
    {"xor", 'r', 0x5},
    {"ldi", 'i', 0x6},
    {"jmp", 'j', 0x7},
    {"bne", "I", 0x8}
};
```
