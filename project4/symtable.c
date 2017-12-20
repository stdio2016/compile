#include <stdio.h>
#include <stdlib.h>
#include "MyHash.h"
#include "symtable.h"
#include "errReport.h"
extern int linenum; // from tokens.l

const char *SymbolKindName[] = {
  "program", "function", "parameter",
  "variable", "constant", "loop var"
};

struct MyHash table;

struct SymTableEntry **stack;
size_t stackSize, stackTop;
#define INIT_STACK_SIZE 5
size_t varDeclStart;

int curScopeLevel;

static inline void OutOfMemory() {
  fprintf(stderr, "Out of memory!\n");
  exit(EXIT_FAILURE);
}
// private!
void needResizeStack(void);
struct SymTableEntry *createSymEntry(char *name, enum SymbolKind kind);
void popSymbol(void);
void showScope(size_t stackstart);

void initSymTable(void) {
  MyHash_init(&table, MyHash_strcmp, MyHash_strhash);
  stackSize = INIT_STACK_SIZE;
  stackTop = 0;
  stack = malloc(sizeof(struct SymTableEntry *) * stackSize);
  if (stack == NULL) OutOfMemory();
  curScopeLevel = 0;
}

void destroySymTable(void) {
  while (stackTop > 0) {
    popSymbol();
  }
  stackSize = 0;
  free(stack);
  curScopeLevel = 0;
  int i;
  free(table._buckets);
}

void needResizeStack(void) {
  if (stackTop >= stackSize) {
    stackSize *= 2;
    struct SymTableEntry **newstack = realloc(stack, sizeof(struct SymTableEntry *) * stackSize);
    if (newstack == NULL) OutOfMemory();
    stack = newstack;
  }
}

struct SymTableEntry *createSymEntry(char *name, enum SymbolKind kind) {
  struct SymTableEntry *en = malloc(sizeof(struct SymTableEntry));
  if (NULL == en) OutOfMemory();
  en->name = name;
  en->kind = kind;
  en->level = curScopeLevel;
  en->type = NULL;
  en->attr.tag = Attribute_NONE;
  en->prev = NULL;
  return en;
}

struct PairName addSymbol(char *name, enum SymbolKind kind) {
  needResizeStack();
  struct SymTableEntry *en = createSymEntry(name, kind);
  struct SymTableEntry *old = MyHash_set(&table, name, en);
  struct PairName out;
  if (old != NULL) {
    if (old->level == curScopeLevel || old->kind == SymbolKind_loopVar) {
      semanticError("symbol " BOLD_TEXT "%s" NORMAL_TEXT " is redeclared\n", name);
      MyHash_set(&table, name, old);
      free(name);
      free(en);
      out.name = old->name;
      out.success = False;
      return out;
    }
    else { // symbol in lower level scope
      stack[stackTop] = en;
      stackTop++;
      en->prev = old;
    }
  }
  else {
    stack[stackTop] = en;
    stackTop++;
  }
  out.name = name;
  out.success = True;
  return out;
}

Bool addLoopVar(char *name) {
  // add a loop var symbol in higher level scope
  curScopeLevel++;
  Bool y = addSymbol(name, SymbolKind_loopVar).success;
  curScopeLevel--;
  if (y) {
    stack[stackTop-1]->type = createScalarType(Type_INTEGER);
  }
  return y;
}

void removeLoopVar(void) {
  if (stack[stackTop-1]->kind == SymbolKind_loopVar) {
    popSymbol();
  }
}

void popSymbol(void) {
  stackTop--;
  struct SymTableEntry *top = stack[stackTop];
  if (top->prev == NULL) { // no shadowed symbol
    struct HashBucket *b = MyHash_delete(&table, top->name);
    free(b);
  }
  else {
    MyHash_set(&table, top->name, top->prev);
  }
  free(top->name);
  destroyType(top->type);
  destroyAttribute(&top->attr);
  free(top);
  stack[stackTop] = NULL;
}

void startVarDecl(void) {
  varDeclStart = stackTop;
}

void startParamDecl(void) {
  varDeclStart = stackTop;
}

void pushScope(void) {
  curScopeLevel++;
}

