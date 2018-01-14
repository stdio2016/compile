%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"
#include "errReport.h"
#include "semcheck.h"
#include "codegen.h"

extern int linenum;             /* declared in tokens.l */
extern FILE *yyin;              /* declared by lex */
extern char *yytext;            /* declared by lex */
extern char buf[256];           /* declared in tokens.l */
extern int Opt_D;               /* declared in tokens.l */

int yyerror( char *msg );
extern int yylex(void);
char *filename;
char *progClassName;
struct Type *funcReturnType = NULL;
%}

%union {
  char *name;
  struct Type *type;
  enum TypeEnum typeEnum;
  struct Constant lit;
  struct PairName pairName; // for program and function name
  Bool boolVal;
  struct Expr *expr;
  struct ExprList args;
  enum Operator op;
  struct BoolExpr boolExpr;
  struct Statement stmt;
  int label;
}

// delimiters
%token COMMA SEMICOLON COLON
%token LPAREN RPAREN // ( )
%token LBRACKET RBRACKET // [ ]

// keywords
%token ARRAY BOOLEAN INTEGER REAL STRING
%token FALSE TRUE
%token DEF
%token DO ELSE END FOR IF OF PRINT READ THEN TO RETURN VAR WHILE
// rename to BEGIN_ because BEGIN is a keyword in lex
%token BEGIN_

// operators
%token ASSIGN
%left OR
%left AND
%right NOT
%left LESS LEQUAL NOTEQUAL GEQUAL GREATER EQUAL
%left PLUS MINUS
%left MULTIPLY DIVIDE MOD

// literals
%token<lit> INT_LIT STR_LIT REAL_LIT
%token<name> IDENT
// token error
%token ERROR

// non-terminals with type
%type<lit> literal_constant integer_constant positive_integer_constant
%type<name> identifier
%type<typeEnum> scalar_type
%type<type> type function_type

%type<expr> variable_reference array_reference function_invoc
%type<boolExpr> minus_expr factor term expression relation_expr boolean_factor boolean_term boolean_expr
%type<op> mul_op plus_op relop
%type<args> arg_list arguments
%type<label> Mark Label
%type<stmt> statements statement compound_stmt conditional_stmt while_stmt for_stmt IfGoto
%type<boolExpr> condition
%%

program		: programname SEMICOLON
		{
		  progClassName = $<pairName>1.name;
		  genProgStart();
		}
		  programbody END IDENT
		{
		  if (strcmp($<pairName>1.name, $6) != 0) {
		    semanticError("program end ID inconsist with the beginning ID\n");
		  }
		  if (strcmp($6, filename) != 0) {
		    semanticError("program end ID inconsist with file name\n");
		  }
		  popScope(Opt_D);
		  free($6);
		}
		;

programname	: identifier
		{
		  if (strcmp($1, filename) != 0) {
		    semanticError("program beginning ID inconsist with file name\n");
		  }
		  $<pairName>$ = addSymbol($1, SymbolKind_program);
		}
		;

programbody	: var_or_const_decls  function_decls
		{ pushScope(); genProgMain(); } compound_stmt
		{ backpatch($4.nextlist,genLabel()); genCode("  return\n",0,0); genFunctionEnd(); }
		;

var_or_const_decls:
  /* empty */
| var_or_const_decls var_decl
| var_or_const_decls const_decl
;

var_decl:
  VAR startVarDecl identifier_list COLON type SEMICOLON { endVarDecl($5); }
;

const_decl:
  VAR startVarDecl identifier_list COLON literal_constant SEMICOLON { endConstDecl($5); }
;

startVarDecl: {
  startVarDecl();
};

identifier_list:
  identifier { addSymbol($1, SymbolKind_variable); }
| identifier_list COMMA identifier { addSymbol($3, SymbolKind_variable); }
;

function_decls:
  /* empty */
| function_decls  function_decl
;

