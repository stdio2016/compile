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

#endif
