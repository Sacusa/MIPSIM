# MIPSIM

This is a cycle-accurate instruction level MIPS32 simulator. It implements a limited subset of the MIPS32 ISA.

It is derived from [CMU 18-447][course_link]'s first lab. The shell has been used as-is. The simulator core has been created from scratch.

## Core structure

The opcodes have been divided into three categories:

1. Instructions that can be interpreted directly from opcode.
2. "SPECIAL" instructions that have a SHAMT field.
3. "REGIMM" instructions that have a TARGET field.

Separate functions pertaining to each type have been defined.

## Running the simulator

The simulator requires the MIPS assembly code to be converted into a binary file. This can be done with SPIM.

Usage: sim \<input file\>

For a list of commands, type '?' without the quotes into the prompt.

[course_link]: https://www.ece.cmu.edu/~ece447/s15/doku.php?id=start