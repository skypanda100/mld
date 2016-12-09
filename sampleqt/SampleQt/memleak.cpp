#include "memleak.h"
#include <windows.h>
#include <QDebug>

MemLeak::MemLeak(){
}

MemLeak::~MemLeak(){

}

void MemLeak::run(){
    char *new_leak = (char *)malloc(504);
}

void MemLeak::nrun(){
    char *new_leak_1 = new char[505];
    char *new_leak_2 = new char[506];
}
