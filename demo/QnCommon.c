//
// Created by chenh on 2018/6/19.
//
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include "QnCommon.h"

#define ARGSBUF_LEN 256

void DemoPrintTime(void)
{
    struct tm *tblock = NULL;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    tblock = localtime(&tv.tv_sec);
    fprintf(stderr, "%d/%d/%d %d:%d:%d:%d",
           1900+tblock->tm_year, 1+tblock->tm_mon,
           tblock->tm_mday, tblock->tm_hour,
           tblock->tm_min, tblock->tm_sec, tv.tv_usec);
}

void QnDemoPrint(IN int level, IN const char *fmt, ...)
{
    if (level <= DEMO_PRINT) {
        DemoPrintTime();

        va_list args;
        char argsBuf[ARGSBUF_LEN] = {0};
        if (argsBuf == NULL) {
            return;
        }
        va_start(args, fmt);
        vsprintf(argsBuf, fmt, args);
        fprintf(stderr, ">>%s\n", argsBuf);
        va_end(args);
		
    }
}
