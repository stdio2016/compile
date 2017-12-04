#pragma once
#ifndef AST_INCLUDED
#define AST_INCLUDED

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
    int boolean;
  };
};

char *dupstr(char *str);

struct Type *copyType(struct Type *ptr);
void destroyType(struct Type *ptr, int includeSelf);
int showType(struct Type *type); // returns chars printed

struct Constant copyConst(struct Constant c);
void destroyConst(struct Constant c);
void showConst(struct Constant c);
#endif
