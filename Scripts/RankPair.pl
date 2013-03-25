#!/usr/bin/perl

use warnings;
use strict;
no warnings 'portable';

my %uniquifier; #used to emit a uniq set of things
my $seq = 0;

#my $failingPairsFile = shift @ARGV; 
my $correctPairsFile = shift @ARGV; die if !defined $correctPairsFile;
my $thresh =  10;
my %names;


#expects a list of formatted BT pairs with $thresh length distance vectors
my $num = 0;
my %failingFrequencies;
while(<>){

  chomp;
  my ($name,$bt1,$bt2) = split /\s+/;   

  if( !exists $failingFrequencies{$bt1} ){
    $failingFrequencies{$bt1} = ();
    $names{$bt1} = ();
  }

  if( !exists $failingFrequencies{$bt1}->{$bt2} ){
    $failingFrequencies{$bt1}->{$bt2} = 1; 
    $names{$bt1}->{$bt2} = "pair$num";
    $num++;
  }

}

my $totalEvents = 0;
my %correctFrequencies;
if( defined $correctPairsFile ){

  my @files = split /:/,$correctPairsFile;

  foreach(@files){

    my $CORR;
    open $CORR, "<$_" or next;

    while(<$CORR>){

      chomp;
  
      my ($e1,$e2,@freqs) = split /\s+/;
      $e1 = &formatBT($e1);
      $e2 = &formatBT($e2);
  
      if( !exists $correctFrequencies{$e1} ){

        $correctFrequencies{$e1} = ();

      }

      if( !exists $correctFrequencies{$e1}->{$e2} ){

        $correctFrequencies{$e1}->{$e2} = &sumOf(@freqs); 
        $totalEvents += &sumOf(@freqs);

      }else{

        $correctFrequencies{$e1}->{$e2} += &sumOf(@freqs); 
        $totalEvents += &sumOf(@freqs);

      }
  
    }
    close $CORR;
  }

}


#&printFailingDistanceVector();
#&computeFailingCorrectDistanceVectorSimilarity();
#&computeFailingUnitCorrectSimilarity();
#&printFailingAndCorrect();
#&computeFailingOtherPairRatio();

#TODO:Compute good rank function
&printPairs(\&OrderAndCoOccurrenceRanker);

sub OrderAndCoOccurrenceRanker(){
  my $e1 = shift;
  my $e2 = shift;
  my $OrderVariation = &OrderVariationRanker($e1,$e2);
  my $CoOccurrence = &CoOccurrenceRanker($e1,$e2);
  return $OrderVariation * $CoOccurrence;
}

sub OrderVariationRanker(){

  my $e1 = shift;
  my $e2 = shift;
  my @zero = (0) x $thresh;
    
  #Compute Order Features
  my $fwdFreq = 0;
  my $revFreq = 0;
  my $ratio = 0;

  if( exists $correctFrequencies{$e1} && exists $correctFrequencies{$e1}->{$e2} ){
    $fwdFreq = $correctFrequencies{$e1}->{$e2};
  }

  if( exists $correctFrequencies{$e2} && exists $correctFrequencies{$e2}->{$e1} ){
    $revFreq = $correctFrequencies{$e2}->{$e1};
  }

  #If this value is large, the pair is more interesting
  if( $fwdFreq + $revFreq > 0 ){
    #if the pair *ever* occurred in correct runs in either order
    return $revFreq / ($fwdFreq + $revFreq);
  }else{
    #Never saw these events in correct runs.  Let's rank it high
    return 0.5;
  }


}


sub CoOccurrenceRanker(){
 
  my $e1 = shift;
  my $e2 = shift;

  my $sum = 0;
  my @sums = ();

  my $other;
  foreach $other(keys %{$correctFrequencies{$e1}}){
    push @sums, $correctFrequencies{$e1}->{$other};
  }

  if( @sums > 0 ){
    $sum = &sumOf(@sums);
  }

  my $num = 0;
  if( defined $correctFrequencies{$e1}->{$e2} ){
    $num = $correctFrequencies{$e1}->{$e2};
  }

  if( $sum > 0 ){

    #e1 was followed by other stuff in correct runs
    #Note the sense is inverted in the last term -- n/s being lower means this is important
    #so (1-(n/s)) being *higher* means this is important
    #s/tE being *higher* means this is important
    #so (1-(n/s)) * s/tE being *higher* means this is important
    return ((1.0 - ($num/$sum)) * ($sum / $totalEvents))

  }else{

    if( $num == 0 ){
      return 0.5;
    }else{
      return -1;
    }

  }

}

