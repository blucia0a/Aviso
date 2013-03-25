#!/usr/bin/perl

use warnings;
use strict;

my %Opt;
$Opt{'PrintFeatures'} = 0;
my $Executable = shift;
my $FreqFile = shift;
my $maxFreq = 0;

warn "LOADING FREQUENCIES\n";
my %freqHash;
my $FREQ;
open $FREQ, "$FreqFile" or die "$!: $FreqFile couldn't be opened\n";
while(<$FREQ>){
  chomp;
  my ($ad, $fr) = split /\s+/;
  $ad =~ s/0x0*//;
  $freqHash{$ad} = $fr;
  if($fr > $maxFreq){
    $maxFreq = $fr;
  }
}
close($FREQ);
warn "DONE LOADING FREQUENCIES\n";

my %blacklist;
my %whitelist;
my %pairHash;
my %all;
while(0){
  chomp;
  my ($a1, $a2, $t) = split /\s+/;
  chomp $a1; chomp $a2;
  $a1 =~ s/0x0*//;
  $a2 =~ s/0x0*//;
  my ($a1func,$a1file,$a1line);
  if( !exists $whitelist{$a1} && !exists $blacklist{$a1} ){
  
    ($a1func,$a1file,$a1line) = &addr2lineCall($Executable,$a1);  
    if( ($a1file =~ /\/usr\/include/) || 
        ($a1file =~ /\?\?/)           ){

      $blacklist{$a1} = undef;

    }else{
   
      $whitelist{$a1} = undef;
  
    }

  }
  my ($a2func,$a2file,$a2line);
  if( !exists $whitelist{$a2} && !exists $blacklist{$a2} ){

    ($a2func,$a2file,$a2line) = &addr2lineCall($Executable,$a2);  
  
    if( ($a2file =~ /\/usr\/include/) || 
        ($a2file =~ /\?\?/)           ){
  
      $blacklist{$a2} = undef;
  
    }else{
  
      $whitelist{$a2} = undef;
    
    }

  }

  if( exists $whitelist{$a1} &&
      exists $whitelist{$a2} ){
    $pairHash{$a1}->{$a2} = $t; 
    $all{$a1} = undef;
    $all{$a2} = undef; 
  }
}



my %all= map { $_ => undef } keys %freqHash;

if( $Opt{'PrintFeatures'} == 1 ){
  print "Pair\tFollFreq\tPredFreq\tCoFreq\tIGain\tCoFollRatio\tCoPredRatio\n";
}

my %scores;
my %max;
my %loopRepl;
my %slcRepl;
#Loop over the code points twice.
#
#1st time: compute the scores 
my $foll;
foreach $foll(keys %freqHash){

  my $curMax = 0;
  my $cp;
  foreach $cp(keys %{$pairHash{$foll}}){
      #next if( $freqHash{$foll} < $freqHash{$cp} );
      #next if( abs($freqHash{$cp} - $pairHash{$foll}->{$cp}) > 0.1 );
    if( $Opt{'PrintFeatures'} == 1 ){
      print "$foll$cp\t".
                          $freqHash{$foll}."\t".
                          $freqHash{$cp}."\t".
                          $pairHash{$foll}->{$cp}."\t".
                          ($freqHash{$cp}/$freqHash{$foll})."\t".
                          ($pairHash{$foll}->{$cp}/$freqHash{$foll})."\t".
                          ($pairHash{$foll}->{$cp}/$freqHash{$cp})."\n";
  

    }
    
    my $IGain = 0;#$freqHash{$cp}/$freqHash{$foll};
    my $NCoFreq = 0;#$pairHash{$foll}->{$cp}/$freqHash{$foll};
    #$loopRepl{$foll}->{$cp} = undef;
  }
 
}
if( $Opt{'PrintFeatures'} == 1 ){
  exit(1);
}

my %out = %all;
warn "Total Instrumentation Points: ".(scalar keys %out)."\n";

foreach $foll(keys %all){
  if($freqHash{$foll} > ($maxFreq / 5.0)){
    #delete $out{$foll};
  }
}

warn "Total Instrumentation Points After Frequency Filtering: ".(scalar keys %out)."\n";

foreach $foll(keys %loopRepl){

  my $cp;
  foreach $cp(keys %{$loopRepl{$foll}}){
    if(exists $out{$foll}){# && exists $out{$cp}){
      #delete $out{$foll};
    }
  }

}

warn "Total Instrumentation Points After Loop Filtering: ".(scalar keys %out)."\n";

foreach $foll(keys %slcRepl){

  my $cp;
  foreach $cp(keys %{$slcRepl{$foll}}){
    if(exists $out{$foll}){
      #delete $out{$foll};
    }
  }

}

warn "Total Instrumentation Points After SLC Filtering: ".(scalar keys %out)."\n";

foreach(keys %all){
  my ($a2func,$a2file,$a2line) = &addr2lineCall($Executable,$_);  
  my $a2fileshort = `basename $a2file`;
  chomp $a2fileshort;
  next if $a2fileshort =~ /\?\?/;
  print "$a2fileshort $a2line\n";
}

exit(1);

sub addr2lineCall($$){
  my $exec = shift;
  my $pc = shift;
  my $ADDR2LINE;
  open($ADDR2LINE, '-|',"addr2line","-C","-f","-e",$exec,$pc); 
  my ($func,$codePoint) = <$ADDR2LINE>;
  close($ADDR2LINE);
  chomp $func;
  my ($codePointFile,$codePointLine) = split /:/,$codePoint;
  chomp $codePointFile;
  chomp $codePointLine;
  return ($func,$codePointFile,$codePointLine); 
}
