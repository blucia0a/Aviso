/*File-scoped constant definitions*/
#define SYNTHETIC_EVENT_INTERVAL 20000 /*50 Microseconds*/
#define RPB_SAMPLE_MAX 150000 /*Tenth of a second*/
#define RPB_SAMPLE_MIN 50000 /*20th of a second*/
#define BACKOFF_INTERVAL pthread_yield() 
#define EFL_SIZE 50000

static void terminationHandler(int signum);
static void thdDestructor(void *vt);
static void thdSigEnd(int signum);
static int GetTid();
static FILE *getRPBFile();

__thread pthread_key_t *tlsKey;

bool handlingFatalSignal = false; 
volatile bool finishedFatalSignal = false; 

/*RPB Output Things*/
pthread_mutex_t outputLock;
FILE *RPBFile;
int numDumped;
/*RPB Output Things*/

std::set<StateMachineFactory *> *Factories;

unsigned long maxMachinesGlobal;
/*event counters, for evaluation purposes*/

/*sigactions to be sure we use the program's signal handlers*/
struct sigaction sigABRTSaver;
struct sigaction sigSEGVSaver;
struct sigaction sigTERMSaver;
/*sigactions to be sure we use the program's signal handlers*/

/*Global Data in this module*/
pthread_t allThreads[MAX_NUM_THDS];
pthread_mutex_t allThreadsLock;
int tids[MAX_NUM_THDS];
unsigned int sequenceLength = 5;
bool initialized = false;
/*Global Data in this module*/
