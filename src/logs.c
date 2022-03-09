#include "logs.h"

#include "esc_def.h"

#include <stdarg.h>
#include <stdio.h>

void log_info(const char* fmt, ...)
{
    fprintf(stdout, ESC_SGR_FGR_COLOR_RGB(250, 255, 50));
    fprintf(stdout, "info:> ");
    fprintf(stdout, ESC_SGR_CLEAR);

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    fprintf(stdout, "\n");
}

void log_error(const char* fmt, ...)
{
    fprintf(stderr, ESC_SGR_FGR_COLOR_RGB(255, 50, 50));
    fprintf(stderr, "error:> ");
    fprintf(stderr, ESC_SGR_CLEAR);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n");
}

void log_message(const char* fmt, ...)
{
    fprintf(stdout, ESC_SGR_FGR_COLOR_RGB(150, 255, 150));
    fprintf(stdout, "message:> ");
    fprintf(stdout, ESC_SGR_CLEAR);

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    fprintf(stdout, "\n");
}