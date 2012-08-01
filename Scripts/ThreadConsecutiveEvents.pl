#!/usr/bin/perl

use warnings;
use strict;

my %uniquifier; #used to emit a uniq set of things
my $seq = 0;
my $thresh = shift @ARGV | 10;

my @bts;
while(<>){

  chomp;
  my ($tid,$bt) = split /:/;   

  my $ent;
  $ent->{'tid'} = $tid;
  $ent->{'bt' } = $bt;

  push @bts, $ent;

}

&GenerateThreadConsecutivePairs( @bts );

sub GenerateThreadConsecutivePairs(@){

  my @bts = @_;

  my $t1;
  for($t1 = 0; $t1 <= ($#bts - 1); $t1++){

    my %seenOthers;
    $seenOthers{ $bts[$t1]->{'tid'} } = undef;

    my $t2;
    for($t2 = $t1 + 1; $t2 <= $#bts && $t2 - $t1 < $thresh; $t2++){
     
      if(  !exists $seenOthers{$bts[$t2]->{'tid'}}  ){
 
        &emitTCP($bts[$t1]->{'bt'},
                 $bts[$t2]->{'bt'});

      }

    }

  }
  
}
sub emitTCP($$$){

  my ($t1,$t2) = @_;
  if(  !exists $uniquifier{&formatBT($t1)." ".&formatBT($t2)}  ){
    $uniquifier{&formatBT($t1)." ".&formatBT($t2)} = undef;
    print "pair$seq ".&formatBT($t1)." ".&formatBT($t2)."\n";
    $seq++;
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