function_decl:
  function_name LPAREN { pushScope(); }
  formal_args RPAREN function_type SEMICOLON
  {
    funcReturnType = copyType($6);
    endFuncDecl($6, $<pairName>1.success);
    genFunctionStart($<pairName>1.name);
  }
  compound_stmt { backpatch($9.nextlist,genLabel()); } END IDENT
  {
    // popScope is done by compound_stmt
    if (strcmp($<pairName>1.name, $12) != 0) {
      semanticError("function end ID inconsist with the beginning ID\n");
    }
    // append return at end
    if (funcReturnType->type == Type_INTEGER || funcReturnType->type == Type_BOOLEAN)
      genCode("  iconst_0\n  ireturn\n",1,0);
    else if (funcReturnType->type == Type_REAL)
      genCode("  fconst_0\n  freturn\n",1,0);
    else if (funcReturnType->type == Type_STRING)
      genCode("  ldc \"\"\n  areturn\n",1,0);
    else
      genCode("  return\n",0,0);
    genFunctionEnd();
    free($<pairName>1.name);
    free($12);
    destroyType(funcReturnType);
    funcReturnType = NULL;
  }
;

function_type:
  COLON type { $$ = $2; }
| /* procedure has no type */ {
    $$ = malloc(sizeof(struct Type));
    $$->itemType = NULL;
    $$->type = Type_VOID;
  }
;

function_name: identifier {
  $<pairName>$ = addSymbol($1, SymbolKind_function);
  $<pairName>$.name = dupstr($<pairName>$.name);
};

formal_args:
  /* no args */
| formal_args_list
;

formal_args_list:
  formal_arg
| formal_args_list SEMICOLON formal_arg
;

formal_arg: { startParamDecl(); }
  identifier_list COLON type { endParamDecl($4); }
;

compound_stmt:
  BEGIN_  var_or_const_decls  statements END
  { popScope(Opt_D); $$.nextlist = $3.nextlist; }
;

statements:
  /* empty */ { $$.nextlist = NULL; }
| statements Label statement
  {
    backpatch($1.nextlist, $2);
    $$.nextlist = $3.nextlist;
  }
;

statement:
  { pushScope(); } compound_stmt { $$.nextlist = $2.nextlist; }
| simple_stmt  { $$.nextlist = NULL; }
| conditional_stmt
| while_stmt
| for_stmt
| return_stmt { $$.nextlist = NULL; }
| procedure_call { $$.nextlist = NULL; }
;

simple_stmt:
  variable_reference ASSIGN boolean_expr SEMICOLON
  {
    assignCheck($1, BoolExprToExpr($3));
    if ($1->op == Op_VAR) {
      struct SymTableEntry *e = getSymEntry($1->name);
      if (e != NULL && e->type->type == Type_REAL) { // integer -> real
        if ($3.expr->type != NULL && $3.expr->type->type == Type_INTEGER) {
          genCode("  i2f\n",0,0);
        }
      }
      genStoreVar($1->name);
    }
    else if ($1->op == Op_INDEX) {
      if ($1->type != NULL && $1->type->type == Type_REAL) { // integer -> real
        if ($3.expr->type != NULL && $3.expr->type->type == Type_INTEGER) {
          genCode("  i2f\n",0,0);
        }
      }
      genStoreArray($1);
    }
    destroyExpr($1); destroyExpr($3.expr);
  }
| PRINT
  { genCode("  getstatic java/lang/System/out Ljava/io/PrintStream;\n",1,+1); }
  boolean_expr SEMICOLON
  {
    printCheck(BoolExprToExpr($3));
    genCode("  invokevirtual java/io/PrintStream/print(",0,-2);
    genTypeCode($3.expr->type);
    genCode(")V\n",0,0);
    destroyExpr($3.expr);
  }