sub printPairs(){
 
  my $ranker = shift; 

  my $e1;
  my $e2;
  my $numPair = 0;
  my %rankHash;

  my @zero = (0) x $thresh;
  foreach $e1(keys %failingFrequencies){
  
    foreach $e2(keys %{$failingFrequencies{$e1}}){

      $rankHash{$e1." ".$e2} = $ranker->($e1,$e2);

    }
  
  }

  my $k; 
  for $k( sort { $rankHash{$b} <=> $rankHash{$a} } keys %rankHash ){

    my ($ev1,$ev2) = split / /, $k;
    print "".($names{$ev1}->{$ev2})." ".$k." ".$rankHash{$k}."\n";

  }


}

sub computeFailingOtherPairRatio(){

  my $e1;
  my $e2;
  my @zero = (0) x $thresh;
  foreach $e1(keys %failingFrequencies){
  
    foreach $e2(keys %{$failingFrequencies{$e1}}){

      my $sum = 0;

      my @sums = ();
      my $other;
      foreach $other(keys %{$correctFrequencies{$e1}}){
        push @sums, &sumOf(@{$correctFrequencies{$e1}->{$other}});
      }

      if( @sums > 0 ){
        $sum = &sumOf(@sums);
      }
      my $num = 0;
      if( defined $correctFrequencies{$e1}->{$e2} ){
        $num = &sumOf( @{$correctFrequencies{$e1}->{$e2}} );
      }
      if( $sum > 0 ){
        #e1 was followed by other stuff in correct runs
        #Note the sense is inverted in the last term -- n/s being lower means this is important
        #so (1-(n/s)) being *higher* means this is important
        #s/tE being *higher* means this is important
        #so (1-(n/s)) * s/tE being *higher* means this is important
        print "$e1 $e2 ".($num/$sum)." ($num/$sum) ".((1.0 - ($num/$sum)) * ($sum / $totalEvents))."\n";
      }else{
        #e1 was not followed by anything but e2 in correct runs
        #or e1 was not followed by anything in correct runs
        #print "$e1 $e2 999999999 ($num / $sum)\n";
      }

    }
  
  }

}


sub computeFailingUnitCorrectSimilarity(){

  my $e1;
  my $e2;
  my @zero = (0) x $thresh;
  foreach $e1(keys %failingFrequencies){
  
    foreach $e2(keys %{$failingFrequencies{$e1}}){

      for(0 .. $#{$failingFrequencies{$e1}->{$e2}}){

        my @correct;
        my @failing = (); 
  
        push @failing, @zero;
        push @failing, @zero;

        die "".$_." ".(join ',', @failing)."\n" if !defined $failing[$thresh + $_];
        if( $failingFrequencies{$e1}->{$e2}->[$_] > 0 ){
          $failing[$thresh + $_] = 1; 
        }
  
        if( defined $correctFrequencies{$e2}->{$e1} ){
          push @correct, reverse @{$correctFrequencies{$e2}->{$e1}};
        }else{
          push @correct, @zero;
        }
  
        if( defined $correctFrequencies{$e1}->{$e2} ){
          push @correct, @{$correctFrequencies{$e1}->{$e2}};
        }else{
          push @correct, @zero;
        }
    
        die "".$#correct." ".$#failing."\n" if (@failing != 2 * $thresh );
        print "$e1 $e2 $_ ".(&cosineSimilarity(\@failing,\@correct))."\n";

      }
  
    }
  
  }

}

