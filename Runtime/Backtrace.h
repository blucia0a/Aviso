#ifndef _BACKTRACE_H_
#define _BACKTRACE_H_
#include <stdio.h>
#include <stdlib.h>
#include "Misc.h"


class Backtrace{
public:

  void **bt;

  Backtrace(){
    bt = (void**)calloc(BTLEN,sizeof(void *));
    for(int i = 0; i < BTLEN; i++){
      this->bt[i] = NULL;
    }
  }
  
  Backtrace(const Backtrace& other){
    for(int i = 0; i < BTLEN; i++){
      this->bt[i] = other.bt[i];
    }
  }
  Backtrace(const Backtrace *other){
    for(int i = 0; i < BTLEN; i++){
      this->bt[i] = other->bt[i];
    }
  }

  Backtrace& operator=(const Backtrace& other){
    for(int i = 0; i < BTLEN; i++){
      this->bt[i] = other.bt[i];
    }
    return *this;
  }

  Backtrace(void **B){
    for(int i = 0; i < BTLEN; i++){
        this->bt[i] = B[i];
    }
  }
 
  ~Backtrace(){
    //fprintf(stderr,"Deleting a backtrace\n"); 
  }

  void set(void **B){
    for(int i = 0; i < BTLEN; i++){
      this->bt[i] = B[i];
    }
  }

  void print(FILE *f){
    fprintf(f,"[");
    for(int i = 0; i < BTLEN; i++){
      if(i < BTLEN-1){
        fprintf(f,"%p->",bt[i]);
      }else{
        fprintf(f,"%p",bt[i]);
      }
    }
    fprintf(f,"] ");
  }

  static size_t hash(const Backtrace *b){
    size_t hashval;
    for(int i = 0; i < BTLEN; i++){
      hashval ^= (unsigned long)b->bt[i]; 
    } 
    return hashval;
  }
  
  static bool areEqual(const Backtrace *b, void **other){
    if(other == NULL){
      if(b == NULL){
        return true;
      }else{
        return false;
      }
    }
    for(int i = 0; i < BTLEN; i++){
      if(b->bt[i] != other[i]){
        return false;
      }
    } 
    return true;
  }
 
  static bool areEqual(const Backtrace *b, const Backtrace *other){
    if(other == NULL){
      if(b == NULL){
        return true;
      }else{
        return false;
      }
    }
    for(int i = 0; i < BTLEN; i++){
      if(b->bt[i] != other->bt[i]){
        return false;
      }
    } 
    return true;
  }

  void *operator[](int i){
    if(i > BTLEN-1){
      //fprintf(stderr,"Accessing out of bounds Backtrace element!\n"); 
      return NULL;
    }
    return this->bt[i]; 
  }

};

struct BTCompare{
  bool operator()(const Backtrace *v1,const Backtrace *v2) const{
    return Backtrace::areEqual(v1,v2) ? 0 : (v1->bt[0] > v2->bt[1] ? 1 : -1);
  } 
};

struct BTEquals{
  bool operator()(const Backtrace *v1,const Backtrace *v2) const{
    return Backtrace::areEqual(v1,v2);
  } 
};

struct LooseBTEquals{
  bool operator()(const Backtrace *b,const Backtrace *other) const{

    if(other == NULL){

      if(b == NULL){

        return true;

      }else{

        return false;

      }

    }

    for(int i = 0; i < BTLEN; i++){
    
      if((unsigned long)b->bt[i] == 0xeeeeeeeeeeeeeeee ||
         (unsigned long)other->bt[i] == 0xeeeeeeeeeeeeeeee){
        continue;
      }

      if((unsigned long)b->bt[i] > 0x700000000000 && 
         (unsigned long)other->bt[i] > 0x700000000000){
        /*if they're both over this magic value, they're ok, dont' return false*/
        continue;
      }


      if( b->bt[i] != other->bt[i] ){
        return false;
      }
    } 
    return true;
  }
};

struct BTHash{
  bool operator()(const Backtrace *v1) const{
    //fprintf(stderr,"Hashing\n");
    return Backtrace::hash(v1);
  } 
};

#endif
