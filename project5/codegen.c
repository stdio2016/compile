#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "StringBuffer.h"
#include "symtable.h"

char *asmName;
FILE *codeOut;
extern char *progClassName; // defined in parser.y

// generator state
static struct StringBuffer *codeArray;
static int codeArraySize, codeArrayCap, labelCount;
int stackLimit, virtStackPos;
static Bool inFunction = False;
extern size_t localVarCount, localVarLimit; // defined in symtable.c

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

void genProgStart() {
  fprintf(codeOut, ".class public %s\n", progClassName);
  fprintf(codeOut, ".super java/lang/Object\n");
  fprintf(codeOut, "\n");
  fprintf(codeOut, ".field public static _sc Ljava/util/Scanner;\n");
}

void endCodeGen() {
  int i;
  for (i = 0; i < codeArraySize; i++) {
    StrBuf_destroy(&codeArray[i]);
  }
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
  int lbl = genInsertPoint();
  genCode("Label",0,0);
  genIntCode(lbl+1);
  genCode(":\n",0,0);
  return lbl;
}

int genInsertPoint() {
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

void genCodeAt(const char *code, int addr) {
  StrBuf_append(&codeArray[addr], code);
}

void genIntCode(int num) {
  char buf[35];
  sprintf(buf, "%d", num);
  genCode(buf, 0, 0);
}

void genTypeCode(struct Type *type) {
  if (type == NULL) return ;
  while (type->type == Type_ARRAY) {
    genCode("[",0,0);
    type = type->itemType;
  }
  switch (type->type) {
    case Type_INTEGER: genCode("I",0,0); break;
    case Type_REAL: genCode("F",0,0); break;
    case Type_BOOLEAN: genCode("Z",0,0); break;
    case Type_STRING: genCode("Ljava/lang/String;",0,0); break;
    case Type_VOID: genCode("V",0,0); break;
  }
}

void genFuncTypeCode(const char *funname) {
  genCode(funname, 0, 0);
  struct SymTableEntry *e = getSymEntry(funname);
  if (e == NULL || e->kind != SymbolKind_function) return;
  genCode("(", 0, 0);
  int n = e->attr.argType.arity, i;
  for (i = 0; i < n; i++) {
    genTypeCode(e->attr.argType.types[i]);
  }
  genCode(")", 0, 0);
  genTypeCode(e->type);
}

void genFunctionStart(const char *funname) {
  labelCount = 0;
  stackLimit = 0;
  virtStackPos = 0;
  fprintf(codeOut, ".method public static ");
  genFuncTypeCode(funname);
  fputs("\n", codeOut);
  inFunction = True;
  StrBuf_clear(&codeArray[0]);
}

void genConstCode(struct Constant val) {
  if (val.type == Type_BOOLEAN) {
    genCode(val.boolean ? "  iconst_1\n" : "  iconst_0\n", 1, +1);
  }
  else if (val.type == Type_STRING) {
    genCode("  ldc \"",0,0);
    size_t i;
    for (i = 0; val.str[i]; i++) {
      if (val.str[i] == '\\') {
        if (val.str[i+1] == 'n') {
          genCode("\\n",0,0);
          i++;
        }
        else genCode("\\\\",0,0);
      }
      else if (val.str[i] == '"') genCode("\\\"",0,0);
      else {
        char a[4] = {0,0,0,0};
        a[0] = val.str[i];
        genCode(a, 0, 0);
      }
    }
    genCode("\"\n",1,+1);
  }
  else if (val.type == Type_INTEGER) {
    if (val.integer == -1) genCode("  iconst_m1\n",1,+1);
    else {
      if (val.integer >= 0 && val.integer <= 5)
        genCode("  iconst_",0,0);
      else if (val.integer >= -128 && val.integer <= 127)
        genCode("  bipush ",0,0);
      else if (val.integer >= -32768 && val.integer <= 32767)
        genCode("  sipush ",0,0);
      else genCode("  ldc ",0,0);
      genIntCode(val.integer);
      genCode("\n",1,+1);
    }
  }
  else if (val.type == Type_REAL) {
    char buf[100];
    if (val.real == 1.0/0.0) strcpy(buf, "  ldc 1.0e+100");
    else if (val.real == -1.0/0.0) strcpy(buf, "  ldc -1.0e+100");
    else if (val.real != val.real) strcpy(buf, "  getstatic java/lang/Float/NaN F");
    else if (val.real == 2.0) strcpy(buf, "  fconst_2");
    else if (val.real == 1.0) strcpy(buf, "  fconst_1");
    else if (val.real == 0.0) strcpy(buf, "  fconst_0");
    else {
      sprintf(buf, "  ldc %.7g", val.real);
      if (strchr(buf, '.') == NULL) {
        strcat(buf, ".0");
      }
    }
    strcat(buf, "\n");
    genCode(buf,1,+1);
  }
}

void genProgMain() {
  labelCount = 0;
  stackLimit = 0;
  virtStackPos = 0;
  fputs(".method public static main([Ljava/lang/String;)V\n", codeOut);
  inFunction = True;
  StrBuf_clear(&codeArray[0]);
  localVarCount = localVarLimit = 1; // because the first temp var is argument of main
  genCode("  new java/util/Scanner\n", 1, +1);
  genCode("  dup\n", 1, +1);
  genCode("  getstatic java/lang/System/in Ljava/io/InputStream;\n", 1, +1);
  genCode("  invokespecial java/util/Scanner/<init>(Ljava/io/InputStream;)V\n", 0, -2);
  genCode("  putstatic ", 0, 0);
  genCode(progClassName, 0, 0);
  genCode("/_sc Ljava/util/Scanner;\n", 0, -1);
  //TODO generate global var initialize
}

void genFunctionEnd() {
  inFunction = False;
  int i;
  fprintf(codeOut, ".limit stack %d\n", stackLimit);
  fprintf(codeOut, ".limit locals %ld\n", localVarLimit);
  fputs(codeArray[0].buf, codeOut);
  for (i = 1; i <= labelCount; i++) {
    fputs(codeArray[i].buf, codeOut);
  }
  fputs(".end method\n", codeOut);
}

struct PatchList *makePatchList(int addr) {
  struct PatchList *lis = malloc(sizeof *lis);
  lis->addr = addr;
  lis->next = lis->prev = lis;
  return lis;
}

void destroyPatchList(struct PatchList *list) {
  if (list == NULL) return;
  struct PatchList *p = list;
  while (p != list) {
    struct PatchList *q = p->next;
    free(p);
    p = q;
  }
}

void backpatch(struct PatchList *list, int label) {
  // TODO
}

struct PatchList *mergePatchList(struct PatchList *p1, struct PatchList *p2) {
  if (p1 == NULL) return p2;
  if (p2 == NULL) return p1;
  struct PatchList *p1Last = p1->prev;
  p1Last->next = p2;
  p1->prev = p2->prev;
  p2->prev->next = p1;
  p2->prev = p1Last;
  return p1;
}
