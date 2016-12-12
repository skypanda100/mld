#ifndef THREADCPP
#define THREADCPP
#include <pthread.h>

class ThreadCPP{

public:
    ThreadCPP();
    ~ThreadCPP();

    void start();

    static void *leak(void *);
};

#endif // THREADCPP

