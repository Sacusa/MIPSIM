#!/usr/bin/python3

import sys

def print_usage():
    print('Usage: asm2hex <asm file>')

def main():
    if len(sys.argv) != 2:
        print_usage()
        exit(1)
    
    asm_file = open(sys.argv[1], 'r')
    hex_file = open(sys.argv[1][:-3] + 'x', 'w')

    # discard the first line
    line = asm_file.readline()

    # strip the rest of the lines
    for line in asm_file:
        hex_file.write(line.strip()[11:19] + '\n')
    
    hex_file.close()
    asm_file.close()

if __name__ == '__main__':
    main()
