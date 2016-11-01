#ifndef PRINT
#define PRINT

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

#define BUFFER_MAX (1024 + 1)
#define TIME_MAX (19 + 1)	//yyyy-MM-dd hh:mm:ss

char *local_time();

void output_init();

void output_print(const char * format, ...);

void output_uninit();

#endif
