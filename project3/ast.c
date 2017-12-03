#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

struct Constant *createConst(enum TypeEnum type) {
  struct Constant *node = malloc(sizeof(struct Constant));
  node->type = type;
  return node;
}

char *dupstr(char *str) {
  size_t n = strlen(str);
  char *nstr = malloc(n + 1);
  strcpy(nstr, str);
  return nstr;
}


