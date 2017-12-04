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

struct Type *copyType(struct Type *ptr) {
  struct Type *cp = malloc(sizeof(struct Type));
  cp->type = ptr->type;
  cp->upperBound = ptr->upperBound;
  cp->lowerBound = ptr->lowerBound;
  if (ptr->type == Type_ARRAY) {
    cp->itemType = copyType(ptr->itemType);
  }
  else
    cp->itemType = NULL;
  return cp;
}

void destroyType(struct Type *ptr, int includeSelf) {
  if (ptr != NULL && ptr->itemType != NULL) {
    destroyType(ptr->itemType, 1);
  }
  if (includeSelf)
    free(ptr);
}

struct Constant copyConst(struct Constant c) {
  struct Constant d = c;
  if (c.type == Type_STRING) {
    d.str = dupstr(c.str);
  }
  return d;
}

void destroyConst(struct Constant c) {
  if (c.type == Type_STRING) {
    free(c.str);
  }
}
