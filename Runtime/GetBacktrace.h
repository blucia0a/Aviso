#ifndef _GETBACKTRACE_H_
#define _GETBACKTRACE_H_

#define MAX_STACK_DEPTH 100

extern "C" void _get_backtrace(void **baktrace,int addrs);

#endif
