#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "StringBuffer.h"

char *asmName;
FILE *codeOut;

// generator state
static struct StringBuffer *codeArray;
static int codeArraySize, codeArrayCap, labelCount;
int stackLimit, virtStackPos;
static Bool inFunction = False;

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
  codeArrayCap = 5;
  codeArraySize = 1;
  codeArray = malloc(sizeof(struct StringBuffer) * codeArrayCap);
  StrBuf_init(&codeArray[0]);
  inFunction = False;
}

void endCodeGen() {
  free(codeArray);
  fclose(codeOut);
  free(asmName);
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

int genLabel() {
  labelCount++;
  if (labelCount >= codeArraySize) {
    if (labelCount >= codeArrayCap) {
      codeArray = realloc(codeArray, codeArrayCap*2);
      if (codeArray == NULL) exit(-1); // out of memory!
      codeArrayCap *= 2;
    }
    StrBuf_init(&codeArray[labelCount]);
    codeArraySize++;
  }
  else {
    StrBuf_clear(&codeArray[labelCount]);
  }
  genIntCode(labelCount);
  genCode(":\n",0,0);
  return labelCount;
}

void genCode(const char *code, int useStack, int stackUpDown) {
  if (inFunction) {
    StrBuf_append(&codeArray[labelCount], code);
    if (virtStackPos + useStack > stackLimit) {
      stackLimit = virtStackPos + useStack;
    }
    virtStackPos += stackUpDown;
  }
  else {
    fprintf(codeOut, "%s", code);
  }
}

void genIntCode(int num) {
  char buf[35];
  sprintf(buf, "%d", num);
  genCode(buf, 0, 0);
}

void genFunctionStart() {
  labelCount = 0;
  stackLimit = 0;
  virtStackPos = 0;
  inFunction = True;
}

void genFunctionEnd() {
  inFunction = False;
}