sub computeFailingCorrectDistanceVectorSimilarity(){

  my $e1;
  my $e2;
  my @zero = (0) x $thresh;
  foreach $e1(keys %failingFrequencies){
  
    foreach $e2(keys %{$failingFrequencies{$e1}}){

      my @correct;
      my @failing; 

      if( defined $failingFrequencies{$e2}->{$e1} ){
        push @failing, reverse @{$failingFrequencies{$e2}->{$e1}};
      }else{
        push @failing, @zero;
      }

      if( defined $correctFrequencies{$e2}->{$e1} ){
        push @correct, reverse @{$correctFrequencies{$e2}->{$e1}};
      }else{
        push @correct, @zero;
      }

      if( defined $correctFrequencies{$e1}->{$e2} ){
        push @correct, @{$correctFrequencies{$e1}->{$e2}};
      }else{
        push @correct, @zero;
      }
      
      push @failing, @{$failingFrequencies{$e1}->{$e2}};
      
      print "$e1 $e2 ".(&cosineSimilarity(\@failing,\@correct))."\n";
  
    }
  
  }

}

sub cosineSimilarity(){

  #Assumes $a1 and $a2 are the same length
  my $a1 = shift;
  my $a2 = shift;

  my $dot = 0;
  for(0 .. $#{$a1}){
    $dot += ($a1->[$_] * $a2->[$_]);
  } 
  
  my $m1 = &vecMag($a1);
  my $m2 = &vecMag($a2);
  if( $m1 != 0 && $m2 != 0){
    return ($dot / ($m1*$m2));
  }else{
    return -1;
  }

}

sub vecMag(){

  my $a1 = shift;
  my $sum = 0;
  foreach( @{$a1} ){
    $sum += ($_*$_);
  }
  return sqrt($sum);
}

sub printFailingAndCorrect(){

  my $e1;
  my $e2;
  my @zero = (0) x $thresh;
  foreach $e1(keys %failingFrequencies){
  
    foreach $e2(keys %{$failingFrequencies{$e1}}){
  
      print "$e1 $e2\n";
      print "\t".(join ' ',(@{$failingFrequencies{$e1}->{$e2}}))."\n";
      if(defined $correctFrequencies{$e1}->{$e2}){
        print "\t".(join ' ',(@{$correctFrequencies{$e1}->{$e2}}))."\n";
      }else{
        print "\t".(join ' ',@zero)."\n";

      }
  
    }
  
  }

}


sub printFailingDistanceVector(){

  my $e1;
  my $e2;
  my @zero = (0) x $thresh;
  foreach $e1(keys %failingFrequencies){
  
    foreach $e2(keys %{$failingFrequencies{$e1}}){
  
      print "$e1 $e2 ";

      if(defined $failingFrequencies{$e2}->{$e1}){

        print "".(join ' ',(reverse @{$failingFrequencies{$e2}->{$e1}}))." ";

      }else{

        print "".(join ' ',@zero)." ";

      }

      print "".(join ' ',(@{$failingFrequencies{$e1}->{$e2}}))." ";

      print "\n";
  
    }
  
  }

}

#&GenerateThreadConsecutivePairs( @bts );