| READ variable_reference SEMICOLON
  {
    readCheck($2);
    genCode("  getstatic ",0,0);
    genCode(progClassName,0,0);
    genCode("/_sc Ljava/util/Scanner;\n",1,+1);
    genCode("  invokevirtual java/util/Scanner/",0,0);
    if ($2->type == NULL) genCode("nextInt()I\n",0,0); 
    else if ($2->type->type == Type_BOOLEAN) genCode("nextBoolean()Z\n",0,0);
    else if ($2->type->type == Type_REAL) genCode("nextFloat()F\n",0,0);
    else if ($2->type->type == Type_INTEGER) genCode("nextInt()I\n",0,0);
    else if ($2->type->type == Type_STRING) genCode("next()Ljava/lang/String;\n",0,0);
    if ($2->op == Op_VAR) genStoreVar($2->name);
    else if ($2->op == Op_INDEX) genStoreArray($2);
    destroyExpr($2);
  }
;

variable_reference:
  identifier { $$ = createVarExpr($1);  varTypeCheck($$); }
| array_reference
;

array_reference:
  identifier { genLoadVar($1); } LBRACKET boolean_expr RBRACKET
  {
    $$ = createExpr(Op_INDEX, createVarExpr($1), BoolExprToExpr($4));
    arrayTypeCheck($$);
    struct SymTableEntry *e = getSymEntry($1);
    if (e != NULL && e->type->type == Type_ARRAY) { // not base 0 index
      int off = e->type->lowerBound;
      genArrayIndexShift(off);
    }
  }
| array_reference { genCode("  aaload\n",0,-1); } LBRACKET boolean_expr RBRACKET
  {
    $$ = createExpr(Op_INDEX, $1, BoolExprToExpr($4));
    if ($1->type != NULL) { // not base 0 index
      int off = $1->type->lowerBound;
      genArrayIndexShift(off);
    }
    mdArrayIndexCheck($$);
  }
;

minus_expr:
  LPAREN boolean_expr RPAREN { $$ = $2; }
| variable_reference
  {
    $$ = ExprToBoolExpr($1);
    if ($1->op == Op_VAR) genLoadVar($1->name);
    else if ($1->op == Op_INDEX) genLoadArray($1);
  }
| function_invoc { $$ = ExprToBoolExpr($1); }
;

factor:
  minus_expr
| literal_constant
  {
    $$ = ExprToBoolExpr(createLitExpr($1));
    genConstCode($1);
  }
| MINUS minus_expr
  {
    $$ = ExprToBoolExpr(createExpr(Op_UMINUS, BoolExprToExpr($2), NULL));
    unaryOpCheck($$.expr);
    Bool isFloat = $$.expr->type != NULL && $$.expr->type->type == Type_REAL;
    genCode(isFloat ? "  fneg\n" : "  ineg\n", 0, 0);
  }
;

term:
  factor
| term mul_op { BoolExprToExpr($1); } Mark factor
  {
    BoolExprToExpr($5);
    $$ = createBoolExpr($2, $1, $5);
    if ($2 == Op_MOD)
      modOpCheck($$.expr);
    else
      arithOpCheck($$.expr);
    Bool isFloat = $$.expr->type != NULL && $$.expr->type->type == Type_REAL;
    if (isFloat) {
      if ($1.expr->type->type == Type_INTEGER) genCodeAt("  i2f\n", $4);
      if ($5.expr->type->type == Type_INTEGER) genCode("  i2f\n",0,0);
    }
    if ($2 == Op_MULTIPLY) genCode(isFloat ? "  fmul\n" : "  imul\n", 0, -1);
    else if ($2 == Op_DIVIDE) genCode(isFloat ? "  fdiv\n" : "  idiv\n", 0, -1);
    else if ($2 == Op_MOD) genCode(isFloat ? "  frem\n" : "  irem\n", 0, -1);
    // frem is illegal in P language
  }
;

Mark: {
  $$ = genInsertPoint();
};

mul_op:
  MULTIPLY { $$ = Op_MULTIPLY; }
| DIVIDE { $$ = Op_DIVIDE; }
| MOD { $$ = Op_MOD; }
;

expression:
  term
