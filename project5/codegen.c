#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "StringBuffer.h"

char *asmName;
FILE *codeOut;
void initCodeGen(const char *filename) {
  size_t n = strlen(filename), i, dot = n;
  asmName = malloc(n+3); // filename '.' 'j' '\0'
  for (i = 0; filename[i]; i++) {
    if (filename[i] == '.') dot = i;
    if (filename[i] == '/') dot = n;
  }
  if (strcmp(&filename[dot], ".j") == 0) { // input file is .j
    dot += 2;
  }
  strcpy(asmName, filename);
  strcpy(asmName+dot, ".j");
  codeOut = fopen(asmName, "w");
}

struct BoolExpr createBoolExpr(enum Operator op, struct BoolExpr op1, struct BoolExpr op2) {
  struct BoolExpr b;
  b.isTFlist = False;
  b.expr = createExpr(op, BoolExprToExpr(op1), BoolExprToExpr(op2));
  return b;
}

struct Expr *BoolExprToExpr(struct BoolExpr expr) {
  if (expr.isTFlist) {
    BoolExpr_toImmed(&expr);
  }
  return expr.expr;
}

struct BoolExpr ExprToBoolExpr(struct Expr *expr) {
  struct BoolExpr b;
  b.isTFlist = False;
  b.expr = expr;
  return b;
}

// BoolExpr has two types: immed and tflist
void BoolExpr_toTFlist(struct BoolExpr *expr) {
}

void BoolExpr_toImmed(struct BoolExpr *expr) {
}

void genCode(const char *code, int useStack, int stackUpDown) {
  fprintf(codeOut, "%s", code);
}
