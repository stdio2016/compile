#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "StringBuffer.h"
#include "symtable.h"

char *asmName;
FILE *codeOut;
extern char *progClassName; // defined in parser.y

struct LabeledStringBuffer {
  struct StringBuffer buf;
  Bool hasLabel;
};

// generator state
static struct LabeledStringBuffer *codeArray;
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
  codeArray = malloc(sizeof(*codeArray) * codeArrayCap);
  codeArray[0].hasLabel = False;
  StrBuf_init(&codeArray[0].buf);
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
    StrBuf_destroy(&codeArray[i].buf);
  }
  free(codeArray);
  fclose(codeOut);
  free(asmName);
}

struct BoolExpr createBoolExpr(enum Operator op, struct BoolExpr op1, struct BoolExpr op2) {
  struct BoolExpr b;
  b.isTFlist = False;
  b.expr = createExpr(op, op1.expr, op2.expr);
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
  genCode("  ifne ",0,-1);
  expr->truelist = makePatchList(genInsertPoint());
  genCode("  goto ",0,0);
  expr->falselist = makePatchList(genLabel());
}

void BoolExpr_toImmed(struct BoolExpr *expr) {
  expr->isTFlist = False;
  int i = genLabel();
  genCode("  iconst_1\n",1,+1);
  genCode("  goto ",0,0);
  backpatch(expr->truelist, i);
  int j = genLabel();
  struct PatchList *p = makePatchList(j);
  genCode("  iconst_0\n",1,0);
  int k = genLabel();
  backpatch(expr->falselist, j);
  backpatch(p, k);
}

int genLabel() {
  int lbl = genInsertPoint();
  return lbl;
}

int genInsertPoint() {
  labelCount++;
  if (labelCount >= codeArraySize) {
    if (labelCount >= codeArrayCap) {
      codeArray = realloc(codeArray, sizeof(codeArray[0]) * codeArrayCap*2);
      if (codeArray == NULL) exit(-1); // out of memory!
      codeArrayCap *= 2;
    }
    StrBuf_init(&codeArray[labelCount].buf);
    codeArraySize++;
  }
  else {
    StrBuf_clear(&codeArray[labelCount].buf);
  }
  codeArray[labelCount].hasLabel = False;
  return labelCount-1;
}

