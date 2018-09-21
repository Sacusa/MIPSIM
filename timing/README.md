# [WIP] Timing Simulator

## Core structure

The core has been structured exactly as described in the course [labs][labs_link].

### 1. L1 Caches

L1 caches have been implemented as described in [Lab 6]. Each cache has 32B blocks and use LRU replacement policies. A cache miss, whether load or store, requires 50 cycles to service. Specific details are:

1. **L1I Cache:** This is a four-way set associative cache that is 8KB in size (64 sets).
1. **L1D Cache:** This is an eight-way set associative cache that is 64KB in size (256 sets). Note that even though accesses to main memory require 50 cycles, dirty evictions are handled _instantaneously_.

### 2. Branch Predictor

The branch predictor consists of a gshare predictor and a branch target predictor (BTB).

1. **Gshare Predictor:** The predictor consists of an 8-bit global history register (GHR) and a 256-entry pattern history table (PHT). Bits [9:2] XORed with the GHR are used to index into PHT, each entry of which is a 2 bit saturating counter.
1. **Branch Target Buffer:** The branch target buffer (BTB) contains 1024 entries indexed by bits [11:2] of the PC. Each entry of the BTB contains (i) an address tag, indicating the full PC; (ii) a valid bit; (iii) a bit indicating whether this branch is unconditional; and (iv) the target of the branch.

Every cycle, the PC is used to index into BTB and PHT. If there is a BTB hit, the branch target indicated by the BTB is followed if (i) unconditional bit in BTB entry is set; (ii) PHT value is >1. Otherwise, the next PC is predicted to be PC+4. The GHR, PHT and BTB are updated in the execute stage.

## Running the simulator

The simulator requires the MIPS assembly code to be converted into a binary file. This can be done with SPIM. Sample input files from the course file have been provided in the directory _447inputs_.

Usage: sim \<input file\>

For a list of commands, type '?' without the quotes into the prompt.

[labs_link]: http://www.archive.ece.cmu.edu/~ece447/s15/doku.php?id=labs
