#ifndef MEMLEAK
#define MEMLEAK
#include <QThread>

class MemLeak : public QThread{
public:
    MemLeak();
    ~MemLeak();

    void run();
    void nrun();
};

#endif // MEMLEAK

