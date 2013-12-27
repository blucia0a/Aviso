#!/bin/bash

AVISOHOME=/home/vagrant/cvsandbox/Aviso
AVISOSCRIPTS=$AVISOHOME/Scripts

TRACE=$1
CORRECTHISTO=$2

x=$RANDOM;
head -n10000 $TRACE > /tmp/rpb${x}; 
mv /tmp/rpb${x} $TRACE;

#TODO: Move these into AvisoServer
$AVISOSCRIPTS/computePairHistogram .fail.histo $TRACE > .fail.histo;
$AVISOSCRIPTS/GeneratePairFSMs.pl .fail.histo $CORRECTHISTO 10;
rm .fail.histo

exit 0
