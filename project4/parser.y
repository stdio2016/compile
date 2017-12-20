%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"
#include "errReport.h"
#include "semcheck.h"

extern int linenum;             /* declared in tokens.l */
extern FILE *yyin;              /* declared by lex */
extern char *yytext;            /* declared by lex */
extern char buf[256];           /* declared in tokens.l */
extern int Opt_D;               /* declared in tokens.l */

int yyerror( char *msg );
extern int yylex(void);
char *filename;
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
%type<expr> minus_expr factor term expression relation_expr boolean_factor boolean_term boolean_expr
%type<op> mul_op plus_op relop
%type<args> arg_list arguments
%%

program		: programname SEMICOLON programbody END IDENT
		{
		  if (strcmp($<pairName>1.name, $5) != 0) {
		    semanticError("program end ID inconsist with the beginning ID\n");
		  }
		  if (strcmp($5, filename) != 0) {
		    semanticError("program end ID inconsist with file name\n");
		  }
		  popScope(Opt_D);
		  free($5);
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

programbody	: var_or_const_decls  function_decls  { pushScope(); } compound_stmt
		;

var_or_const_decls :
  /* empty */
| var_or_const_decls var_decl
| var_or_const_decls const_decl
;

var_decl :
  VAR startVarDecl identifier_list COLON type SEMICOLON { endVarDecl($5); }
;

const_decl :
  VAR startVarDecl identifier_list COLON literal_constant SEMICOLON { endConstDecl($5); }
;

startVarDecl: {
  startVarDecl();
};

identifier_list :
  identifier { addSymbol($1, SymbolKind_variable); }
| identifier_list COMMA identifier { addSymbol($3, SymbolKind_variable); }
;

function_decls :
  /* empty */
| function_decls  function_decl
;

function_decl :
  function_name LPAREN { pushScope(); }
  formal_args RPAREN function_type SEMICOLON
  {
    funcReturnType = copyType($6);
    endFuncDecl($6, $<pairName>1.success);
  }
  compound_stmt END IDENT
  {
    // popScope is done by compound_stmt
    if (strcmp($<pairName>1.name, $11) != 0) {
      semanticError("function end ID inconsist with the beginning ID\n");
    }
    free($<pairName>1.name);
    free($11);
    free(funcReturnType);
    funcReturnType = NULL;
  }
;

function_type :
  COLON type { $$ = $2; }
| /* procedure has no type */ {
    $$ = malloc(sizeof(struct Type));
    $$->itemType = NULL;
    $$->type = Type_VOID;
  }
;

function_name : identifier {
  $<pairName>$ = addSymbol($1, SymbolKind_function);
  $<pairName>$.name = dupstr($1);
};

formal_args :
  /* no args */
| formal_args_list
;

formal_args_list :
  formal_arg
| formal_args_list SEMICOLON formal_arg
;

formal_arg : { startParamDecl(); }
  identifier_list COLON type { endParamDecl($4); }
;

compound_stmt :
  BEGIN_  var_or_const_decls  statements END { popScope(Opt_D); }
;

statements :
  /* empty */
| statements statement
;

statement :
  { pushScope(); } compound_stmt
| simple_stmt
| conditional_stmt
| while_stmt
| for_stmt
| return_stmt
| procedure_call
;

simple_stmt :
  variable_reference ASSIGN boolean_expr SEMICOLON
  {
    assignCheck($1, $3);
    destroyExpr($1); destroyExpr($3);
  }
| PRINT boolean_expr SEMICOLON
  {
    printCheck($2);
    destroyExpr($2);
  }
| READ variable_reference SEMICOLON
  {
    readCheck($2);
    destroyExpr($2);
  }
;

variable_reference :
  identifier { $$ = createVarExpr($1);  varTypeCheck($$); }
| array_reference
;

array_reference :
  identifier LBRACKET boolean_expr RBRACKET
  {
    $$ = createExpr(Op_INDEX, createVarExpr($1), $3);
    arrayTypeCheck($$);
  }
| array_reference LBRACKET boolean_expr RBRACKET
  {
    $$ = createExpr(Op_INDEX, $1, $3);
    mdArrayIndexCheck($$);
  }
;

minus_expr :
  LPAREN boolean_expr RPAREN { $$ = $2; }
| variable_reference
| function_invoc
;

factor :
  minus_expr
| literal_constant { $$ = createLitExpr($1); }
| MINUS minus_expr
  {
    $$ = createExpr(Op_UMINUS, $2, NULL);
  }
;

term :
  factor
| term mul_op factor
  {
    $$ = createExpr($2, $1, $3);
  }
;

mul_op :
  MULTIPLY { $$ = Op_MULTIPLY; }
| DIVIDE { $$ = Op_DIVIDE; }
| MOD { $$ = Op_MOD; }
;

expression :
  term
| expression plus_op term
  {
    $$ = createExpr($2, $1, $3);
  }
;

plus_op :
  PLUS { $$ = Op_PLUS; }
| MINUS { $$ = Op_MINUS; }
;

relation_expr :
  expression
| relation_expr relop expression
  {
    $$ = createExpr($2, $1, $3);
  }
;

relop :
  LESS { $$ = Op_LESS; }
| LEQUAL { $$ = Op_LEQUAL; }
| EQUAL { $$ = Op_EQUAL; }
| GEQUAL { $$ = Op_GEQUAL; }
| GREATER { $$ = Op_GREATER; }
| NOTEQUAL { $$ = Op_NOTEQUAL; }
;

boolean_factor :
  relation_expr
| NOT boolean_factor
  {
    $$ = createExpr(Op_NOT, $2, NULL);
  }
;

boolean_term :
  boolean_factor
| boolean_term AND boolean_factor
  {
    $$ = createExpr(Op_AND, $1, $3);
  }
;

boolean_expr :
  boolean_term
| boolean_expr OR boolean_term
  {
    $$ = createExpr(Op_OR, $1, $3);
  }
;

function_invoc : identifier LPAREN arg_list RPAREN
  {
    $$ = createFuncExpr($1, $3.first);
  }
;

arg_list :
  /* no arguments */ { initExprList(&$$); }
| arguments
;

arguments :
  boolean_expr
  {
    initExprList(&$$);
    addToExprList(&$$, $1);
  }
| arguments COMMA boolean_expr
  {
    addToExprList(&$1, $3); $$ = $1;
  }
;

conditional_stmt :
  IF condition THEN
  statements
  ELSE
  statements
  END IF
|
  IF condition THEN
  statements
  END IF
;

condition :
  boolean_expr {
    conditionCheck($1, "if");
    destroyExpr($1);
  }
;

while_stmt :
  WHILE boolean_expr {
    conditionCheck($2, "while");
    destroyExpr($2);
  } DO
  statements
  END DO
;

for_stmt :
  FOR identifier { $<boolVal>$ = addLoopVar($2); } ASSIGN integer_constant TO integer_constant
  { forCheck($5.integer, $7.integer); }
  DO
  statements
  END DO { if ($<boolVal>3) removeLoopVar(); }
;

return_stmt : RETURN boolean_expr SEMICOLON
  {
    returnCheck($2, funcReturnType);
    destroyExpr($2);
  }
;

procedure_call : function_invoc SEMICOLON { destroyExpr($1); }
;

literal_constant :
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

type :
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

scalar_type :
  BOOLEAN  { $$ = Type_BOOLEAN; }
| INTEGER  { $$ = Type_INTEGER; }
| REAL  { $$ = Type_REAL; }
| STRING  { $$ = Type_STRING; }
;

// must be positive
positive_integer_constant :
  INT_LIT
;

integer_constant :
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
	yyparse();
	destroySymTable();
	free(filename);
	//yylex_destroy();
	fclose(fp);

	exit(0);
}
