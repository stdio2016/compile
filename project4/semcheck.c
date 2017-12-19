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
