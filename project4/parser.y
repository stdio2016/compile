%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"
#include "errReport.h"

extern int linenum;             /* declared in tokens.l */
extern FILE *yyin;              /* declared by lex */
extern char *yytext;            /* declared by lex */
extern char buf[256];           /* declared in tokens.l */
extern int Opt_D;               /* declared in tokens.l */

int yyerror( char *msg );
extern int yylex(void);
char *filename;
%}

%union {
  char *name;
  struct Type *type;
  enum TypeEnum typeEnum;
  struct Constant lit;
  struct PairName pairName; // for program and function name
  Bool boolVal;
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
		  if (strcmp($<pairName>1.name, $5) != 0) {
		    semanticError("program end ID inconsist with the beginning ID");
		  }
		  if (strcmp($5, filename) != 0) {
		    semanticError("program end ID inconsist with file name");
		  }
		  popScope(Opt_D);
		  free($5);
		}
		;

programname	: identifier
		{
		  if (strcmp($1, filename) != 0) {
		    semanticError("program beginning ID inconsist with file name");
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
  formal_args RPAREN function_type SEMICOLON { endFuncDecl($6, $<pairName>1.success); }
  compound_stmt END IDENT
  {
    // popScope is done by compound_stmt
    if (strcmp($<pairName>1.name, $11) != 0) {
      semanticError("function end ID inconsist with the beginning ID");
    }
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

function_name : identifier { $<pairName>$ = addSymbol($1, SymbolKind_function); };

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
| PRINT boolean_expr SEMICOLON
| READ variable_reference SEMICOLON
;

variable_reference :
  identifier { free($1); }
| array_reference
;

array_reference :
  identifier { free($1); } LBRACKET boolean_expr RBRACKET
| array_reference LBRACKET boolean_expr RBRACKET
;

minus_expr :
  LPAREN boolean_expr RPAREN
| variable_reference
| function_invoc
;

factor :
  minus_expr
| literal_constant { if ($1.type == Type_STRING) free($1.str); }
| MINUS minus_expr
;

term :
  factor
| term mul_op factor
;

mul_op :
  MULTIPLY
| DIVIDE
| MOD
;

expression :
  term
| expression plus_op term
;

plus_op :
  PLUS
| MINUS
;

relation_expr :
  expression
| relation_expr relop expression
;

relop :
  LESS
| LEQUAL
| EQUAL
| GEQUAL
| GREATER
| NOTEQUAL
;

boolean_factor :
  relation_expr
| NOT boolean_factor
;

boolean_term :
  boolean_factor
| boolean_term AND boolean_factor
;

boolean_expr :
  boolean_term
| boolean_expr OR boolean_term
;

function_invoc : identifier LPAREN arg_list RPAREN
;

arg_list :
  /* no arguments */
| arguments
;

arguments :
  boolean_expr
| arguments COMMA boolean_expr
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

while_stmt :
  WHILE boolean_expr DO
  statements
  END DO
;

for_stmt :
  FOR identifier { $<boolVal>$ = addLoopVar($2); } ASSIGN integer_constant TO integer_constant DO
  statements
  END DO { if ($<boolVal>3) removeLoopVar(); }
;

return_stmt : RETURN boolean_expr SEMICOLON
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
