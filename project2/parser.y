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
%%

program		: programname SEMICOLON programbody END IDENT
		;

programname	: identifier
		;

programbody	: var_or_const_decls
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

literal_constant :
  STR_LIT
| integer_constant
| REAL_LIT
| TRUE
| FALSE
;

type :
  scalar_type
| ARRAY integer_constant TO integer_constant OF type
;

scalar_type :
  BOOLEAN
| INTEGER
| REAL
| STRING
;

integer_constant : INT_LIT
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