sub GenerateThreadConsecutivePairs(@){

  my @bts = @_;

  my $t1;
  for($t1 = 0; $t1 <= ($#bts - 1); $t1++){

    my %seenOthers;
    $seenOthers{ $bts[$t1]->{'tid'} } = undef;

    my $t2;
    for($t2 = $t1 + 1; $t2 <= $#bts && $t2 - $t1 < $thresh; $t2++){
    
      my $distance = $t2 - $t1;
      if(  !exists $seenOthers{$bts[$t2]->{'tid'}}  ){

        #&emitTCP($bts[$t1]->{'bt'},
        #         $bts[$t2]->{'bt'},
        #         $distance);

        &printFeatures($bts[$t1]->{'bt'},
                       $bts[$t2]->{'bt'},
                       $distance);


      }

    }

  }
  
}


sub printFeatures($$$){

  my ($t1,$t2,$d) = @_;
  if(  !exists $uniquifier{&formatBT($t1)." ".&formatBT($t2)}  ){

    $uniquifier{&formatBT($t1)." ".&formatBT($t2)} = undef;

    #Compute Order Features
    my $fwdFreq = 0;
    my $revFreq = 0;
    my $ratio = 0;
    if( exists $correctFrequencies{&formatBT($t1)} && exists $correctFrequencies{&formatBT($t1)}->{&formatBT($t2)} ){
      $fwdFreq = &sumOf(@{$correctFrequencies{&formatBT($t1)}->{&formatBT($t2)}});
    }
    if( exists $correctFrequencies{&formatBT($t2)} && exists $correctFrequencies{&formatBT($t2)}->{&formatBT($t1)} ){
      $revFreq = &sumOf(@{$correctFrequencies{&formatBT($t2)}->{&formatBT($t1)}});
    }
    if( $fwdFreq != 0 ){
      $ratio = $revFreq / $fwdFreq;
    }else{

      if( $revFreq > 0 ){
        $ratio = 999999;
      }

    }

    #Compute Distance Features
    my $distanceImpact = 0;
    my $corrFreqSum = &sumOf(@{$correctFrequencies{&formatBT($t1)}->{&formatBT($t2)}});

    if( $corrFreqSum > 0 ){

      if(defined $correctFrequencies{&formatBT($t1)}->{&formatBT($t2)}->[$d]){

        $distanceImpact = ( $corrFreqSum - $correctFrequencies{&formatBT($t1)}->{&formatBT($t2)}->[$d]) / $corrFreqSum;

      }else{

        $distanceImpact = 1.0;

      }

    }else{

      $distanceImpact = 0.000000888888888;

    }

    #Note that T2 and T1 are swapped here.
    #To avoid bugs, we see T2 to start the FSM 
    #and then subsequently delay T1. 
    print "pair$seq ".&formatBT($t2)." ".&formatBT($t1)." $fwdFreq $revFreq $ratio $distanceImpact\n";
    $seq++;
  }


}

sub emitTCP($$$){

  my ($t1,$t2,$d) = @_;
  if(  !exists $uniquifier{&formatBT($t1)." ".&formatBT($t2)}  ){

    $uniquifier{&formatBT($t1)." ".&formatBT($t2)} = undef;

    #Compute Order Features
    my $fwdFreq = 0;
    my $revFreq = 0;
    my $ratio = 0;
    if( exists $correctFrequencies{&formatBT($t1)} && exists $correctFrequencies{&formatBT($t1)}->{&formatBT($t2)} ){
      $fwdFreq = &sumOf(@{$correctFrequencies{&formatBT($t1)}->{&formatBT($t2)}});
    }
    if( exists $correctFrequencies{&formatBT($t2)} && exists $correctFrequencies{&formatBT($t2)}->{&formatBT($t1)} ){
      $revFreq = &sumOf(@{$correctFrequencies{&formatBT($t2)}->{&formatBT($t1)}});
    }
    if( $fwdFreq != 0 ){
      $ratio = $revFreq / $fwdFreq;
    }else{

      if( $revFreq > 0 ){
        $ratio = 999999;
      }

    }

    #Compute Distance Features
    my $distanceImpact = 0;
    my $corrFreqSum = &sumOf(@{$correctFrequencies{&formatBT($t1)}->{&formatBT($t2)}});

    if( $corrFreqSum > 0 ){

      if(defined $correctFrequencies{&formatBT($t1)}->{&formatBT($t2)}->[$d]){

        $distanceImpact = ( $corrFreqSum - $correctFrequencies{&formatBT($t1)}->{&formatBT($t2)}->[$d]) / $corrFreqSum;

      }else{

        $distanceImpact = 1.0;

      }

    }else{

      $distanceImpact = 0.000000888888888;

    }

    #Note that T2 and T1 are swapped here.
    #To avoid bugs, we see T2 to start the FSM 
    #and then subsequently delay T1. 
    print "pair$seq ".&formatBT($t2)." ".&formatBT($t1)." $fwdFreq $revFreq $ratio $distanceImpact\n";
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

sub sumOf($){
  my @ar = @_;
  my $sum = 0;
  $sum += $_ for @ar;
  return $sum;
}
