#!/usr/bin/perl

use warnings;
use strict;

#[0xe000000000403bd1->0x404c0d->0x7f303c9731c4->0x4016e9->(nil)]  [0xe000000000403bf2->0x404c0d->0x7f303c9731c4->0x4016e9->(nil)]  [0xe000000000403e7a->0x404c0d->0x7f303c9731c4->0x4016e9->(nil)]  [0xe000000000403e9d->0x404c0d->0x7f303c9731c4->0x4016e9->(nil)]  [0xc00000000402e90->0x7f303d87f3f7->0x7f303ca2cbbd->(nil)->(nil)]  [0xc00000000402ebe->0x7f303d87f3f7->0x7f303ca2cbbd->(nil)->(nil)]  [0xc00000000402ec3->0x7f303d87f3f7->0x7f303ca2cbbd->(nil)->(nil)]  [0x180000000402e90->0x7f303d87f3f7->0x7f303ca2cbbd->(nil)->(nil)]  [0x180000000402ebe->0x7f303d87f3f7->0x7f303ca2cbbd->(nil)->(nil)]  [0x180000000402ec3->0x7f303d87f3f7->0x7f303ca2cbbd->(nil)->(nil)]

my $windowLength = shift @ARGV;
my $prefix = shift @ARGV || "AutoGen";
my $seqnum = 0; #static counter of emitted things
my %uniquifier; #used to emit a uniq set of things

my @bts;
while(<>){

  chomp;
  my ($tid,$bt) = split /:/;   

  my $ent;
  $ent->{'tid'} = $tid;
  $ent->{'bt' } = $bt;
  if($#bts > $windowLength - 1){
    shift @bts;
  }

  push @bts, $ent;

  &GenerateTriples( ($windowLength, @bts) );

}

sub GenerateTriples(@){

  my $wLen = shift;
  my @bts = @_;

  my $t1e1;
  for($t1e1 = 0; $t1e1 <= ($#bts - 2); $t1e1++){

    my $t2;
    for($t2 = $t1e1 + 1; $t2 <= ($#bts - 1); $t2++){
      
      my $t1e2;
      for($t1e2 = $t2 + 1; $t1e2 <= $#bts; $t1e2++){
      
          
        if($bts[$t1e1]->{'tid'} != $bts[$t2]->{'tid'} && #1 and 2 are diff
           $bts[$t1e1]->{'tid'} == $bts[$t1e2]->{'tid'}){#1 and 3 are same
        #if($bts[$t1e1]->{'tid'} != $bts[$t2]->{'tid'} || #1 and 2 are diff
        #   $bts[$t1e2]->{'tid'} != $bts[$t2]->{'tid'} || #3 and 2 are diff
        #   $bts[$t1e1]->{'tid'} != $bts[$t1e2]->{'tid'}){#1 and 3 are diff
  
          &emitTriple($bts[$t1e1]->{'bt'},
                      $bts[$t2]->{'bt'}, 
                      $bts[$t1e2]->{'bt'});

        }

      }

    }

  }
  
}

sub emitTriple($$$){

  my ($t1e1,$t2,$t1e2) = @_;
  if(!exists $uniquifier{&formatBT($t1e1)." ".
                         &formatBT($t1e2)." ".
                         &formatBT($t2)}){
    $uniquifier{&formatBT($t1e1)." ".
                &formatBT($t1e2)." ".
                &formatBT($t2)} = undef;
    print "$prefix$seqnum ".&formatBT($t1e1).
          " ".&formatBT($t1e2).
          " ".&formatBT($t2)."\n";
    $seqnum++;
  }


}

sub formatBT($){

  my $btString = shift;
  $btString =~ s/->/:/g;
  $btString =~ s/\(nil\)/0x0/g;

  my @addrs = split /:/, $btString;
  foreach( @addrs ){

    if( hex($_) > 0x700000000000 && $_ ne '0xeeeeeeeeeeeeeeee' ){
      $_ = '0xffffffffffffffff';
    }

  }
  return join ':',@addrs;

}
