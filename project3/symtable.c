#include <stdio.h>
#include <stdlib.h>
#include "MyHash.h"
#include "symtable.h"
extern int linenum; // from tokens.l

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

void addSymbol(char *name, enum SymbolKind kind) {
  needResizeStack();
  struct SymTableEntry *en = createSymEntry(name, kind);
  struct SymTableEntry *old = MyHash_set(&table, name, en);
  if (old != NULL) {
    if (old->level == curScopeLevel) {
      printf("<Error> found in Line %d: symbol %s is redeclared\n", linenum, name);
      MyHash_set(&table, name, old);
      free(name);
      free(en);
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
  destroyType(top->type, 1);
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
  size_t j;
  for (j = stackstart; j < stackTop; j++) {
    printf("%s\n", stack[j]->name);
  }
}

void popScope(int toShowScope) {
  curScopeLevel--;
  size_t i = stackTop;
  while (i > 0 && stack[i-1]->level > curScopeLevel) {
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

void endParamDecl(struct Type *type) {

}

void endFuncDecl(struct Type *retType) {

}

void destroyAttribute(struct Attribute *attr) {
  if (attr->tag == Attribute_ARGTYPE) {
    int i;
    for (i = 0; i < attr->argType.arity; i++) {
      destroyType(&attr->argType.types[i], 0);
    }
    free(attr->argType.types);
  }
  else if (attr->tag == Attribute_CONST) {
    destroyConst(attr->constant);
  }
  attr->tag = Attribute_NONE;
}
