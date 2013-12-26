#include <curl/curl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

extern "C"{

void AVISO_TimeTick(){

  CURL *curl;
  CURLcode res;

  static char *fsms = getenv("IR_Plugins");
  static pid_t mypid = getpid();
  char *cursor;
  int len = 0;
  if( fsms != NULL ){

    len = strlen(fsms);
    cursor = fsms + len - 1;
    while( cursor > fsms && *cursor != '/'){ cursor--; }
    cursor++;
    fprintf(stderr,"Found the string! %s\n",cursor);
    len = strlen(cursor);

  }
  char postdata[ len + 20 ];
  memset( postdata, 0, len + 20 );
  sprintf(postdata,"fsm=%s;&pid=%lu",fsms == NULL ? "baseline" : cursor, (unsigned long)mypid);

  fprintf(stderr,"[AVISO] Tick Post: %s\n",postdata);
 
  curl = curl_easy_init();
  if(curl) {

     
    curl_easy_setopt(curl, CURLOPT_URL, "http://pinga.cs.washington.edu:22221/tick");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
 
    /* if we don't provide POSTFIELDSIZE, libcurl will strlen() by
 *        itself */ 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)len+20);
 
    res = curl_easy_perform(curl);
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }

}


}
