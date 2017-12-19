#include <stdio.h>
#include <stdlib.h>
#include "semcheck.h"
#include "errReport.h"

void returnCheck(struct Expr *expr, struct Type *expected) {
  if (expected == NULL) {
    semanticError("program cannot be returned\n");
  }
  else if (expected->type == Type_VOID) {
    semanticError("void function cannot be returned\n");
  }
  else if (expr->type == NULL) {
    semanticError("return type is unknown, but '");
    showType(expected);
    printf("' is expected\n");
  }
  else if (!typeEqual(expr->type, expected)) {
    semanticError("return type mismatch, return type is '");
    showType(expr->type);
    printf("' but '");
    showType(expected);
    printf("' is expected\n");
    return;
  }
}

void forCheck(int lowerBound, int upperBound) {
  if (lowerBound < 0 || upperBound < 0)
    semanticError("loop parameter must be greater than or equal to zero\n");
  if (lowerBound > upperBound)
    semanticError("loop parameter must be in the incremental order\n");
}

void conditionCheck(struct Expr *expr, const char *ifwhile) {
  if (expr->type == NULL || expr->type->type != Type_BOOLEAN) {
    semanticError("operand of %s statement is not boolean type\n", ifwhile);
    return;
  }
}
