#include <stdio.h>
#include <stdlib.h>
#include "semcheck.h"
#include "errReport.h"

void returnCheck(struct Expr *expr, struct Type *expected) {
  if (expected == NULL) {
    semanticError("program cannot be returned\n");
  }
  else if (expected->type == Type_VOID) {
    semanticError("procedure cannot be returned\n");
  }
  else if (expr->type == NULL) {
    ; // don't show error
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
    semanticError("loop parameter is less than zero\n");
  if (lowerBound > upperBound)
    semanticError("loop parameter is not in the incremental order\n");
}

void conditionCheck(struct Expr *expr, const char *ifwhile) {
  if (expr->type == NULL) {
    ; // don't show error
  }
  else if (expr->type->type != Type_BOOLEAN) {
    semanticError("operand of %s statement is not boolean type\n", ifwhile);
    return;
  }
}
