#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "STQueue.h"
#include "ClockPortability.h"
#include "ThreadData.h"
#include "ConfigurationManager.h"

extern aviso_config *globalConfig;

/*All threads dump to this file*/
static FILE *correctRunFile;
static char *correctRunFileName;

/*This lock protects access to the correctRunFile*/
static pthread_mutex_t correctRunFileLock;

unsigned long target = 0;
pthread_t watcher;
bool watcherStarted = false;

static long lastFirstDump = -1;

/*This interval is the amount of time in nanoseconds between a thread's dumps*/
/*This should be very large or performance will suck*/
static unsigned long dumpIntervalLow;
static unsigned long dumpIntervalHi;

/*This flag says whether the code in this module should do anything*/
static bool active;

/*This is the time in nanoseconds since the epoch of the last dump*/
/*__thread unsigned long lastDump = 0;
__thread unsigned long correctRunDumpInterval = 0;
__thread bool correctDumped = false;*/


/*External reference to the RPB that will be dumped*/
//extern __thread STQueue *myRPB;

extern __thread pthread_key_t *tlsKey;

void initializeCorrectRunDump(){

  /*Assume something messes up during initialization*/
  active = false;

  /*Get the file that we'll dump correct runs to*/
  correctRunFileName = AvisoConfig_getCorrectRunSampleRpb(globalConfig);
  
  correctRunFile = fopen( correctRunFileName, "w" );
  if( correctRunFile == NULL ){

    return;

  }

  dumpIntervalLow = AvisoConfig_getCorrectRunSampleIntervalLow(globalConfig);
  dumpIntervalHi  = AvisoConfig_getCorrectRunSampleIntervalHigh(globalConfig);

  fprintf(stderr,"[AVISO] Sampling correct execution with interval %lu-%lu\n",dumpIntervalLow,dumpIntervalHi);

  struct timespec t;
  clocktime(&t);

  unsigned long now = t.tv_sec*1000000000 + t.tv_nsec;
  unsigned long range = dumpIntervalLow + (rand() % (dumpIntervalHi - dumpIntervalLow));
  target = now + range;

  /*Got the file with no error.  Got the interval with no error. GO!*/
  active = true; 

}

inline bool timeForCorrectDump(unsigned long now){

  ThreadData *t = (ThreadData *)pthread_getspecific(*tlsKey);

  /*Assumes now is larger than lastDump*/
  if( !t->correctDumped && now > target && target != 0){

    t->correctDumped = true;
    fprintf(stderr,"Dumping %lu %lu \n",now,t->lastDump); 
    return true;

  }

  return false;

}


void sendRPB( ){

  active = false;

  CURL *curl;
  CURLcode res;
  struct stat file_info;

  /* get the file size of the local file */ 
  int fd = open(correctRunFileName, O_RDONLY) ;

  fstat(fd, &file_info);
  
  FILE *fdPtr = fdopen(fd, "rb");
  
  if( !fdPtr ){

    fprintf(stderr,"[AVISO] Couldn't open file to store correct execution: %s\n", correctRunFileName);
    return;

  }
 
  /* In windows, this will init the winsock stuff */ 
  curl_global_init(CURL_GLOBAL_ALL);
 
  /* get a curl handle */ 
  curl = curl_easy_init();

  if(curl) {

    /* we want to use our own read function */ 
    //curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
 
    /* enable uploading */ 
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
 
    /* HTTP PUT please */ 
    curl_easy_setopt(curl, CURLOPT_PUT, 1L);
 
    /* specify target URL, and note that this URL should include a file
     * name, not only a directory */ 
    const char *url = AvisoConfig_getCorrectPostURL(globalConfig);
    curl_easy_setopt(curl, CURLOPT_URL, url);
 
    /* now specify which file to upload */ 
    curl_easy_setopt(curl, CURLOPT_READDATA, fdPtr);
 
    /* provide the size of the upload, we specicially typecast the value
     * to curl_off_t since we must be sure to use the correct data size */ 
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                     (curl_off_t)file_info.st_size);
 
    /* Now run off and do what you've been told! */ 
    res = curl_easy_perform(curl);
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }

  fclose(fdPtr); /* close the local file */ 

  curl_global_cleanup();
  return;

}

void *watcherFn(void *v){

  sleep( 5 );
  pthread_mutex_lock(&correctRunFileLock);

  //fprintf(correctRunFile, "\n{\"time\" : 0,\"thread\" : 0, \"backtrace\" : [\"(nil)\",\"(nil)\",\"(nil)\",\"(nil)\",\"(nil)\"]} ]\n");

  fclose( correctRunFile );
  fprintf(stderr,"Watcher sending RPB...\n");
  sendRPB( );    
  fprintf(stderr,"Watcher sent RPB\n");
  correctRunFile = fopen( correctRunFileName, "w" );

  pthread_mutex_unlock(&correctRunFileLock);
  
  pthread_exit(NULL);

}

inline void dumpCorrect( unsigned long now ){

  ThreadData *t = (ThreadData *)pthread_getspecific(*tlsKey);
  
  fprintf(stderr,"Dumping for a correct run in dumpCorrect\n");
  pthread_mutex_lock(&correctRunFileLock);

  if( active == false ){
    pthread_mutex_unlock(&correctRunFileLock);
    return;
  }

  if( watcherStarted == false ){
    watcherStarted = true;
    pthread_create(&watcher,NULL,watcherFn,NULL);
  }

  t->myRPB->Dump( correctRunFile );
  int ret = fflush( correctRunFile );
  if( ret != 0 ){

    fprintf(stderr,"[AVISO] There was an error flushing the RPB\n");

  } 

  pthread_mutex_unlock(&correctRunFileLock);
  
}

extern "C" void dumpIfRequired( unsigned long now ){

  if( !active ){ 

    return; 

  }


  if( timeForCorrectDump(now) ){

    fprintf(stderr,"Dumping for a correct run\n");
    dumpCorrect(now);

  }

}
