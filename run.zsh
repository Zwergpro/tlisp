#!/bin/zsh

cc -std=c11 -Wall main.c mpc.c -ledit -lm -o main;
./main
