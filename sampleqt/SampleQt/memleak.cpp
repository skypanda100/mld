#include "memleak.h"

MemLeak::MemLeak(){




    char *new_leak = new char[504];
}

MemLeak::~MemLeak(){

}
