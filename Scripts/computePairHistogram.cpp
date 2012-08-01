#include <ext/hash_map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <assert.h>

using __gnu_cxx::hash_map;
using __gnu_cxx::hash;
using namespace std;

#define NUM_THREADS 16
#define THRESHOLD 10
#define BTLEN 5

namespace __gnu_cxx 
{                                                                                             
  template<> struct hash< std::string >                                                       
  {                                                                                           
    size_t operator()( const std::string& x ) const                                           
    {                                                                                         
      return hash< const char* >()( x.c_str() );                                              
    }                                                                                         
  };                                                                                          
}


vector<string> files;
bool done = false;
pthread_mutex_t gLock = PTHREAD_MUTEX_INITIALIZER;

pthread_rwlock_t histoLock;
/*Read lock histo to find your bin. Write lock it to create a new bin*/
hash_map< string, hash_map<string, vector<unsigned long> > > histo;



class Event{

public:

  Event(){
    bt = vector<unsigned long>();
    thread = 0;
  }
  
  string btString(){

    stringstream s;
    string st;
    s << hex; 
    for(vector<unsigned long>::iterator i = bt.begin();
        i != bt.end(); i++){
      s << "0x" << *i;
      if( i + 1 != bt.end() ){
        s << ":";
      }
    }
    st = s.str();
    return st;
  }

  string toString(){

    string st;
    stringstream s;
    s << thread;
    s << hex;
    for(vector<unsigned long>::iterator i = bt.begin();
        i != bt.end(); i++){
      s << ":";
      s << *i;
    }
    st = s.str();
    //cerr << st << endl;
    return st;
  }

  vector<unsigned long> bt;
  unsigned long thread;

};


unsigned long normalizeAddr(char *a){

  unsigned long ad;
  if( !strcmp(a,"(nil)") ){
    ad = 0;
  }else{
    sscanf(a,"%lx", &ad);
  }
  if( ad == 0 ){
    return 0x0;
  }else if( ad > 0x700000000000  ){
    return 0xffffffffffffffff;
  }else{
    return ad;
  }
}

Event *lineToEvent(string s){

  Event *newE = new Event();  
  const char *str_orig = s.c_str();
  char *str = strstr(str_orig,":");
  unsigned long thread = atoi((s.substr(0, str - str_orig)).c_str());
  newE->thread = thread;

  char *add;  
  char *sp;
  string output("");
  add = strtok_r(str, ":", &sp);
  
  newE->bt.push_back( normalizeAddr(add) );
  //free(add);

  while( add = strtok_r(NULL, ":", &sp) ){

    unsigned long aq = normalizeAddr(add);
    newE->bt.push_back( aq );
    //free(add);
  }
  while( newE->bt.size() < BTLEN ){
    newE->bt.push_back(0);
  }

  return newE;
  
}

void countPairs(vector<Event*> &events){

  for( vector<Event*>::iterator fst = events.begin();
       fst != events.end(); fst++){
    
    for( vector<Event*>::iterator snd = fst;
         snd != events.end() && snd - fst < THRESHOLD; snd++){

      if( (*fst)->thread != (*snd)->thread ){

        string fString = (*fst)->btString();
        string sString = (*snd)->btString();
      
        bool found = false;

        pthread_rwlock_rdlock( &histoLock );
        if( histo.find( fString ) != histo.end() ){
          
          if( histo[fString].find( sString ) != histo[fString].end() ){
            found = true;
            histo[fString][sString][snd-fst]++;
          }

        }
        pthread_rwlock_unlock( &histoLock );

        if( !found ){

          pthread_rwlock_wrlock( &histoLock );

          if( (histo.find( fString )) == histo.end() ){
            histo.insert(
              std::pair<string,
                        hash_map<string, vector<unsigned long> > >(fString,
                                                                   hash_map<string, vector<unsigned long> >()));
          }

          if( histo[fString].find(sString) == histo[fString].end() ){
            histo[fString].insert(std::pair<string,
                                            vector<unsigned long> >(sString,
                                                                    vector<unsigned long>()));
            for(int i = 0; i < THRESHOLD; i++ ){
              histo[fString][sString].push_back(0);
            }
          } 
          assert(snd - fst < THRESHOLD);
          histo[fString][sString][snd-fst]++;
          pthread_rwlock_unlock( &histoLock );

        }
         
      } 
      
    }
    
  }

}

