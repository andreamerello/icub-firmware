#!/bin/bash
dfu-util -a 0 -d 0483:df11 -s 0x08000000:leave  -D build/PMC_00.elf