void genCode(const char *code, int useStack, int stackUpDown) {
  if (inFunction) {
    StrBuf_append(&codeArray[labelCount].buf, code);
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
  StrBuf_append(&codeArray[addr].buf, code);
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

void genFuncTypeCode(struct SymTableEntry *e, const char *funname) {
  // don't generate "main" for function main
  genCode(strcmp(funname, "main") ? funname : "_main", 0, 0);
  if (e == NULL || e->kind != SymbolKind_function) return;
  genCode("(", 0, 0);
  int n = e->attr.argType.arity, i;
  for (i = 0; i < n; i++) {
    genTypeCode(e->attr.argType.types[i]);
  }
  genCode(")", 0, 0);
  genTypeCode(e->type);
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

void genFunctionStart(const char *funname) {
  labelCount = 0;
  stackLimit = 0;
  virtStackPos = 0;
  fprintf(codeOut, ".method public static ");
  genFuncTypeCode(getFunctionEntry(funname), funname);
  fputs("\n", codeOut);
  inFunction = True;
  StrBuf_clear(&codeArray[0].buf);
  codeArray[0].hasLabel = False;
}

void genProgMain() {
  labelCount = 0;
  stackLimit = 0;
  virtStackPos = 0;
  fputs(".method public static main([Ljava/lang/String;)V\n", codeOut);
  inFunction = True;
  StrBuf_clear(&codeArray[0].buf);
  codeArray[0].hasLabel = False;
  localVarCount = localVarLimit = 1; // because the first temp var is argument of main
  genCode("  new java/util/Scanner\n", 1, +1);
  genCode("  dup\n", 1, +1);
  genCode("  getstatic java/lang/System/in Ljava/io/InputStream;\n", 1, +1);
  genCode("  invokespecial java/util/Scanner/<init>(Ljava/io/InputStream;)V\n", 0, -2);
  genCode("  putstatic ", 0, 0);
  genCode(progClassName, 0, 0);
  genCode("/_sc Ljava/util/Scanner;\n", 0, -1);
  int i;
  extern int stackTop; // defined in symtable.c
  extern struct SymTableEntry **stack;
  for (i = 0; i < stackTop; i++) {
    if (stack[i]->kind == SymbolKind_variable)
      genGlobalVarInit(stack[i]->name, stack[i]->type);
  }
}

void genFunctionEnd() {
  inFunction = False;
  int i;
  fprintf(codeOut, ".limit stack %d\n", stackLimit);
  fprintf(codeOut, ".limit locals %ld\n", localVarLimit);
  fputs(codeArray[0].buf.buf, codeOut);
  for (i = 1; i <= labelCount; i++) {
    if (codeArray[i].hasLabel) fprintf(codeOut, "Label%d:\n",i);
    fputs(codeArray[i].buf.buf, codeOut);
  }
  fputs(".end method\n\n", codeOut);
}

void genFunctionCall(struct SymTableEntry *fun, const char *funname, struct PatchList *p, struct Expr *args) {
  if (fun != NULL) {
    int i, n = fun->attr.argType.arity;
    struct Expr *e = args;
    for (i = 0; i < n; i++) {
      if (e == NULL) break;
      if (fun->attr.argType.types[i]->type == Type_REAL
        && e->type != NULL && e->type->type == Type_INTEGER) {
        if (e->next == NULL)
          genCode("  i2f\n",0,0);
        else
          genCodeAt("  i2f\n",p->addr);
      }
      e = e->next;
      if (p != NULL)
        p = p->next;
    }
  }
  int argn = 0;
  struct Expr *e = args;
  while (e != NULL) {
    argn++;
    e = e->next;
  }
  genCode("  invokestatic ", 0, -argn);
  genCode(progClassName, 0, 0);
  genCode("/",0,0);
  genFuncTypeCode(fun, funname);
  int n = fun != NULL && fun->kind == SymbolKind_function && fun->type->type != Type_VOID;
  genCode("\n", n, +n);
}

void genLoadLocalVar(int tmpVarId, enum TypeEnum type) {
  switch (type) {
    case Type_BOOLEAN: case Type_INTEGER:
      genCode("  iload",1,+1); break;
    case Type_REAL:
      genCode("  fload",1,+1); break;
    default:
      genCode("  aload",1,+1); break;
  }
  if (tmpVarId < 4) { // short version!
    genCode("_",0,0);
  }
  else genCode(" ",0,0); // long version
  genIntCode(tmpVarId);
  genCode("\n",0,0);
}

void genStoreLocalVar(int tmpVarId, enum TypeEnum type) {
  switch (type) {
    case Type_BOOLEAN: case Type_INTEGER:
      genCode("  istore",0,-1); break;
    case Type_REAL:
      genCode("  fstore",0,-1); break;
    default:
      genCode("  astore",0,-1); break;
  }
  if (tmpVarId < 4) { // short version!
    genCode("_",0,0);
  }
  else genCode(" ",0,0); // long version
  genIntCode(tmpVarId);
  genCode("\n",0,0);
}

void genLoadVar(const char *varname) {
  struct SymTableEntry *e = getSymEntry(varname);
  if (e == NULL) {
    genCode("  iconst_0 ; unknown variable ",1,+1);
    genCode(varname,0,0);
    genCode("\n",0,0);
    return;
  }
  if (e->kind == SymbolKind_constant) { // constant
    genConstCode(e->attr.constant);
  }
  else if (e->level == 0) { // global var
    genCode("  getstatic ",1,+1);
    genCode(progClassName,0,0);
    genCode("/",0,0);
    genCode(varname,0,0);
    genCode(" ",0,0);
    genTypeCode(e->type);
    genCode("\n",0,0);
  }
  else { // local var
    genLoadLocalVar(e->attr.tmpVarId, e->type->type);
  }
}

void genStoreVar(const char *varname) {
  struct SymTableEntry *e = getSymEntry(varname);
  if (e == NULL) {
    genCode("  pop ; unknown variable ",0,-1);
    genCode(varname,0,0);
    genCode("\n",0,0);
    return;
  }
  if (e->kind == SymbolKind_constant) { // constant
    genCode("  pop ; cannot store into constant ",0,-1);
    genCode(varname,0,0);
    genCode("\n",0,0);
    return;
  }
  else if (e->level == 0) { // global var
    genCode("  putstatic ",0,-1);
    genCode(progClassName,0,0);
    genCode("/",0,0);
    genCode(varname,0,0);
    genCode(" ",0,0);
    genTypeCode(e->type);
    genCode("\n",0,0);
  }
  else { // local var
    genStoreLocalVar(e->attr.tmpVarId, e->type->type);
  }
}

void genLoadArray(struct Expr *expr) {
  if (expr->type == NULL) {
    genCode("  iaload ; unknown item type\n",0,-1);
    return;
  }
  switch (expr->type->type) {
    case Type_BOOLEAN:
      genCode("  baload\n",0,-1); break;
    case Type_INTEGER:
      genCode("  iaload\n",0,-1); break;
    case Type_REAL:
      genCode("  faload\n",0,-1); break;
    default:
      genCode("  aaload\n",0,-1); break;
  }
}

void genStoreArray(struct Expr *expr) {
  if (expr->type == NULL) {
    genCode("  iastore ; unknown item type\n",0,-3);
    return;
  }
  switch (expr->type->type) {
    case Type_BOOLEAN:
      genCode("  bastore\n",0,-3); break;
    case Type_INTEGER:
      genCode("  iastore\n",0,-3); break;
    case Type_REAL:
      genCode("  fastore\n",0,-3); break;
    default:
      genCode("  aastore\n",0,-3); break;
  }
}

void genArrayIndexShift(int offset) {
  if (offset != 0) {
    struct Constant c;
    c.type = Type_INTEGER;
    c.integer = offset;
    genConstCode(c);
    genCode("  isub\n",0,-1);
  }
}

void genGlobalVarCode(const char *name, struct Type *type) {
  fprintf(codeOut, ".field public static %s ", name);
  genTypeCode(type);
  fputs("\n", codeOut);
}

void genGlobalVarInit(const char *name, struct Type *type) {
  if (type->type == Type_STRING) {
    genCode("  ldc \"\"\n",1,+1);
    genCode("  putstatic ",0,-1);
    genCode(progClassName,0,0);
    genCode("/",0,0);
    genCode(name,0,0);
    genCode(" Ljava/lang/String;\n",0,0);
  }
  else if (type->type == Type_ARRAY) {
    //TODO genCreateArray(type);
  }
  // integer, real and boolean doesn't need init
}

void genLocalVarInit(int tmpVarId, struct Type *type) {
  if (type->type == Type_INTEGER || type->type == Type_BOOLEAN) genCode("  iconst_0\n",1,+1);
  else if (type->type == Type_REAL) genCode("  fconst_0\n",1,+1);
  else if (type->type == Type_STRING) genCode("  ldc \"\"\n",1,+1);
  else if (type->type == Type_ARRAY) {
    //TODO genCreateArray(type);
  }
  genStoreLocalVar(tmpVarId, type->type);
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
  do {
    struct PatchList *q = p->next;
    free(p);
    p = q;
  } while (p != list);
}

void backpatch(struct PatchList *list, int label) {
  if (list == NULL) return;
  struct PatchList *p = list;
  do {
    char buf[25];
    sprintf(buf, "Label%d\n", label+1);
    genCodeAt(buf, p->addr);
    struct PatchList *q = p->next;
    free(p);
    p = q;
  } while (p != list);
  codeArray[label+1].hasLabel = True;
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
