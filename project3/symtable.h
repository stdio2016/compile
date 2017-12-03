#pragma once
#ifndef SYMTABLE_INCLUDED
#define SYMTABLE_INCLUDED

enum SymbolKind {
  SymbolKind_program, SymbolKind_function, SymbolKind_parameter,
  SymbolKind_variable, SymbolKind_constant
};

struct Type {
  enum {
    Type_INTEGER, Type_REAL, Type_BOOLEAN, Type_STRING, Type_ARRAY, Type_VOID
  } type;
  int dimension;
  int *sizes;
};

struct Attribute {
  enum {
    Attribute_ARGTYPE, Attribute_CONST, Attribute_NONE
  } tag;
  union {
    struct Attribute_ArgType {
      int arity;
      struct Type *types;
    } argType;
    char *constant;
  };
};

struct SymTableEntry {
  char *name;
  enum SymbolKind kind;
  int level;
  struct Type type;
  struct Attribute attr;
  struct SymTableEntry *prev;
};

void initSymTable(void);

void startVarDecl(void);
void startParamDecl(void);

// name is not copied
void addSymbol(char *name, enum SymbolKind kind);

void endVarDecl(enum SymbolKind kind, struct Type *type, char *constant);
void endParamDecl(struct Type *type);
void endFuncDecl(struct Type *returnType);
#endif
