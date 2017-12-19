#pragma once
#ifndef SEMCHECK_INCLUDED
#define SEMCHECK_INCLUDED

#include "symtable.h"

// check if expression is exactly *expected type
void returnCheck(struct Expr *expr, struct Type *expected);

#endif
