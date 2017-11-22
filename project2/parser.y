%{
#include <stdio.h>
#include <stdlib.h>

extern int linenum;             /* declared in lex.l */
extern FILE *yyin;              /* declared by lex */
extern char *yytext;            /* declared by lex */
extern char buf[256];           /* declared in lex.l */

int yyerror( char *msg );
extern int yylex(void);
%}

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
%right UMINUS // unary minus '-'

// literals
%token INT_LIT STR_LIT REAL_LIT
%token IDENT
// token error
%token ERROR
%%

program		: programname SEMICOLON programbody END IDENT
		;

programname	: identifier
		;

programbody	: var_or_const_decls  function_decls  compound_stmt
		;

var_or_const_decls :
  /* empty */
| var_or_const_decls var_decl
| var_or_const_decls const_decl
;

var_decl :
  VAR identifier_list COLON type SEMICOLON
;

const_decl :
  VAR identifier_list COLON literal_constant SEMICOLON
;

identifier_list :
  identifier
| identifier_list COMMA identifier
;

function_decls :
  /* empty */
| function_decls  function_decl
;

function_decl :
  function_name LPAREN formal_args RPAREN function_type SEMICOLON
  function_body END IDENT
;

function_type :
  COLON type
| /* procedure has no type */
;

function_name : identifier
;

formal_args :
  /* no args */
| formal_args_list
;

formal_args_list :
  formal_arg
| formal_args_list COMMA formal_arg
;

formal_arg : identifier_list COLON type
;

function_body : compound_stmt
;

compound_stmt :
  BEGIN_  var_or_const_decls  statements END
;

statements :
  /* empty */
| statements statement
;

statement :
  compound_stmt
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
  identifier
| array_reference
;

array_reference :
  identifier LBRACKET integer_expression RBRACKET
| array_reference LBRACKET integer_expression RBRACKET
;

integer_expression : expression
;

expression :
  LPAREN expression RPAREN
| literal_constant_no_minus
| variable_reference
| function_invoc

| MINUS expression %prec UMINUS
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
  FOR identifier ASSIGN integer_constant TO integer_constant DO
  statements
  END DO
;

return_stmt : RETURN expression SEMICOLON
;

procedure_call : function_invoc SEMICOLON
;

literal_constant :
  literal_constant_no_minus
| MINUS INT_LIT
| MINUS REAL_LIT
;

literal_constant_no_minus :
  STR_LIT
| INT_LIT
| REAL_LIT
| TRUE
| FALSE
;

type :
  scalar_type
| ARRAY positive_integer_constant TO positive_integer_constant OF type
;

scalar_type :
  BOOLEAN
| INTEGER
| REAL
| STRING
;

// must be positive
positive_integer_constant :
  INT_LIT
;

integer_constant :
  INT_LIT
| MINUS INT_LIT
;

identifier	: IDENT
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
	yyparse();

	fprintf( stdout, "\n" );
	fprintf( stdout, "|--------------------------------|\n" );
	fprintf( stdout, "|  There is no syntactic error!  |\n" );
	fprintf( stdout, "|--------------------------------|\n" );
	exit(0);
}
