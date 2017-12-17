#include "errReport.h"
#include <stdio.h>
#include <stdarg.h>

extern int linenum;             /* declared in tokens.l */

void sementicError(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  printf(RED_TEXT "<Error>" NORMAL_TEXT " found in Line " BOLD_TEXT "%d: " NORMAL_TEXT, linenum);
  vprintf(fmt, ap);
  printf("\n");
  va_end(ap);
}

