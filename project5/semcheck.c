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
  else if (!isSameType(expr->type, expected)) {
    semanticError("return type mismatch, return type is '");
    showType(expr->type);
    printf("' but '");
    showType(expected);
    printf("' is expected\n");
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
  }
}

void assignCheck(struct Expr *var, struct Expr *expr) {
  if (var->type == NULL || expr->type == NULL) { // variable error
    return ;
  }
  if (canConvertTypeImplicitly(expr->type, var->type)) {
    if (var->type->type == Type_ARRAY) {
      semanticError("array assignment is not allowed\n");
    }
    else if (var->op == Op_VAR) {
      struct SymTableEntry *a = getSymEntry(var->name);
      if (a == NULL) ;
      else if (a->kind == SymbolKind_loopVar) {
        semanticError("loop variable '%s' cannot be assigned\n", var->name);
      }
      else if (a->kind == SymbolKind_constant) {
        semanticError("constant '%s' cannot be assigned\n", var->name);
      }
    }
  }
  else {
    semanticError("type mismatch, LHS = ");
    showType(var->type);
    printf(", RHS = ");
    showType(expr->type);
    puts("");
  }
}

static struct SymTableEntry *readableVarCheck(const char *name, const char *vararr) {
  struct SymTableEntry *a = getSymEntry(name);
  if (a == NULL) {
    semanticError("symbol '%s' is not defined\n", name);
    return NULL;
  }
  if (a->kind == SymbolKind_program || a->kind == SymbolKind_function) {
    semanticError("symbol '%s' is not %s\n", name, vararr);
    return NULL;
  }
  return a;
}

void varTypeCheck(struct Expr *var) {
  struct SymTableEntry *a = readableVarCheck(var->name, "a variable");
  if (a == NULL) return ; // error already reported
  var->type = copyType(a->type);
}

void arrayTypeCheck(struct Expr *arr) {
  struct Expr *var = arr->args, *idx = var->next;
  struct SymTableEntry *a = readableVarCheck(var->name, "an array");
  Bool lhsgood = False;
  if (a == NULL) ; // error already reported
  else if (a->type->type != Type_ARRAY) {
    semanticError("symbol '%s' is not an array\n", var->name);
  }
  else lhsgood = True;

  Bool rhsgood = False;
  if (idx->type == NULL) ; // don't report unknown type
  else if (idx->type->type != Type_INTEGER) {
    semanticError("array subscript is not an integer\n");
  }
  else rhsgood = True;

  if (lhsgood && rhsgood) {
    arr->type = copyType(a->type->itemType);
  }
}

void mdArrayIndexCheck(struct Expr *arr) {
  struct Expr *var = arr->args, *idx = var->next;
  Bool lhsgood = False;
  if (var->type == NULL) ; // error already reported
  else if (var->type->type != Type_ARRAY) {
    // find the array name
    struct Expr *find = var;
    while (find->op == Op_INDEX) {
      find = find->args;
    }
    semanticError("too many subscripts on array '%s'\n", find->name);
  }
  else lhsgood = True;

  Bool rhsgood = False;
  if (idx->type == NULL) ; // don't report unknown type
  else if (idx->type->type != Type_INTEGER) {
    semanticError("array subscription is not an integer\n");
  }
  else rhsgood = True;

  if (lhsgood && rhsgood) {
    arr->type = var->type->itemType;
    free(var->type);
    var->type = NULL;
  }
}

void printCheck(struct Expr *expr) {
  if (expr->type == NULL) return;
  if (isScalarType(expr->type)) {
    ;
  }
  else if (expr->type->type == Type_ARRAY) {
    semanticError("operand of print statement is array type\n");
  }
  else if (expr->type->type == Type_VOID) {
    semanticError("operand of print statement is void type\n");
  }
}

void readCheck(struct Expr *var) {
  if (var->type == NULL) return;
  if (isScalarType(var->type)) {
    if (var->op == Op_VAR) {
      struct SymTableEntry *a = getSymEntry(var->name);
      if (a == NULL) ;
      else if (a->kind == SymbolKind_loopVar) {
        semanticError("loop variable '%s' cannot be assigned\n", var->name);
      }
      else if (a->kind == SymbolKind_constant) {
        semanticError("constant '%s' cannot be assigned\n", var->name);
      }
    }
  }
  else if (var->type->type == Type_ARRAY) {
    semanticError("operand of read statement is array type\n");
  }
  else if (var->type->type == Type_VOID) {
    semanticError("operand of read statement is void type\n");
  }
}

static void binaryOpErr(struct Expr *op1, struct Expr *op2, enum Operator op) {
  semanticError("invalid operand to binary '%s', LHS = ", OpName[op]);
  showType(op1->type);
  printf(", RHS = ");
  showType(op2->type);
  puts("");
}

