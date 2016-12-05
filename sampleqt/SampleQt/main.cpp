#include "mld.h"
#include <QCoreApplication>

void test(){
    char *new_leak = new char[499];
    char *malloc_leak = (char *)malloc(500);
    char *calloc_leak = (char *)calloc(501, 1);
    char *heap_alloc_leak = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 502);
}

int main(int argc, char *argv[])
{
    MingwLeakDetector();

    test();

    QCoreApplication a(argc, argv);

    return a.exec();
}