| expression plus_op { BoolExprToExpr($1); } Mark term
  {
    BoolExprToExpr($5);
    $$ = createBoolExpr($2, $1, $5);
    arithOpCheck($$.expr);
    Bool isFloat = $$.expr->type != NULL && $$.expr->type->type == Type_REAL;
    if (isFloat) {
      if ($1.expr->type->type == Type_INTEGER) genCodeAt("  i2f\n", $4);
      if ($5.expr->type->type == Type_INTEGER) genCode("  i2f\n",0,0);
    }
    if ($2 == Op_PLUS) {
      if ($$.expr->type != NULL && $$.expr->type->type == Type_STRING) { // concat string
        genCode("  invokevirtual java/lang/String/concat(Ljava/lang/String;)Ljava/lang/String;\n", 0, -1);
      }
      else genCode(isFloat ? "  fadd\n" : "  iadd\n", 0, -1);
    }
    else if ($2 == Op_MINUS) genCode(isFloat ? "  fsub\n" : "  isub\n", 0, -1);
  }
;

plus_op:
  PLUS { $$ = Op_PLUS; }
| MINUS { $$ = Op_MINUS; }
;

relation_expr:
  expression
| relation_expr relop { BoolExprToExpr($1); } Mark expression
  {
    BoolExprToExpr($5);
    $$ = createBoolExpr($2, $1, $5);
    relOpCheck($$.expr);
    int real1 = $$.expr->type != NULL && $1.expr->type->type == Type_REAL;
    int real2 = $$.expr->type != NULL && $5.expr->type->type == Type_REAL;
    Bool f = real1 || real2;
    if (f) {
      if (!real1) genCode("  i2f\n",0,0);
      if (!real2) genCodeAt("  i2f\n",$4);
    }
    switch ($2) {
      case Op_LESS: genCode(f?"  fcmpg\n  iflt ":"  if_icmplt ",0,-2); break;
      case Op_LEQUAL: genCode(f?"  fcmpg\n  ifle ":"  if_icmple ",0,-2); break;
      case Op_EQUAL: genCode(f?"  fcmpl\n  ifeq ":"  if_icmpeq ",0,-2); break;
      case Op_GEQUAL: genCode(f?"  fcmpl\n  ifge ":"  if_icmpge ",0,-2); break;
      case Op_GREATER: genCode(f?"  fcmpl\n  ifgt ":"  if_icmpgt ",0,-2); break;
      case Op_NOTEQUAL: genCode(f?"  fcmpl\n  ifne ":"  if_icmpne ",0,-2); break;
    }
    $$.isTFlist = True;
    $$.truelist = makePatchList(genInsertPoint());
    genCode("  goto ",0,0);
    $$.falselist = makePatchList(genLabel());
  }
;

relop:
  LESS { $$ = Op_LESS; }
| LEQUAL { $$ = Op_LEQUAL; }
| EQUAL { $$ = Op_EQUAL; }
| GEQUAL { $$ = Op_GEQUAL; }
| GREATER { $$ = Op_GREATER; }
| NOTEQUAL { $$ = Op_NOTEQUAL; }
;

boolean_factor:
  relation_expr
| NOT boolean_factor
  {
    $$ = ExprToBoolExpr(createExpr(Op_NOT, $2.expr, NULL));
    unaryOpCheck($$.expr);
    $$.isTFlist = $2.isTFlist;
    if ($2.isTFlist) {
      $$.truelist = $2.falselist;
      $$.falselist = $2.truelist;
    }
    else {
      genCode("  iconst_1\n  ixor\n",1,0);
    }
  }
;

Label: {
  $$ = genLabel();
};

boolean_term:
  boolean_factor
| boolean_term AND Label boolean_factor
  {
    $$ = ExprToBoolExpr(createExpr(Op_AND, $1.expr, $4.expr));
    boolOpCheck($$.expr);
    $$.isTFlist = True;
    if (!$1.isTFlist) {
      genCodeAt("  ifeq ", $3);
      $1.falselist = makePatchList($3);
      $1.truelist = NULL;
      genCode("",0,-1); // move stack pointer back
    }
    if (!$4.isTFlist) {
      BoolExpr_toTFlist(&$4);
    }
    backpatch($1.truelist, $3);
    $$.falselist = mergePatchList($1.falselist, $4.falselist);
    $$.truelist = $4.truelist;
  }
