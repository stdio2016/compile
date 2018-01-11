#pragma once
#ifndef AST_INCLUDED
#define AST_INCLUDED

// I like boolean, but I don't know whether I can use C99 or not
#if __STDC_VERSION__ >= 199901L
  #include <stdbool.h>
  #define True true
  #define False false
  typedef bool Bool;
#else
  #define True 1
  #define False 0
  typedef int Bool;
#endif

enum Operator {
  Op_NONE,
  Op_OR, Op_AND, Op_NOT,
  Op_LESS, Op_LEQUAL, Op_NOTEQUAL, Op_GEQUAL, Op_GREATER, Op_EQUAL,
  Op_PLUS, Op_MINUS,
  Op_MULTIPLY, Op_DIVIDE, Op_MOD,
  Op_UMINUS, // -num
  Op_FUNC, // f(x)
  Op_INDEX, // a[i]
  Op_LIT, // literal
  Op_VAR // variable reference
};

enum TypeEnum {
  Type_INTEGER, Type_REAL, Type_BOOLEAN, Type_STRING, Type_ARRAY, Type_VOID
};

extern char *OpName[];

struct Type {
  enum TypeEnum type;
  // only for array
  int upperBound, lowerBound;
  struct Type *itemType;
};

struct Constant {
  enum TypeEnum type;
  union {
    char *str;
    int integer;
    float real;
    Bool boolean;
  };
};

struct ExprList {
  struct Expr *first;
  struct Expr *last;
  struct PatchList *marks;
};

struct Expr {
  enum Operator op;
  struct Type *type;
  union {
    struct Constant lit;
    char *name; // variable name
    struct Expr *args;
  };
  struct Expr *next;
};

// for program and function name
// name points to char array in symbol table, so don't release name
struct PairName {
  char *name;
  Bool success; // name in symbol table?
};

// for code generation
struct PatchList {
  int addr;
  struct PatchList *next, *prev;
};

struct BoolExpr {
  Bool isTFlist;
  struct Expr *expr;
  struct PatchList *truelist, *falselist;
};

struct Statement {
  struct PatchList *nextlist;
};

char *dupstr(const char *str);

struct Type *copyType(struct Type *ptr);
void destroyType(struct Type *ptr);
int showType(struct Type *type); // returns chars printed
struct Type *createScalarType(enum TypeEnum type);
Bool isSameType(struct Type *t1, struct Type *t2);
Bool canConvertTypeImplicitly(struct Type *from, struct Type *to);
// maybe my parser just created a malformed Type
Bool isLegalType(struct Type *type);
Bool isScalarType(struct Type *type);

struct Constant copyConst(struct Constant c);
void destroyConst(struct Constant c);
void showConst(struct Constant c);

struct Expr *createExpr(enum Operator op, struct Expr *op1, struct Expr *op2);
struct Expr *createLitExpr(struct Constant lit);
struct Expr *createVarExpr(char *name);
struct Expr *createFuncExpr(char *name, struct Expr *args);
void destroyExpr(struct Expr *expr);
void showExpr(struct Expr *expr, int depth);

void initExprList(struct ExprList *list);
void addToExprList(struct ExprList *list, struct Expr *expr);

#endif
