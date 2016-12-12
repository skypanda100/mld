#include "threadcpp.h"

ThreadCPP::ThreadCPP(){

}

ThreadCPP::~ThreadCPP(){

}

void ThreadCPP::start(){
    pthread_t t;
    int ret = pthread_create(&t, NULL, ThreadCPP::leak, NULL);
}

void *ThreadCPP::leak(void*){
    char *new_leak = new char[505];
    return NULL;
}
