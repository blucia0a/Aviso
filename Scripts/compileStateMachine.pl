#!/usr/bin/perl
use warnings;
use strict;

my @args = @ARGV;
my $argstring = "";

while(<>){
  $argstring .= $_;
}    

chomp $argstring;
push @args, (split /\s+/,$argstring);

my $machineName = shift @args;

if( $#args == 2 ){
  &tripleGen(@args);
}elsif( $#args == 1 ){
  &pairGen(@args);
}


sub pairGen($$$){

  #$fst activates the FSM.
  #Once activated, $snd is delayed by the FSM
  my ($fst,$snd) = @_;
  my @first = split /[:,-\/\\>]/, $fst;
  my @second= split /[:,-\/\\>]/, $snd;


print "

\#include <pthread.h>
\#include <stdio.h>

\#include \"../Backtrace.h\"
\#include \"SMAction.h\"

";

print "
\#include \"StateMachine.h\"
\#include \"StateMachineFactory.h\"

class ".$machineName."StateMachine : public StateMachine{

  unsigned curState;
  int T1Binding;
  Backtrace *interleaver;

public:

  ".$machineName."StateMachine();
  virtual enum SMAction run(Backtrace *t, unsigned long tid);

};

class ".$machineName."StateMachineFactory : public StateMachineFactory{

public:

  virtual StateMachine *CreateMachine(){
    return new ".$machineName."StateMachine();
  }

  virtual bool isStartState(Backtrace *t){
    if( (t->bt[0] == (void*)".$first[0].") && ";

  for(1 .. ($#first - 1)){
    if($first[$_] ne "0xffffffffffffffff" &&
       $first[$_] ne "0xeeeeeeeeeeeeeeee"
      ){ 
      print "
       (t->bt[$_] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$_] == (void*)".$first[$_].") && ";
    }
  }
  if($first[$#first] ne "0xffffffffffffffff" &&
     $first[$#first] ne "0xeeeeeeeeeeeeeeee"
    ){ 
    print "
        (t->bt[$#first] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$#first] == (void*)".$first[$#first].")";
  }else{
    print " true ";
  }
print "
      ){
      return true;
    }
    return false;
  }

};


";

print "".
$machineName."StateMachine::".$machineName."StateMachine(){
  curState = 0;
  T1Binding = -1; 
  Backtrace *interleaver = new Backtrace();
  IDString = strdup(\"".(join ':', @first)." ".(join ':', @second)."\");";

print "
}

enum SMAction ".$machineName."StateMachine::run(Backtrace *t, unsigned long tid){

  if(curState == 0){/*Empty*/

    if( t->bt[0] == (void*)".$first[0]." && 
      (";
  for(1 .. ($#first - 1)){
    if($first[$_] ne "0xffffffffffffffff" &&
       $first[$_] ne "0xeeeeeeeeeeeeeeee"
      ){ 
      print "
       (t->bt[$_] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$_] == (void*)".$first[$_].") && ";
    }
  }
  if($first[$#first] ne "0xffffffffffffffff" && 
     $first[$#first] ne "0xeeeeeeeeeeeeeeee"
    ){ 
    print "
       (t->bt[$#first] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$#first] == (void*)".$first[$#first].")";
  }else{
    print " true ";
  }
print "
      )

    ){ 

      curState = 1;
      T1Binding = tid;
      return CONT;

    }else{

      return CONT;

    }

  }else if(curState == 1){/*Saw first event that shouldnt be intlv*/

    if(tid != T1Binding){
      if( t->bt[0] == (void*)".$second[0]." &&
        ";
  for(1 .. ($#second - 1)){
    if($second[$_] ne "0xffffffffffffffff" &&
       $second[$_] ne "0xeeeeeeeeeeeeeeee"
      ){ 
      print "
       (t->bt[$_] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$_] == (void*)".$second[$_].") && ";
    }
  }
  if($second[$#second] ne "0xffffffffffffffff" &&
     $second[$#second] ne "0xeeeeeeeeeeeeeeee"
    ){ 
    print "
        (t->bt[$#second] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$#second] == (void*)".$second[$#second].")";
  }else{
    print " true ";
  }
print " 
      ){

        /*Interleaver is running*/
        return WAIT;

      }else{

        return CONT;

      }

    }else{

        return CONT;

    }

  }else{

    fprintf(stderr,\"Invalid State %u\\n\", curState);
    return CONT;

  }

  return CONT;
}

extern \"C\"{
StateMachineFactory *getFactory(){
  return new ".$machineName."StateMachineFactory(); 
}
}
";

}



sub tripleGen($$$){

  my ($fst,$snd,$int) = @_;
  my @first = split /[:,-\/\\>]/, $fst;
  my @second= split /[:,-\/\\>]/, $snd;
  my @intlv = split /[:,-\/\\>]/, $int;


print "

\#include <pthread.h>
\#include <stdio.h>

\#include \"../Backtrace.h\"
\#include \"SMAction.h\"

";

print "
\#include \"StateMachine.h\"
\#include \"StateMachineFactory.h\"

class ".$machineName."StateMachine : public StateMachine{

  unsigned curState;
  int T1Binding;
  Backtrace *interleaver;

public:

  ".$machineName."StateMachine();
  virtual enum SMAction run(Backtrace *t, unsigned long tid);

};

class ".$machineName."StateMachineFactory : public StateMachineFactory{

public:

  virtual StateMachine *CreateMachine(){
    return new ".$machineName."StateMachine();
  }

  virtual bool isStartState(Backtrace *t){
    if( (t->bt[0] == (void*)".$first[0].") && ";

  for(1 .. ($#first - 1)){
    if($first[$_] ne "0xffffffffffffffff" &&
       $first[$_] ne "0xeeeeeeeeeeeeeeee"
      ){ 
      print "
       (t->bt[$_] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$_] == (void*)".$first[$_].") && ";
    }
  }
  if($first[$#first] ne "0xffffffffffffffff" &&
     $first[$#first] ne "0xeeeeeeeeeeeeeeee"
    ){ 
    print "
        (t->bt[$#first] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$#first] == (void*)".$first[$#first].")";
  }else{
    print " true ";
  }
print "
      ){
      return true;
    }
    return false;
  }

};


";

print "".
$machineName."StateMachine::".$machineName."StateMachine(){
  curState = 0;
  T1Binding = -1; 
  Backtrace *interleaver = new Backtrace();
  IDString = strdup(\"".(join ':', @first)." ".(join ':', @second)." ".(join ':', @intlv)."\");";

print "
}

enum SMAction ".$machineName."StateMachine::run(Backtrace *t, unsigned long tid){

  if(curState == 0){/*Empty*/

    if(
      (";
  for(0 .. ($#first - 1)){
    if($first[$_] ne "0xffffffffffffffff" &&
       $first[$_] ne "0xeeeeeeeeeeeeeeee" 
      ){ 
      print "
       t->bt[$_] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$_] == (void*)".$first[$_]." && ";
    }
  }
  if($first[$#first] ne "0xffffffffffffffff" &&
     $first[$#first] ne "0xeeeeeeeeeeeeeeee" 
    ){ 
    print "
       t->bt[$#first] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$#first] == (void*)".$first[$#first]."";
  }else{
    print " true ";
  }
print "
      )

    ){ 

      curState = 1;
      T1Binding = tid;
      return CONT;

    }else{

      return CONT;

    }

  }else if(curState == 1){/*Saw first event that shouldnt be intlv*/

    if(tid != T1Binding){
      if(
        ";
  for(0 .. ($#intlv - 1)){
    if($intlv[$_] ne "0xffffffffffffffff" &&
       $intlv[$_] ne "0xeeeeeeeeeeeeeeee" 
      ){ 
      print "
       t->bt[$_] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$_] == (void*)".$intlv[$_]." && ";
    }
  }
  if($intlv[$#intlv] ne "0xffffffffffffffff" &&
     $intlv[$#intlv] ne "0xeeeeeeeeeeeeeeee" 
    ){ 
    print "
       t->bt[$#intlv] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$#intlv] == (void*)".$intlv[$#intlv]."";
  }else{
    print " true ";
  }
print " 
      ){

        /*Interleaver is running*/
        return WAIT;

      }else{

        return CONT;

      }

    }else{

      if(";

  for(0 .. ($#second - 1)){
    if($second[$_] ne "0xffffffffffffffff" &&
       $second[$_] ne "0xeeeeeeeeeeeeeeee" 
      ){ 
      print "
       t->bt[$_] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$_] == (void*)".$second[$_]." && ";
    }
  }
  if($second[$#second] ne "0xffffffffffffffff" &&
     $second[$#second] ne "0xeeeeeeeeeeeeeeee" 
    ){ 
    print "
       t->bt[$#second] == (void*)0xeeeeeeeeeeeeeeee || t->bt[$#second] == (void*)".$second[$#second]."";
  }else{
    print " true ";
  }
print " 
        ){   
          /*original thread finished its pair.  done. back to start state */
          return TERM;
      }else{

        return CONT;

      }

    }

  }else{

    fprintf(stderr,\"Invalid State %u\\n\", curState);
    return CONT;

  }

  return CONT;
}

extern \"C\"{
StateMachineFactory *getFactory(){
  return new ".$machineName."StateMachineFactory(); 
}
}
";

}
