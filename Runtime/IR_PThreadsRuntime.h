#include <pthread.h>
extern "C"{
int IR_LockInit(pthread_mutex_t *lock,pthread_mutexattr_t *attr);
int IR_Unlock(pthread_mutex_t *lock);
int IR_Lock(pthread_mutex_t *lock);
int IR_ThreadExit(void *value_ptr);
void IR_SyntheticEvent();

void Aviso_Constructor()__attribute__((constructor));
void IR_Destructor()__attribute__((destructor));
}
