#ifndef _CONFIGURATIONMANAGER_H_
#define _CONFIGURATIONMANAGER_H_

typedef struct aviso_config{

  /*Server Settings*/
  char *avisoServerName;
  char *avisoServerPort;

  char *tickPostURL;
  char *correctPostURL;
  char *startPostURL;
  char *endPostURL;

  /*Correct Run Sampling Configuration*/
  char *correctRunSampleRpb;
  unsigned long correctRunSampleIntervalLow;
  unsigned long correctRunSampleIntervalHigh;

  /*Recent Past Buffer Settings*/
  char *rpbDir;
  unsigned sequenceLength;
  int useJSON;
    


} aviso_config;

extern "C"{

/*public interfaces*/
aviso_config *loadConfigurationFile(char *configFileName);

char *AvisoConfig_getTickPostURL(aviso_config *c);
char *AvisoConfig_getCorrectPostURL(aviso_config *c);
char *AvisoConfig_getStartPostURL(aviso_config *c);
char *AvisoConfig_getEndPostURL(aviso_config *c);

char *AvisoConfig_getCorrectRunSampleRpb(aviso_config *c);
unsigned long AvisoConfig_getCorrectRunSampleIntervalLow(aviso_config *c);
unsigned long AvisoConfig_getCorrectRunSampleIntervalHigh(aviso_config *c);

unsigned AvisoConfig_getSequenceLength(aviso_config *c);
char *AvisoConfig_getRpbDir(aviso_config *c);
int AvisoConfig_getUseJson(aviso_config *c);

}
#endif
