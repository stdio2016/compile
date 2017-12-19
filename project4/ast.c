#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

char *dupstr(const char *str) {
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

void destroyType(struct Type *ptr) {
  if (ptr != NULL && ptr->itemType != NULL) {
    destroyType(ptr->itemType);
  }
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

struct Type *createScalarType(enum TypeEnum type) {
  struct Type *t = malloc(sizeof(struct Type));
  t->type = type;
  t->itemType = NULL;
  return t;
}

Bool typeEqual(struct Type *t1, struct Type *t2) {
  while (t1->type == Type_ARRAY && t2->type == Type_ARRAY) {
    // check size of each dimension
    if (t1->upperBound - t1->lowerBound != t2->upperBound - t2->lowerBound) {
      return False;
    }
    t1 = t1->itemType;
    t2 = t2->itemType;
  }
  if (t1->type != t2->type) return False;
  return True;
}

struct Expr *createExpr(enum Operator op, struct Expr *arg1, struct Expr *arg2) {
  struct Expr *n = malloc(sizeof(struct Expr));
  n->op = op;
  n->type = NULL;
  n->next = NULL;
  n->args = arg1;
  if (arg1 != NULL) {
    arg1->next = arg2;
  }
  return n;
}

struct Expr *createLitExpr(struct Constant lit) {
  struct Expr *n = malloc(sizeof(struct Expr));
  n->op = Op_LIT;
  n->type = createScalarType(lit.type);
  n->next = NULL;
  n->lit = lit;
  return n;
}

struct Expr *createVarExpr(char *name) {
  struct Expr *n = malloc(sizeof(struct Expr));
  n->op = Op_VAR;
  n->type = NULL;
  n->next = NULL;
  n->name = name;
  return n;
}

struct Expr *createFuncExpr(char *name, struct Expr *args) {
  struct Expr *n = malloc(sizeof(struct Expr));
  n->op = Op_FUNC;
  n->type = NULL;
  n->next = NULL;
  n->args = createVarExpr(name);
  n->args->next = args;
  return n;
}

void destroyExpr(struct Expr *expr) {
  struct Expr *p = expr, *q;
  while (p != NULL) {
    if (p->op == Op_VAR) {
      free(p->name);
    }
    else if (p->op == Op_LIT) {
      destroyConst(p->lit);
    }
    else {
      destroyExpr(p->args);
    }
    if (p->type != NULL) free(p->type);
    q = p;
    p = p->next;
    free(q);
  }
}

char *OpName[] = {
  "NONE",
  "or", "and", "not",
  "<", "<=", "<>", ">=", ">", "=",
  "+", "-",
  "*", "/", "mod",
  "UMINUS", // -num
  "FUNC", // f(x)
  "INDEX", // a[i]
  "", // literal
  "VAR" // variable reference
};

void showExpr(struct Expr *expr, int depth) {
  int i;
  for (i = 0; i < depth; i++) {
    printf("  ");
  }
  printf("%s", OpName[expr->op]);
  if (expr->op == Op_VAR) {
    printf(" %s\n", expr->name);
  }
  else if (expr->op == Op_LIT) {
    showConst(expr->lit);
    puts("");
  }
  else {
    puts("");
    struct Expr *p = expr->args;
    while (p != NULL) {
      showExpr(p, depth + 1);
      p = p->next;
    }
  }
}

void initExprList(struct ExprList *list) {
  list->first = list->last = NULL;
}

void addToExprList(struct ExprList *list, struct Expr *expr) {
  if (list->first == NULL) {
    list->first = expr;
  }
  else {
    list->last->next = expr;
  }
  list->last = expr;
}

