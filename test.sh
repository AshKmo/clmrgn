#!/bin/bash
make clean
make && valgrind --leak-check=full --show-leak-kinds=all ./bin/clmrgn examples/pi.clmrgn
