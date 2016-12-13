#include "threadqt.h"

ThreadQT::ThreadQT(){

}

ThreadQT::~ThreadQT(){

}

void ThreadQT::run(){
    char *new_leak = new char[504];
    char *malloc_leak = (char *)malloc(506);
}
