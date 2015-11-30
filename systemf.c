#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

void systemf(const char* format, ...){
        va_list vl;
        va_start(vl, format);
        int buf_size = vsnprintf(NULL, 0, format, vl);
        va_end(vl);
        va_start(vl, format);
        char buf[buf_size+1];
        vsprintf(buf, format, vl);
        va_end(vl);
        system(buf);
}

