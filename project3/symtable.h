#pragma once
#ifndef SYMTABLE_INCLUDED
#define SYMTABLE_INCLUDED
#include "ast.h"

enum SymbolKind {
  SymbolKind_program, SymbolKind_function, SymbolKind_parameter,
  SymbolKind_variable, SymbolKind_constant, SymbolKind_loopVar
};

extern const char *SymbolKindName[];

struct Attribute {
  enum {
    Attribute_ARGTYPE, Attribute_CONST, Attribute_NONE
  } tag;
  union {
    struct Attribute_ArgType {
      int arity;
      struct Type *types;
    } argType;
    struct Constant constant;
  };
};

struct SymTableEntry {
  char *name;
  enum SymbolKind kind;
  int level;
  struct Type *type;
  struct Attribute attr;
  struct SymTableEntry *prev;
};

void initSymTable(void);

void startVarDecl(void);
void startParamDecl(void);

// name is not copied
int addSymbol(char *name, enum SymbolKind kind);
int addLoopVar(char *name);
void removeLoopVar(void);

void endVarDecl(struct Type *type);
void endConstDecl(struct Constant constant);
void endParamDecl(struct Type *type);
void endFuncDecl(struct Type *returnType, int funcExists);

void pushScope(void);
void popScope(int toShowScope);

void destroyAttribute(struct Attribute *attr);
void showAttribute(struct Attribute attr);
#endif
