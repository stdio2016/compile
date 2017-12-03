#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

char *dupstr(char *str) {
  size_t n = strlen(str);
  char *nstr = malloc(n + 1);
  strcpy(nstr, str);
  return nstr;
}


