#include "log_error.h"
#include <stdio.h>

static void report(int line, const char *where, const char *message) {
  printf("[line %d ] Error %s : %s", line, where, message);
}

void log_error(int line, const char *message) { report(line, "", message); }
