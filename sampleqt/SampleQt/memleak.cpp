#include "memleak.h"
#include <windows.h>
#include <QDebug>

MemLeak::MemLeak(){
}

MemLeak::~MemLeak(){

}

void MemLeak::run(){
    char *new_leak = new char[504];
    qDebug() << "run";
}
