#ifndef MEMLEAK
#define MEMLEAK
#include <QThread>

class MemLeak : public QThread{
public:
    MemLeak();
    ~MemLeak();

    void run();
};

#endif // MEMLEAK

