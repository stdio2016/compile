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

size_t curScopeLevel;

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
  printf("OK\n");
  struct SymTableEntry *old = MyHash_set(&table, name, stack[stackTop]);
  if (old != NULL && old->level == curScopeLevel) {
    printf("<Error> found in Line %d: symbol %s is redeclared\n", linenum, name);
    MyHash_set(&table, name, old);
    free(name);
  }
  else {
    printf("<Info> defined %s\n", name);
    stackTop++;
  }
}

void startVarDecl(void) {
  varDeclStart = stackTop;
}
