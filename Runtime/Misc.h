#ifndef __MISC_H_
#define __MISC_H_

/*Backtrace length - Controls Event Specificity*/
#define BTLEN 5

/*ONLINE PRUNING PARAMETERS*/
/*Events separated by SE_INT_MIN or less are omitted*/
#define SE_INT_MIN 1000/* 10  us*/

/*Events separated by SE_INT_1 or less are truncated to one address*/
#define SE_INT_1  2000 /* 50  us*/

/*MAX_NUM_THDS controls the size of a statically allocated thread buffer*/
#define MAX_NUM_THDS 256

/*Max number of machines a thread can have active*/
#define AVOID_SET_SIZE 10

/*Shorthand for the intrinsic cas operation*/
#define CAS(a,b,c) __sync_bool_compare_and_swap(a, b, c)

/*This is the amount of time a thread waits before trying
 *a delayed event again.  In us*/
#define AVOIDANCE_WAIT 10
#define AVOIDANCE_MAX_RETRIES 100

#endif
