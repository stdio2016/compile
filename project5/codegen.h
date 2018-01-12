#pragma once
#ifndef CODEGEN_INCLUDED
#define CODEGEN_INCLUDED
#include "ast.h"
#include "symtable.h"

// called at start
void initCodeGen(const char *filename);
void genProgStart(void);

// called at end
void endCodeGen(void);

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
// generate an insert point without a label
int genInsertPoint(void);

// output code!
void genCode(const char *code, int useStack, int stackUpDown);
void genCodeAt(const char *code, int addr);
// output integer
void genIntCode(int num);
// output type
void genTypeCode(struct Type *type);
// output function name and type
void genFuncTypeCode(struct SymTableEntry *e, const char *funname);
// output constant
void genConstCode(struct Constant val);

// generate a function, called at end of a function
void genFunctionStart(const char *funname);
void genProgMain(void);
void genFunctionEnd();

void genFunctionCall(struct SymTableEntry *e, const char *funname, struct PatchList *list, struct Expr *args);
void genLoadVar(const char *varname);
void genStoreVar(const char *varname);
void genLoadArray(struct Expr *expr);
void genStoreArray(struct Expr *expr);
void genArrayIndexShift(int offset);

void genGlobalVarCode(const char *name, struct Type *type);

struct PatchList *makePatchList(int addr);
void destroyPatchList(struct PatchList *list);

// will make *list unusable
void backpatch(struct PatchList *list, int label);

// will make *p1 and *p2 unusable
struct PatchList *mergePatchList(struct PatchList *p1, struct PatchList *p2);


#endif
