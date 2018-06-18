# [WIP] Timing Simulator

## Core structure

Caches have been implemented as described in [Lab 6][labs_link] of the course. Each cache has 32B blocks and use LRU replacement policies. A cache miss, whether load or store, requires 50 cycles to service. Specific details are:

1. **L1I Cache:** This is a four-way set associative cache that is 8KB in size (64 sets).
1. **L1D Cache:** This is an eight-way set associative cache that is 64KB in size (256 sets). Note that even though accesses to main memory require 50 cycles, dirty evictions are handled _instantaneously_.

## Running the simulator

The simulator requires the MIPS assembly code to be converted into a binary file. This can be done with SPIM. Sample input files from the course file have been provided in the directory _447inputs_.

Usage: sim \<input file\>

For a list of commands, type '?' without the quotes into the prompt.

[labs_link]: https://www.ece.cmu.edu/~ece447/s15/doku.php?id=labs
