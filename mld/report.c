#include "report.h"

static char g_output[BUFFER_MAX] = {'\0'};
static char time_buf[TIME_MAX] = {'\0'};
static FILE *trace_file = NULL;

static char *local_time()
{
	memset(time_buf, TIME_MAX, '\0');
	time_t t = time( 0 );
    strftime(time_buf, TIME_MAX, "%Y-%m-%d %H:%M:%S", localtime(&t));
    return time_buf;
}

void init_report()
{
	if (trace_file == NULL) 
	{
		trace_file = fopen("trace.log", "w");
		report("******************************************************************\n"
				"*******                 %s                *******\n"
		     	"******************************************************************\n"
			 	, local_time());
	}
}

void report(const char * format, ...)
{
	memset(g_output, '\0', BUFFER_MAX);
	
	va_list ap;
	va_start(ap, format);
	vsnprintf(g_output, BUFFER_MAX, format, ap);
	va_end(ap);

	if (trace_file != NULL) {
		fprintf(trace_file, "%s", g_output);
		fflush(trace_file);
	}
}

void uninit_report()
{
	if (trace_file != NULL) 
	{
		fflush(trace_file);
		fclose(trace_file);
	}
}

