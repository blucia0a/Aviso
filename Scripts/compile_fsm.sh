#!/bin/bash
AVISOHOME=/sampa/home/blucia/cvsandbox/Aviso
AVISOSCRIPTS=$AVISOHOME/Scripts
SMC=$AVISOSCRIPTS/compileStateMachine.pl

CONF=$1

$SMC < $CONF > $CONF.cpp
g++ -I$AVISOHOME/Runtime/StateMachines -g -fPIC -c -o $CONF.o $CONF.cpp
g++ -I$AVISOHOME/Runtime/StateMachines -g -shared -fPIC -o $CONF.so $CONF.o
rm $CONF.o $CONF.cpp