void arithOpCheck(struct Expr *expr) {
  struct Expr *op1 = expr->args, *op2 = op1->next;
  if (op1->type == NULL || op2->type == NULL) return ;
  Bool lhs = op1->type->type == Type_INTEGER || op1->type->type == Type_REAL;
  Bool rhs = op2->type->type == Type_INTEGER || op2->type->type == Type_REAL;
  if (lhs && rhs) { // correct
    if (op1->type->type == Type_REAL || op2->type->type == Type_REAL) {
      // coerce to real
      expr->type = createScalarType(Type_REAL);
    }
    else {
      expr->type = createScalarType(Type_INTEGER);
    }
  }
  else if (expr->op == Op_PLUS
    && op1->type->type == Type_STRING && op2->type->type == Type_STRING) {
    // string + string is legal
    expr->type = createScalarType(Type_STRING);
  }
  else {
    binaryOpErr(op1, op2, expr->op);
  }
}

void modOpCheck(struct Expr *expr) {
  struct Expr *op1 = expr->args, *op2 = op1->next;
  if (op1->type == NULL || op2->type == NULL) return ;
  Bool lhs = op1->type->type == Type_INTEGER;
  Bool rhs = op2->type->type == Type_INTEGER;
  if (lhs && rhs) { // correct
    expr->type = createScalarType(Type_INTEGER);
  }
  else {
    binaryOpErr(op1, op2, expr->op);
  }
}

void boolOpCheck(struct Expr *expr) {
  struct Expr *op1 = expr->args, *op2 = op1->next;
  if (op1->type == NULL || op2->type == NULL) return ;
  Bool lhs = op1->type->type == Type_BOOLEAN;
  Bool rhs = op2->type->type == Type_BOOLEAN;
  if (lhs && rhs) { // correct
    expr->type = createScalarType(Type_BOOLEAN);
  }
  else {
    binaryOpErr(op1, op2, expr->op);
  }
}

void relOpCheck(struct Expr *expr) {
  struct Expr *op1 = expr->args, *op2 = op1->next;
  if (op1->type == NULL || op2->type == NULL) return ;
  Bool lhs = op1->type->type == Type_INTEGER || op1->type->type == Type_REAL;
  Bool rhs = op2->type->type == Type_INTEGER || op2->type->type == Type_REAL;
  if (lhs && rhs) { // correct
    expr->type = createScalarType(Type_BOOLEAN);
  }
  else {
    binaryOpErr(op1, op2, expr->op);
  }
}

// there are only 2 unary ops: '-' and 'not'
void unaryOpCheck(struct Expr *expr) {
  struct Expr *op = expr->args;
  if (op->type == NULL) return ;
  Bool good = False;
  if (expr->op == Op_UMINUS) {
    if (op->type->type == Type_INTEGER || op->type->type == Type_REAL) {
      expr->type = copyType(op->type);
      good = True;
    }
    else {
      semanticError("invalid operand to unary minus, type = ");
    }
  }
  else if (expr->op == Op_NOT) {
    if (op->type->type == Type_BOOLEAN) {
      expr->type = createScalarType(Type_BOOLEAN);
      good = True;
    }
    else {
      semanticError("invalid operand to unary not, type = ");
    }
  }
  if (!good) {
    showType(op->type);
    puts("");
  }
}

void functionCheck(struct Expr *expr) {
  struct Expr *nm = expr->args, *args = nm->next;
  struct SymTableEntry *a = getSymEntry(nm->name);
  // find a function symbol
  if (a == NULL) {
    semanticError("symbol '%s' is not defined\n", nm->name);
    return ;
  }
  while (a->kind != SymbolKind_function && a->prev != NULL) {
    a = a->prev;
  }
  if (a->kind != SymbolKind_function) {
    semanticError("symbol '%s' is not a function\n", nm->name);
    return ;
  }
  expr->type = copyType(a->type);
  int i, nargs = a->attr.argType.arity;
  struct Expr *ptr = args;
  for (i = 0; i < nargs && ptr != NULL; i++, ptr = ptr->next) {
    if (ptr->type == NULL) continue;
    if (!canConvertTypeImplicitly(ptr->type, a->attr.argType.types[i])) {
      semanticError("argument %d of '%s' type mismatch, expected '", i+1, nm->name);
      showType(a->attr.argType.types[i]);
      printf("' but argument is '");
      showType(ptr->type);
      puts("'");
    }
  }
  if (ptr != NULL) {
    semanticError("too many arguments to function '%s'\n", nm->name);
  }
  else if (i < nargs) {
    semanticError("too few arguments to function '%s'\n", nm->name);
  }
}
