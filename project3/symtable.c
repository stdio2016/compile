#include <stdio.h>
#include <stdlib.h>
#include "MyHash.h"
#include "symtable.h"
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
    if (old->level == curScopeLevel || old->kind == SymbolKind_loopVar) {
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

void addLoopVar(char *name) {
  // add a loop var symbol in higher level scope
  curScopeLevel++;
  addSymbol(name, SymbolKind_loopVar);
  curScopeLevel--;
}

void removeLoopVar(void) {
  popSymbol();
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

void endVarDecl(struct Type *type) {
  size_t i;
  for (i = varDeclStart; i < stackTop; i++) {
    stack[i]->kind = SymbolKind_variable;
    stack[i]->type = copyType(type);
  }
  destroyType(type, 1);
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
  size_t i;
  for (i = varDeclStart; i < stackTop; i++) {
    stack[i]->kind = SymbolKind_parameter;
    stack[i]->type = copyType(type);
  }
  destroyType(type, 1);
}

void endFuncDecl(struct Type *retType) {
  size_t i = stackTop;
  while (i > 0 && stack[i-1]->kind == SymbolKind_parameter) {
    i--;
  }
  size_t nargs = stackTop - i;
  struct Type *argtype = malloc(sizeof(struct Type) * nargs);
  size_t j;
  for (j = i; j < stackTop; j++) {
    struct Type *c = copyType(stack[j]->type);
    argtype[j-i] = *c;
    free(c);
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
      destroyType(&attr->argType.types[i], 0);
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
      showType(&attr.argType.types[i]);
    }
  }
  else if (attr.tag == Attribute_CONST) {
    showConst(attr.constant);
  }
}
