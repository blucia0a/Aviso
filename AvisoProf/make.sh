#!/bin/bash

PIN_ROOT="$HOME/cvsandbox/pin-2.13-61206-gcc.4.4.7-linux"
PATH="$PIN_ROOT:$PATH"

make -f Makefile.avisoprof PIN_ROOT="$PIN_ROOT" $@
