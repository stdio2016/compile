%{
#include <stdio.h>
#include <stdlib.h>
#include "symtable.h"
#include <string.h>

extern int linenum;             /* declared in tokens.l */
extern FILE *yyin;              /* declared by lex */
extern char *yytext;            /* declared by lex */
extern char buf[256];           /* declared in tokens.l */
extern int Opt_D;               /* declared in tokens.l */

int yyerror( char *msg );
extern int yylex(void);
%}

%union {
  char *name;
  struct Type *type;
  enum TypeEnum typeEnum;
  struct Constant lit;
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
%%

program		: programname SEMICOLON programbody END IDENT
		{
		  popScope(Opt_D);
		  free($5);
		}
		;

programname	: identifier { addSymbol($1, SymbolKind_program); }
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
  formal_args RPAREN function_type SEMICOLON { endFuncDecl($6); }
  compound_stmt END IDENT
  {
    // popScope is done by compound_stmt
    free($11);
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

function_name : identifier { addSymbol($1, SymbolKind_function); };

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
  variable_reference ASSIGN expression SEMICOLON
| PRINT expression SEMICOLON
| READ variable_reference SEMICOLON
;

variable_reference :
  identifier { free($1); }
| array_reference
;

array_reference :
  identifier { free($1); } LBRACKET integer_expression RBRACKET
| array_reference LBRACKET integer_expression RBRACKET
;

integer_expression : expression
;

minus_expr :
  LPAREN expression RPAREN
| variable_reference
| function_invoc
;

expression :
  minus_expr
| literal_constant { if ($1.type == Type_STRING) free($1.str); }
| MINUS minus_expr

| expression MULTIPLY expression
| expression DIVIDE   expression
| expression MOD      expression
| expression PLUS  expression
| expression MINUS expression

| expression LESS     expression
| expression LEQUAL   expression
| expression EQUAL    expression
| expression GEQUAL   expression
| expression GREATER  expression
| expression NOTEQUAL expression

| NOT expression
| expression AND expression
| expression OR expression
;

function_invoc : identifier LPAREN arg_list RPAREN
;

arg_list :
  /* no arguments */
| arguments
;

arguments :
  expression
| arguments COMMA expression
;

conditional_stmt :
  IF boolean_expr THEN
  statements
  ELSE
  statements
  END IF
|
  IF boolean_expr THEN
  statements
  END IF
;

boolean_expr : expression
;

while_stmt :
  WHILE boolean_expr DO
  statements
  END DO
;

for_stmt :
  FOR identifier { addLoopVar($2); } ASSIGN integer_constant TO integer_constant DO
  statements
  END DO { removeLoopVar(); }
;

return_stmt : RETURN expression SEMICOLON
;

procedure_call : function_invoc SEMICOLON
;

literal_constant :
  integer_constant
| STR_LIT
| REAL_LIT
| MINUS REAL_LIT {
  $$ = $2;
  $$.real = -($$.real);
}
| TRUE { $$.type = Type_BOOLEAN; $$.boolean = 1; }
| FALSE { $$.type = Type_BOOLEAN; $$.boolean = 0; }
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

identifier	: IDENT { $<name>$ = $1; }
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

int  main( int argc, char **argv )
{
	if( argc != 2 ) {
		fprintf(  stdout,  "Usage:  ./parser  [filename]\n"  );
		exit(0);
	}

	FILE *fp = fopen( argv[1], "r" );
	
	if( fp == NULL )  {
		fprintf( stdout, "Open  file  error\n" );
		exit(-1);
	}
	
	yyin = fp;
	initSymTable();
	yyparse();

	exit(0);
}
