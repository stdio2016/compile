#pragma once
#ifndef SEMCHECK_INCLUDED
#define SEMCHECK_INCLUDED

#include "symtable.h"

// check if expression is exactly *expected type
void returnCheck(struct Expr *expr, struct Type *expected);

// check for loop parameter
void forCheck(int lowerBound, int upperBound);

// check condition expr part of conditional (if...else) and while statement
void conditionCheck(struct Expr *expr, const char *ifwhile);

// check for assignment
void assignCheck(struct Expr *var, struct Expr *expr);

// check variable type
void varTypeCheck(struct Expr *var);

// check array type
void arrayTypeCheck(struct Expr *arr);

// check multi dimension array subscript
void mdArrayIndexCheck(struct Expr *arr);

// check print and read statement
void printCheck(struct Expr *expr);
void readCheck(struct Expr *expr);

#endif
