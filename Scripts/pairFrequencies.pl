#!/usr/bin/perl
use warnings;
use strict;
no warnings 'portable';

my $lastLine = "";
my %pairFrequency;

my $maxlen = 9;

#history[0] is the most recent. history[maxlen] is the oldest.
my @history; 

#histogram{event 1}->{event 2}->[d] = f 
#means event 1 and event 2 occurred consecutively f times
my %histogram;

#expects an RPB with timestamps stripped off
while(<>){

  chomp;
  

  my $d;
  for $d(0 .. $maxlen){

    if(!defined $history[$d]){

      unshift @history, $_;
      last;

    }

    my $lastLine = $history[$d];
    my ($tid1,$bt1) = split /:/, $lastLine;
    my ($tid2,$bt2) = split /:/, $_;

    $bt1 = &formatBT($bt1);
    $bt2 = &formatBT($bt2);

    if ($tid1 != $tid2){
      
      if( !exists $histogram{$bt1} ){
        $histogram{$bt1} = ();
      }

      if( !exists $histogram{$bt1}->{$bt2} ){
        $histogram{$bt1}->{$bt2}->[$d] = 0;
      }

      $histogram{$bt1}->{$bt2}->[$d]++; 

    }

  }

  if( $#history == $maxlen - 1 ){
    pop @history;
  }
  unshift @history, $_;

}


my $k;
foreach $k(keys %histogram){
    
  my $k2;
  foreach $k2(keys %{$histogram{$k}}){

    print "$k $k2 ";
    my $d;
    foreach $d(0 .. $maxlen){

      if(defined $histogram{$k}->{$k2}->[$d]){
        print " ".$histogram{$k}->{$k2}->[$d] ;
      }else{
        print " 0";
      }

    }
    print "\n";
  
  }

}

sub formatBT($){

  my $btString = shift;
  $btString =~ s/->/:/g;
  $btString =~ s/\(nil\)/0x0/g;

  my @addrs = split /:/, $btString;
  foreach( @addrs ){

    if( hex($_) > 0x700000000000 ){
      $_ = '0xffffffffffffffff';
    }

  }
  return join ':',@addrs;

}
