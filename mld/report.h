#ifndef REPORT
#define REPORT

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

#define BUFFER_MAX (1024 + 1)
#define TIME_MAX (19 + 1)	//yyyy-MM-dd hh:mm:ss

static char *local_time();

void init_report();

void report(const char * format, ...);

void uninit_report();

#endif