void showScope(size_t stackstart) {
  int i;
  for (i = 0; i < 110; i++)
    printf("=");
  printf("\n");
  printf("%-33s%-11s%-11s%-17s%-11s\n","Name","Kind","Level","Type","Attribute");
  for (i = 0; i < 110; i++)
    printf("-");
  printf("\n");
  size_t j;
  for (j = stackstart; j < stackTop; j++) {
    if (stack[j]->kind == SymbolKind_loopVar) continue ;
    printf("%-33s", stack[j]->name);
    printf("%-11s", SymbolKindName[stack[j]->kind]);
    printf("%d%-10s", stack[j]->level, stack[j]->level == 0 ? "(global)" : "(local)");
    int n = showType(stack[j]->type), k;
    for (k = n; k < 17; k++) putchar(' '); // align
    showAttribute(stack[j]->attr);
    printf("\n");
  }
  for (i = 0; i < 110; i++)
    printf("-");
  printf("\n");
}

void popScope(int toShowScope) {
  curScopeLevel--;
  size_t i = stackTop;
  while (i > 0 && stack[i-1]->level > curScopeLevel
    && stack[i-1]->level != SymbolKind_loopVar) {
    i--;
  }
  size_t j;
  if (toShowScope) {
    showScope(i);
  }
  for (j = stackTop; j > i; j--) {
    popSymbol();
  }
}

static void endVarOrParamDecl(struct Type *type, enum SymbolKind kind) {
  size_t i, n = stackTop;
  if (isLegalType(type)) {
    for (i = varDeclStart; i < n; i++) {
      stack[i]->kind = kind;
      stack[i]->type = copyType(type);
    }
  }
  else {
    for (i = varDeclStart; i < n; i++) {
      if (type != NULL && type->type == Type_ARRAY) {
        semanticError("wrong dimension declaration for array %s\n", stack[i]->name);
      }
    }
    for (i = varDeclStart; i < n; i++)
      popSymbol();
  }
  destroyType(type);
}

void endVarDecl(struct Type *type) {
  endVarOrParamDecl(type, SymbolKind_variable);
}

void endConstDecl(struct Constant constant) {
  size_t i;
  struct Type constType;
  constType.type = constant.type;
  constType.upperBound = 0;
  constType.lowerBound = 0;
  constType.itemType = NULL;
  for (i = varDeclStart; i < stackTop; i++) {
    stack[i]->kind = SymbolKind_constant;
    stack[i]->type = copyType(&constType);
    stack[i]->attr.tag = Attribute_CONST;
    stack[i]->attr.constant = copyConst(constant);
  }
  destroyConst(constant);
}

void endParamDecl(struct Type *type) {
  endVarOrParamDecl(type, SymbolKind_parameter);
}

void endFuncDecl(struct Type *retType, Bool funcExists) {
  Bool isArray = retType->type == Type_ARRAY;
  if (isArray) {
    semanticError("a function cannot return an array type\n");
  }
  if (!funcExists) {
    free(retType);
    return ;
  }
  size_t i = stackTop;
  while (i > 0 && stack[i-1]->kind == SymbolKind_parameter) {
    i--;
  }
  if (isArray) {
    // remove function from symbol table
    struct SymTableEntry *t = stack[i];
    for (i = i + 1; i < stackTop; i++) {
      stack[i-1] = stack[i];
    }
    stack[stackTop-1] = t;
    popSymbol();
    free(retType);
    return ;
  }
  size_t nargs = stackTop - i;
  struct Type **argtype = malloc(sizeof(struct Type *) * nargs);
  size_t j;
  for (j = i; j < stackTop; j++) {
    struct Type *c = copyType(stack[j]->type);
    argtype[j-i] = c;
  }
  stack[i-1]->attr.tag = Attribute_ARGTYPE;
  stack[i-1]->attr.argType.arity = nargs;
  stack[i-1]->attr.argType.types = argtype;
  stack[i-1]->type = retType;
}

void destroyAttribute(struct Attribute *attr) {
  if (attr->tag == Attribute_ARGTYPE) {
    int i;
    for (i = 0; i < attr->argType.arity; i++) {
      destroyType(attr->argType.types[i]);
    }
    free(attr->argType.types);
  }
  else if (attr->tag == Attribute_CONST) {
    destroyConst(attr->constant);
  }
  attr->tag = Attribute_NONE;
}

void showAttribute(struct Attribute attr) {
  if (attr.tag == Attribute_ARGTYPE) {
    int i;
    for (i = 0; i < attr.argType.arity; i++) {
      if (i > 0) printf(", ");
      showType(attr.argType.types[i]);
    }
  }
  else if (attr.tag == Attribute_CONST) {
    showConst(attr.constant);
  }
}

struct SymTableEntry *getSymEntry(const char *name) {
  return MyHash_get(&table, name);
}
