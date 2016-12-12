#include "mld.h"
#include <QApplication>
#include "mainwindow.h"
#include "threadqt.h"
#include "threadcpp.h"

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
    MainWindow *w = new MainWindow;
    w->show();

    ThreadQT threadQT;
    threadQT.start();

    ThreadCPP threadCPP;
    threadCPP.start();

    return a.exec();
}

