%{
#include "y.tab.h"
#define LIST  mystrcat(yytext)
#define token(t)            {LIST; if (Opt_T) printf("<%s>\n", #t);}
#define tokenMultiChar(t)   {LIST; if (Opt_T) printf("<%s>\n", (t));}
#define tokenChar(t)        {LIST; if (Opt_T) printf("<%c>\n", (t));}
#define tokenString(t, s)   {LIST; if (Opt_T) printf("<%s: %s>\n", #t, (s));}
#define tokenError()   {LIST; return ERROR;}
#define MAX_LINE_LENG  256

int Opt_S = 1;
int Opt_T = 1;
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
else    { token(KWelas); return ELSE; }
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
[A-Za-z][A-Za-z0-9]* { tokenString(id, yytext); return IDENT; }

[1-9][0-9]*|0 {
  tokenString(integer, yytext);
  return INT_LIT;
}
0[0-7]+ {
  tokenString(oct_integer, yytext);
  return INT_LIT;
}
{float} {
  tokenString(float, yytext);
  return REAL_LIT;
}
({float}|[0-9]+)[eE][-+]?[0-9]+ {
  tokenString(scientific, yytext);
  return REAL_LIT;
}
\"([^"\n]|\"\")*\" {
  int i, j = 0;
  for (i = 1; yytext[i+1]; i++) {
    strlit_buf[j++] = yytext[i];
    if (yytext[i] == '"') {
      i++;
    }
  }
  strlit_buf[j] = '\0';
  tokenString(string, strlit_buf);
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
  while (*text) {
    *buf_ptr = *text;
    buf_ptr++;
    text++;
  }
  *buf_ptr = '\0';
}

