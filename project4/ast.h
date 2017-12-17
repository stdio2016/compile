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

struct Type {
  enum TypeEnum {
    Type_INTEGER, Type_REAL, Type_BOOLEAN, Type_STRING, Type_ARRAY, Type_VOID
  } type;
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

// for program and function name
// name points to char array in symbol table, so don't release name
struct PairName {
  char *name;
  Bool success; // name in symbol table?
};

char *dupstr(char *str);

struct Type *copyType(struct Type *ptr);
void destroyType(struct Type *ptr);
int showType(struct Type *type); // returns chars printed

struct Constant copyConst(struct Constant c);
void destroyConst(struct Constant c);
void showConst(struct Constant c);
#endif
