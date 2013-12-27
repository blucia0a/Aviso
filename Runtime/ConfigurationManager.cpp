#include "ConfigurationManager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

extern "C"{

aviso_config *globalConfig;

aviso_config *loadConfigurationFile(char *configFileName){

  aviso_config *conf = (aviso_config *)malloc( sizeof(aviso_config) );

  FILE *configFile;
  configFile = fopen(configFileName, "r");

  char tok1[256]; char tok2[256];
  while( fscanf( configFile, "%s %s\n", tok1, tok2 ) != EOF ){

    if( !strncmp( "AVISO_SampleRpb", tok1, strlen(tok1) > strlen("AVISO_SampleRpb") ? strlen(tok1) : strlen("AVISO_SampleRpb") ) ){

      conf->correctRunSampleRpb = (char *)malloc( strlen(tok2) + 1 );
      strncpy( conf->correctRunSampleRpb, tok2, strlen(tok2) + 1 );

    }else if( !strncmp( "AVISO_SampleInterval", tok1, strlen(tok1) > strlen("AVISO_SampleInterval") ? strlen(tok1) : strlen("AVISO_SampleInterval") ) ){
    
      char *end; 
      conf->correctRunSampleIntervalLow = strtol( tok2, &end, 10 );
     
      end = end + 1; 
      conf->correctRunSampleIntervalHigh = strtol( end, NULL, 10 );

    }else if( !strncmp( "AVISO_SeqLen", tok1, strlen(tok1) > strlen("AVISO_SeqLen") ? strlen(tok1) : strlen("AVISO_SeqLen") ) ){
   
      conf->sequenceLength = (unsigned) strtol( tok2, NULL, 10 );
 
    }else if( !strncmp( "AVISO_RpbDir", tok1, strlen(tok1) > strlen("AVISO_RpbDir") ? strlen(tok1) : strlen("AVISO_RpbDir") ) ){
      
      conf->rpbDir = (char *)malloc( strlen(tok2) + 1 );
      strncpy( conf->rpbDir, tok2, strlen(tok2) + 1 );

    }else if( !strncmp( "AVISO_Json", tok1, strlen(tok1) > strlen("AVISO_Json") ? strlen(tok1) : strlen("AVISO_Json") ) ){
      
      conf->useJSON = (int) strtol( tok2, NULL, 10 );

    }else if( !strncmp( "AVISO_RpbServerHost", tok1, strlen(tok1) > strlen("AVISO_RpbServerHost") ? strlen(tok1) : strlen("AVISO_RpbServerHost") ) ){

      conf->avisoServerName = (char *)malloc( strlen(tok2) + 1 );
      strncpy( conf->avisoServerName, tok2, strlen(tok2) + 1 );
      

    }else if( !strncmp( "AVISO_RpbServerPort", tok1, strlen(tok1) > strlen("AVISO_RpbServerPort") ? strlen(tok1) : strlen("AVISO_RpbServerPort") ) ){
      
      conf->avisoServerPort = (char *)malloc( strlen(tok2) + 1 );
      strncpy( conf->avisoServerPort, tok2, strlen(tok2) + 1 );

    }else{

      fprintf(stderr,"[AVISO] Error Loading Configuration %s=%s\n", tok1, tok2 );
      exit(1);

    }

  }
  
  if( conf->avisoServerName == NULL || conf->avisoServerPort == NULL ){

      fprintf(stderr,"[AVISO] Error Loading aviso server name or port http://%s:%s\n", conf->avisoServerName, conf->avisoServerPort);
      exit(1);
  
  }

  int len = strlen("http://:/tick") + strlen( conf->avisoServerName ) + strlen( conf->avisoServerPort ) + 1;
  conf->tickPostURL = (char *)malloc( len );
  sscanf( conf->tickPostURL, "http://%s:%s/tick", conf->avisoServerName, conf->avisoServerPort );
  
  len = strlen("http://:/correct") + strlen( conf->avisoServerName ) + strlen( conf->avisoServerPort ) + 1;
  conf->correctPostURL = (char *)malloc( len );
  sscanf( conf->correctPostURL, "http://%s:%s/correct", conf->avisoServerName, conf->avisoServerPort );
  
  len = strlen("http://:/start") + strlen( conf->avisoServerName ) + strlen( conf->avisoServerPort ) + 1;
  conf->startPostURL = (char *)malloc( len );
  sscanf( conf->startPostURL, "http://%s:%s/start", conf->avisoServerName, conf->avisoServerPort );
  
  len = strlen("http://:/end") + strlen( conf->avisoServerName ) + strlen( conf->avisoServerPort ) + 1;
  conf->endPostURL = (char *)malloc( len );
  sscanf( conf->endPostURL, "http://%s:%s/end", conf->avisoServerName, conf->avisoServerPort );
  
  return conf;   

}

char *AvisoConfig_getTickPostURL(aviso_config *c){
  return c->tickPostURL;
}

char *AvisoConfig_getCorrectPostURL(aviso_config *c){
  return c->correctPostURL;
}

char *AvisoConfig_getStartPostURL(aviso_config *c){
  return c->startPostURL;
}

char *AvisoConfig_getEndPostURL(aviso_config *c){
  return c->endPostURL;
}

char *AvisoConfig_getCorrectRunSampleRpb(aviso_config *c){
  assert(c != NULL);
  return c->correctRunSampleRpb;
}

unsigned long AvisoConfig_getCorrectRunSampleIntervalLow(aviso_config *c){
  return c->correctRunSampleIntervalLow;
}

unsigned long AvisoConfig_getCorrectRunSampleIntervalHigh(aviso_config *c){
  return c->correctRunSampleIntervalHigh;
}

unsigned AvisoConfig_getSequenceLength(aviso_config *c){
  return c->sequenceLength;
}

char *AvisoConfig_getRpbDir(aviso_config *c){
  return c->rpbDir;
}

int AvisoConfig_getUseJson(aviso_config *c){
  return c->useJSON;
}

}
