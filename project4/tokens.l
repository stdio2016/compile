%{
#include <stdio.h>
#include "ast.h"
#include "y.tab.h"
#define LIST  mystrcat(yytext)
#define token(t)            {LIST; if (Opt_T) printf("<%s>\n", #t);}
#define tokenMultiChar(t)   {LIST; if (Opt_T) printf("<%s>\n", (t));}
#define tokenChar(t)        {LIST; if (Opt_T) printf("<%c>\n", (t));}
#define tokenString(t, s)   {LIST; if (Opt_T) printf("<%s: %s>\n", #t, (s));}
#define tokenError()   {LIST; return ERROR;}
#define MAX_LINE_LENG  256
#define MAX_IDENT_LENG  32

int Opt_S = 1;
int Opt_T = 1;
int Opt_D = 1;
int linenum = 1;
char buf[MAX_LINE_LENG ];
char strlit_buf[ MAX_LINE_LENG ];
char *buf_ptr = buf;
void mystrcat(const char *text);
%}
%x C_COMMENT
float  [0-9]+\.[0-9]+
%%
"," { tokenChar(','); return COMMA; }
";" { tokenChar(';'); return SEMICOLON; }
":" { tokenChar(':'); return COLON; }
"(" { tokenChar('('); return LPAREN; }
")" { tokenChar(')'); return RPAREN; }
"[" { tokenChar('['); return LBRACKET; }
"]" { tokenChar(']'); return RBRACKET; }

"+" { tokenChar('+'); return PLUS; }
"-" { tokenChar('-'); return MINUS; }
"*" { tokenChar('*'); return MULTIPLY; }
"/" { tokenChar('/'); return DIVIDE; }
mod { token(mod); return MOD; }
":="    { tokenMultiChar(":="); return ASSIGN; }
"<"     { tokenChar('<'); return LESS; }
"<="    { tokenMultiChar("<="); return LEQUAL; }
"<>"    { tokenMultiChar("<>"); return NOTEQUAL; }
">="    { tokenMultiChar(">="); return GEQUAL; }
">"     { tokenChar('>'); return GREATER; }
"="     { tokenChar('='); return EQUAL; }
and     { token(and); return AND; }
or      { token(or); return OR; }
not     { token(not); return NOT; }

array   { token(KWarray); return ARRAY; }
begin   { token(KWbegin); return BEGIN_; }
boolean { token(KWboolean); return BOOLEAN; }
def     { token(KWdef); return DEF; }
do      { token(KWdo); return DO; }
else    { token(KWelse); return ELSE; }
end     { token(KWend); return END; }
false   { token(KWfalse); return FALSE; }
for     { token(KWfor); return FOR; }
integer { token(KWinteger); return INTEGER; }
if      { token(KWif); return IF; }
of      { token(KWof); return OF; }
print   { token(KWprint); return PRINT; }
read    { token(KWread); return READ; }
real    { token(KWreal); return REAL; }
string  { token(KWstring); return STRING; }
then    { token(KWthen); return THEN; }
to      { token(KWto); return TO; }
true    { token(KWtrue); return TRUE; }
return  { token(KWreturn); return RETURN; }
var     { token(KWvar); return VAR; }
while   { token(KWwhile); return WHILE; }
[A-Za-z][A-Za-z0-9]* {
  tokenString(id, yytext);
  size_t n = yyleng;
  if (n > MAX_IDENT_LENG)
    n = MAX_IDENT_LENG;
  char *nstr = malloc(n + 1);
  strncpy(nstr, yytext, n);
  nstr[n] = '\0';
  yylval.name = nstr;
  return IDENT;
}

[1-9][0-9]*|0 {
  tokenString(integer, yytext);
  int i;
  int n = 0;
  for (i = 0; i < yyleng; i++) {
    n = n * 10 + (yytext[i] - '0');
  }
  yylval.lit.type = Type_INTEGER;
  yylval.lit.integer = n;
  return INT_LIT;
}
0[0-7]+ {
  tokenString(oct_integer, yytext);
  int i;
  int n = 0;
  for (i = 0; i < yyleng; i++) {
    n = n * 8 + (yytext[i] - '0');
  }
  yylval.lit.type = Type_INTEGER;
  yylval.lit.integer = n;
  return INT_LIT;
}
{float} {
  float num = 0.0;
  tokenString(float, yytext);
  sscanf(yytext, "%f", &num); // use standard C library to read float
  yylval.lit.type = Type_REAL;
  yylval.lit.real = num;
  return REAL_LIT;
}
({float}|[0-9]+)[eE][-+]?[0-9]+ {
  float num = 0.0;
  tokenString(scientific, yytext);
  sscanf(yytext, "%f", &num); // use standard C library to read float
  yylval.lit.type = Type_REAL;
  yylval.lit.real = num;
  return REAL_LIT;
}
\"([^"\n]|\"\")*\" {
  int i, j = 0;
  for (i = 1; yytext[i+1] && i < MAX_LINE_LENG; i++) {
    strlit_buf[j++] = yytext[i];
    if (yytext[i] == '"') {
      i++;
    }
  }
  strlit_buf[j] = '\0';
  tokenString(string, strlit_buf);
  yylval.lit.type = Type_STRING;
  yylval.lit.str = dupstr(strlit_buf);
  return STR_LIT;
}
\"([^"\n]|\"\")* {
  printf("error at line %d: unterminated string literal\n", linenum);
  tokenError();
}

[ \t]+ { LIST; }
<INITIAL,C_COMMENT>\r?\n  {
      if (Opt_S)
        printf("%d: %s\n", linenum, buf);
      linenum++;
      buf[0] = '\0';
      buf_ptr = buf;
    }
<INITIAL><<EOF>>  {
      if (Opt_S && buf_ptr != buf)
        printf("%d: %s\n", linenum, buf);
      return 0;
    }
"/*" { // C style comment
  LIST;
  BEGIN(C_COMMENT);
}
<C_COMMENT>"*/" { // end of C style comment
  LIST;
  BEGIN(INITIAL);
}
<C_COMMENT>. { LIST; }
<C_COMMENT><<EOF>> {
  printf("error at end of file: unterminated C-style comment\n");
  LIST;
  BEGIN(INITIAL);
}
"//&"[A-Z][-+].* { // pseudocomment
  LIST;
  char option = yytext[3];
  char enabled = (yytext[4] == '+');
  switch (option) {
    case 'S': Opt_S = enabled; break;
    case 'T': Opt_T = enabled; break;
    case 'D': Opt_D = enabled; break;
  }
}
"//".* { // C++ style comment
  LIST;
}

.   {
  printf("error at line %d: bad character \"%s\"\n", linenum, yytext);
  tokenError();
}
%%

void mystrcat(const char *text)
{
  while (*text && buf_ptr < buf + MAX_LINE_LENG - 1) {
    *buf_ptr = *text;
    buf_ptr++;
    text++;
  }
  *buf_ptr = '\0';
}

