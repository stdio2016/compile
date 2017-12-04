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

void initSymTable(void) {
  MyHash_init(&table, MyHash_strcmp, MyHash_strhash);
  stackSize = INIT_STACK_SIZE;
  stackTop = 0;
  stack = malloc(sizeof(struct SymTableEntry *) * stackSize);
  if (stack == NULL) OutOfMemory();
  curScopeLevel = 0;
}

void addSymbol(char *name, enum SymbolKind kind) {
  if (stackTop >= stackSize) {
    stackSize *= 2;
    struct SymTableEntry **newstack = realloc(stack, sizeof(struct SymTableEntry *) * stackSize);
    if (newstack == NULL) OutOfMemory();
    stack = newstack;
  }
  stack[stackTop] = malloc(sizeof(struct SymTableEntry));
  if (NULL == stack[stackTop]) OutOfMemory();
  stack[stackTop]->name = name;
  stack[stackTop]->level = curScopeLevel;
  struct SymTableEntry *old = MyHash_set(&table, name, stack[stackTop]);
  if (old != NULL && old->level == curScopeLevel) {
    printf("<Error> found in Line %d: symbol %s is redeclared\n", linenum, name);
    MyHash_set(&table, name, old);
    free(name);
    free(stack[stackTop]);
  }
  else {
    stackTop++;
  }
}

void popSymbol(void) {
  stackTop--;
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
  for (j = i; j < stackTop; j++) {
    popSymbol();
  }
}

void endParamDecl(struct Type *type) {

}

void endFuncDecl(struct Type *retType) {

}