void process(string fname){

  vector<Event*> events;
  ifstream f( fname.c_str() );     
  string input_line;

  while( getline(f, input_line) ){

    if( input_line.size() < 5 ){ 
      continue; 
    }

    events.push_back( lineToEvent(input_line) );
    //cerr  << (unsigned long)pthread_self() << ": " << lineToEvent(input_line) <<  endl;

  }

  countPairs(events);
  for(vector<Event*>::iterator i = events.begin(); i!= events.end(); i++){
    delete *i;
  }
  events.clear(); 

/*
  for( vector<Event>::iterator i = events.begin();
       i != events.end(); i++ ){
    cerr << i->toString() << endl;
  }*/

}


void *runner(void *v){

  unsigned long me = (unsigned long)v;
  while( true ){

    string myFile;
    pthread_mutex_lock(&gLock);

    if( done ){

      pthread_mutex_unlock(&gLock);
      pthread_exit(0);

    }

    if( files.size() <= 0 ){

      done = true;
      pthread_mutex_unlock(&gLock);
      pthread_exit(0);

    }
 
    myFile = files.back();
    files.pop_back();
    //cerr << "Thread " << me << " " << files.size() << " to go" << endl;
    pthread_mutex_unlock(&gLock);

    process(myFile);

  }
  
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {

    std::stringstream ss(s);
    std::string item;

    while(std::getline(ss, item, delim)) {

        elems.push_back(item);

    }

    return elems;

}

std::vector<std::string> split(const std::string &s, char delim) {

    std::vector<std::string> elems;
    return split(s, delim, elems);

}

void deserializeFromFile( char *fname ){

  ifstream f( fname );     
  string input_line;

  while( getline(f, input_line) ){

    if( input_line.size() < 5 ){ 
      continue; 
    }

    std::vector<std::string> toks = split(input_line, ' ');
    std::vector<std::string>::iterator i = toks.begin();
   
    i++;
    i++;

    for( ;
         i != toks.end();
         i++ ){

      histo[ toks[ 0 ] ][ toks[ 1 ] ].push_back(  (unsigned long)atol( i->c_str() )  );

    }

  }

}

int main(int argc, char *argv[]){

  for( int i = 1; i < argc; i++){
    fprintf(stderr,"[[[[[%s]]]]]",argv[i]);
  }
  char *fname;

  pthread_rwlock_init(&histoLock,NULL);
  for( int i = 2; i < argc; i++){
    files.push_back(string(argv[i]));
  }

  deserializeFromFile( argv[1] );

  //cerr << "Size is: " << files.size() << endl;

  pthread_t threads[NUM_THREADS];
  for(int i = 0; i < NUM_THREADS; i++){
    pthread_create(&(threads[i]),NULL,runner,(void *)i);
  }
  
  for(int i = 0; i < NUM_THREADS; i++){
    pthread_join(threads[i],NULL);
  }

  for( hash_map<string, hash_map<string, vector<unsigned long> > >::iterator it = histo.begin();
       it != histo.end(); it++ ){

    for(hash_map<string, vector<unsigned long> >::iterator it2 = it->second.begin();
        it2 != it->second.end(); it2++){

      cout << it->first << " " << it2->first << " ";

      for(vector<unsigned long>::iterator vi = it2->second.begin();
          vi != it2->second.end(); vi++ ){

        cout << *vi << " ";


      }

      cout << endl;

    }

  }
  
}
