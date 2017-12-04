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

int showType(struct Type *type) {
  int n = 0;
  if (type == NULL) {
    return printf("void");
  }
  switch (type->type) {
    case Type_INTEGER: n += printf("integer"); break;
    case Type_REAL: n += printf("real"); break;
    case Type_BOOLEAN: n += printf("boolean"); break;
    case Type_STRING: n += printf("string"); break;
    case Type_ARRAY: {
      struct Type *inner = type;
      while (inner->type == Type_ARRAY) {
        inner = inner->itemType;
      }
      n += showType(inner);
      n += printf(" ");
      inner = type;
      while (inner->type == Type_ARRAY) {
        int dim;
        if (inner->upperBound >= inner->lowerBound) 
          dim = inner->upperBound - inner->lowerBound + 1;
        else
          dim = inner->lowerBound - inner->upperBound + 1;
        n += printf("[%d]", dim);
        inner = inner->itemType;
      }
    }
    break;
    case Type_VOID: n += printf("void"); break;
  }
  return n;
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

void showConst(struct Constant c) {
  int n;
  switch (c.type) {
    case Type_INTEGER: n += printf("%d", c.integer); break;
    case Type_REAL: n += printf("%f", c.real); break;
    case Type_BOOLEAN: n += printf(c.boolean ? "true" : "false"); break;
    case Type_STRING: n += printf("\"%s\"", c.str); break;
  }
}