;

boolean_expr:
  boolean_term
| boolean_expr OR Label boolean_term
  {
    $$ = ExprToBoolExpr(createExpr(Op_OR, $1.expr, $4.expr));
    boolOpCheck($$.expr);
    $$.isTFlist = True;
    if (!$1.isTFlist) {
      genCodeAt("  ifne ", $3);
      $1.truelist = makePatchList($3);
      $1.falselist = NULL;
      genCode("",0,-1);// move stack pointer back
    }
    if (!$4.isTFlist) {
      BoolExpr_toTFlist(&$4);
    }
    backpatch($1.falselist, $3);
    $$.truelist = mergePatchList($1.truelist, $4.truelist);
    $$.falselist = $4.falselist;
  }
;

function_invoc: identifier LPAREN arg_list RPAREN
  {
    $$ = createFuncExpr($1, $3.first);
    struct SymTableEntry *fun = getFunctionEntry($1);
    functionCheck($$, fun);
    struct Expr *e = $3.first;
    struct PatchList *p = $3.marks;
    genFunctionCall(fun, $1, p, e);
    destroyPatchList($3.marks);
  }
;

arg_list:
  /* no arguments */ { initExprList(&$$); }
| arguments
;

arguments:
  boolean_expr
  {
    initExprList(&$$);
    addToExprList(&$$, BoolExprToExpr($1));
    $$.marks = NULL;
  }
| arguments COMMA Mark boolean_expr
  {
    addToExprList(&$1, BoolExprToExpr($4)); $$ = $1;
    $$.marks = mergePatchList($1.marks, makePatchList($3));
  }
;

conditional_stmt:
  IF condition THEN Label
  statements IfGoto
  ELSE Label
  statements
  END IF
  {
    if (!$2.isTFlist) {
      genCodeAt("  ifeq ",$4);
      genCode("",0,-1); // move stack pointer back
      $2.truelist = NULL; // fallthrough
      $2.falselist = makePatchList($4);
    }
    backpatch($2.truelist, $4);
    backpatch($2.falselist, $8);
    struct PatchList *tmp = mergePatchList($5.nextlist, $6.nextlist);
    $$.nextlist = mergePatchList(tmp, $9.nextlist);
    destroyExpr($2.expr);
  }
|
  IF condition THEN Label
  statements
  END IF
  {
    if (!$2.isTFlist) {
      genCodeAt("  ifeq ",$4);
      genCode("",0,-1); // move stack pointer back
      $2.truelist = NULL; // fallthrough
      $2.falselist = makePatchList($4);
    }
    backpatch($2.truelist, $4);
    $$.nextlist = mergePatchList($2.falselist, $5.nextlist);
    destroyExpr($2.expr);
  }
;

IfGoto: {
  genCode("  goto ",0,0);
  $$.nextlist = makePatchList(genInsertPoint());
};

condition:
  boolean_expr {
    conditionCheck($1.expr, "if");
    $$ = $1;
  }
;

while_stmt:
  WHILE Label boolean_expr {
    //TODO
    conditionCheck($3.expr, "while");
    destroyExpr($3.expr);
  } DO Label
  statements
  END DO {
    if (!$3.isTFlist) {
      genCodeAt("  ifeq ",$6);
      genCode("",0,-1); // move stack pointer back
      $3.truelist = NULL; // fallthrough
      $3.falselist = makePatchList($6);
    }
    genCode("  goto ",0,0);
    int i = genInsertPoint();
    struct PatchList *tmp = mergePatchList(makePatchList(i), $7.nextlist);
    backpatch(tmp, $2);
    backpatch($3.truelist, $6);
    $$.nextlist = $3.falselist;
  }
;

