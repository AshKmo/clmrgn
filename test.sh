#!/bin/bash
gcc -g main.c && valgrind --leak-check=full --show-leak-kinds=all ./a.out script-test.clmrgn
