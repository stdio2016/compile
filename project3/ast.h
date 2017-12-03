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

struct Constant *createConst(enum TypeEnum type);

char *dupstr(char *str);

#endif
