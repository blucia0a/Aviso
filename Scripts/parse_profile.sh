#!/bin/bash
cat $@ | xargs addr2line -e ../Tests/Crasher/Crasher | grep ^[^?][^?] | perl -e 'while(<>){ chomp; @parts = split /:/;  $bbn = `basename $parts[0]`; chomp $bbn; print $bbn; print " "; print $parts[1]; print "\n";}'
