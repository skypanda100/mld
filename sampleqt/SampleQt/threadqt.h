#ifndef THREADQT
#define THREADQT
#include <QThread>

class ThreadQT : public QThread{
public:
    ThreadQT();
    ~ThreadQT();

    void run();
};


#endif // THREADQT

