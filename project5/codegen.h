#pragma once
#ifndef CODEGEN_INCLUDED
#define CODEGEN_INCLUDED
#include "ast.h"

// called at start
void initCodeGen(const char *filename);

// BoolExpr to Expr conversion
struct BoolExpr createBoolExpr(enum Operator op, struct BoolExpr op1, struct BoolExpr op2);
struct Expr *BoolExprToExpr(struct BoolExpr expr);
struct BoolExpr ExprToBoolExpr(struct Expr *expr);

// BoolExpr has two types: immed and tflist
// immed: push either 1 for true or 0 for false
// tflist: use jump to represent true/false
void BoolExpr_toTFlist(struct BoolExpr *expr);
void BoolExpr_toImmed(struct BoolExpr *expr);

// generate a label!
int genLabel();

// output code!
void genCode(const char *code, int useStack, int stackUpDown);

// generate a function, called at end of a function
void genFunction();

// will make *list unusable
void backpatch(struct PatchList *list, int label);

// will make *p1 and *p2 unusable
struct PatchList *mergePatchList(struct PatchList *p1, struct PatchList *p2);


#endif
