#include "mld.h"
#include <QApplication>
#include "mainwindow.h"

class MemLeak{
public:
    MemLeak(){
        m_i = new int[100];
    }
    ~MemLeak(){

    }
public:
    int *m_i;
};
MemLeak *ml = new MemLeak;

void test(){
    char *new_leak = new char[499];
    char *malloc_leak = (char *)malloc(500);
    char *calloc_leak = (char *)calloc(501, 1);
    char *heap_alloc_leak = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 502);
}

int main(int argc, char *argv[])
{
    test();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}

