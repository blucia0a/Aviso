#!/usr/bin/perl

my $app = shift @ARGV;
my %thdTabs;

while(<>){ 

  chomp; 
  my ($thd,$bt) = split /:/; 
  my @outp=`addr2line -e $app $bt`; 
  $_=`basename $_` for @outp; 
  chomp @outp; 

  my $num = -1;
  if(exists $thdTabs{$thd}){

    $num = $thdTabs{$thd};

  }else{

    $thdTabs{$thd} = $maxTabs;
    $num = $maxTabs;
    $maxTabs++;

  }

  my $tabs = "\t" x $num;
  print "$tabs $thd\n";
  print "$tabs--------------\n";
  print "$tabs $_\n" for @outp;
  print "$tabs--------------\n";

}