for_stmt:
  FOR identifier { $<boolVal>$ = addLoopVar($2); } ASSIGN integer_constant TO integer_constant
  { forCheck($5.integer, $7.integer); }
  DO
  statements
  END DO { if ($<boolVal>3) removeLoopVar(); }
;

return_stmt: RETURN boolean_expr SEMICOLON
  {
    returnCheck(BoolExprToExpr($2), funcReturnType);
    if ($2.expr->type != NULL) {
      enum TypeEnum t = $2.expr->type->type;
      if (t == Type_INTEGER || t == Type_BOOLEAN)
        genCode("  ireturn\n", 0, -1);
      else if (t == Type_REAL) genCode("  freturn\n", 0, -1);
      else if (t == Type_STRING) genCode("  areturn\n", 0, -1);
    }
    destroyExpr($2.expr);
  }
;

procedure_call: function_invoc SEMICOLON
  {
    if ($1->type != NULL && $1->type->type != Type_VOID) {
      genCode("  pop\n", 0, -1);
    }
    destroyExpr($1);
  }
;

literal_constant:
  integer_constant
| STR_LIT
| REAL_LIT
| MINUS REAL_LIT {
  $$ = $2;
  $$.real = -($$.real);
}
| TRUE { $$.type = Type_BOOLEAN; $$.boolean = True; }
| FALSE { $$.type = Type_BOOLEAN; $$.boolean = False; }
;

type:
  scalar_type
  {
    $$ = malloc(sizeof(struct Type));
    $$->itemType = NULL;
    $$->type = $1;
  }
| ARRAY positive_integer_constant TO positive_integer_constant OF type
  {
    $$ = malloc(sizeof(struct Type));
    $$->itemType = $6;
    $$->upperBound = $4.integer;
    $$->lowerBound = $2.integer;
    $$->type = Type_ARRAY;
  }
;

scalar_type:
  BOOLEAN  { $$ = Type_BOOLEAN; }
| INTEGER  { $$ = Type_INTEGER; }
| REAL  { $$ = Type_REAL; }
| STRING  { $$ = Type_STRING; }
;

// must be positive
positive_integer_constant:
  INT_LIT
;

integer_constant:
  INT_LIT
| MINUS INT_LIT {
  $$ = $2;
  $$.integer = -($$.integer);
}
;

identifier	: IDENT { $$ = $1; }
		;

%%

int yyerror( char *msg )
{
        fprintf( stderr, "\n|--------------------------------------------------------------------------\n" );
	fprintf( stderr, "| Error found in Line #%d: %s\n", linenum, buf );
	fprintf( stderr, "|\n" );
	fprintf( stderr, "| Unmatched token: %s\n", yytext );
        fprintf( stderr, "|--------------------------------------------------------------------------\n" );
        exit(-1);
}

char *getfilename(const char *path) {
  const char *name = strrchr(path, '/');
  if (name == NULL) name = path;
  else name = name + 1;
  const char *dot = strchr(name, '.');
  if (dot == NULL) dot = name + strlen(name);
  size_t len = dot - name;
  char *newname = malloc(len+1);
  memcpy(newname, name, len);
  newname[len] = '\0';
  return newname;
}

int  main( int argc, char **argv )
{
	if( argc != 2 ) {
		fprintf(  stdout,  "Usage:  ./parser  [filename]\n"  );
		exit(0);
	}

	FILE *fp = fopen( argv[1], "r" );
	filename = getfilename(argv[1]);
	
	if( fp == NULL )  {
		fprintf( stdout, "Open  file  error\n" );
		free(filename);
		exit(-1);
	}
	
	yyin = fp;
	initSymTable();
	initCodeGen(argv[1]);
	yyparse();
	destroySymTable();
	free(filename);
	endCodeGen();
	//yylex_destroy();
	fclose(fp);
	if (errorCount == 0) {
		puts("|-------------------------------------------|");
		puts("| There is no syntactic and semantic error! |");
		puts("|-------------------------------------------|");
	}
	exit(0);
}
