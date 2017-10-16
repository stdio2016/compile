%{
#define LIST  mystrcat(yytext)
#define token(t)            {LIST; if (Opt_T) printf("<%s>\n", #t);}
#define tokenMultiChar(t)   {LIST; if (Opt_T) printf("<%s>\n", (t));}
#define tokenChar(t)        {LIST; if (Opt_T) printf("<%c>\n", (t));}
#define tokenInteger(t, i)  {LIST; if (Opt_T) printf("<%s: %d>\n", #t, (i));}
#define tokenOctInt(t, o)   {LIST; if (Opt_T) printf("<%s: %s>\n", #t, (o));}
#define tokenFloat(t, f)    {LIST; if (Opt_T) printf("<%s: %s>\n", #t, (f));}
#define tokenString(t, s)   {LIST; if (Opt_T) printf("<%s: %s>\n", #t, (s));}
#define MAX_LINE_LENG  256

int Opt_S = 1;
int Opt_T = 1;
int linenum = 1;
char buf[MAX_LINE_LENG ];
char strlit_buf[ MAX_LINE_LENG ];
char *buf_ptr = buf;
void mystrcat(char *text);
%}
%x C_COMMENT
float  [0-9]+\.[0-9]+
%%
"," { tokenChar(','); }
";" { tokenChar(';'); }
":" { tokenChar(':'); }
"(" { tokenChar('('); }
")" { tokenChar(')'); }
"[" { tokenChar('['); }
"]" { tokenChar(']'); }

"+" { tokenChar('+'); }
"-" { tokenChar('-'); }
"*" { tokenChar('*'); }
"/" { tokenChar('/'); }
mod { token(mod); }
":="    { tokenMultiChar(":="); }
"<"     { tokenChar('<'); }
"<="    { tokenMultiChar("<="); }
"<>"    { tokenMultiChar("<>"); }
">="    { tokenMultiChar(">="); }
">"     { tokenChar('>'); }
"="     { tokenChar('='); }
and     { token(and); }
or      { token(or); }
not     { token(not); }

array   { token(KWarray); }
begin   { token(KWbegin); }
boolean { token(KWboolean); }
def     { token(KWdef); }
do      { token(KWdo); }
else    { token(KWelas); }
end     { token(KWend); }
false   { token(KWfalse); }
for     { token(KWfor); }
integer { token(KWinteger); }
if      { token(KWif); }
of      { token(KWof); }
print   { token(KWprint); }
read    { token(KWread); }
real    { token(KWreal); }
string  { token(KWstring); }
then    { token(KWthen); }
to      { token(KWto); }
true    { token(KWtrue); }
return  { token(KWreturn); }
var     { token(KWvar); }
while   { token(KWwhile); }
[A-Za-z][A-Za-z0-9]* { tokenString(id, yytext); }

[1-9][0-9]*|0 {
  int num = 0, i;
  for (i = 0; yytext[i]; i++) {
    num = num * 10 + (yytext[i] - '0');
  }
  tokenInteger(integer, num);
}
0[0-7]+ {
  tokenOctInt(oct_integer, yytext);
}
0[0-9]+ {
  printf("error at line %d: invalid octal \"%s\"\n", linenum, yytext);
  exit(-100);
}
{float} {
  tokenFloat(float, yytext);
}
({float}|[0-9]+)[eE][-+]?[0-9]+ {
  tokenFloat(scientific, yytext);
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
}
\"([^"\n]|\"\")*/\n {
  printf("error at line %d: unterminated string literal\n", linenum);
  exit(-100);
}

[ \t]+ { LIST; }
<INITIAL,C_COMMENT>\n  {
      if (Opt_S)
        printf("%d: %s\n", linenum, buf);
      linenum++;
      buf[0] = '\0';
      buf_ptr = buf;
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
  exit(-100);
}
%%

void mystrcat(char *text)
{
  while (*text) {
    *buf_ptr = *text;
    buf_ptr++;
    text++;
  }
  *buf_ptr = '\0';
}

int main(int argc, char *argv[])
{
  if ( argc != 2 ) {
    fprintf( stderr, "Usage: ./pscanner [filename]\n" );
    exit(1);
  }

  FILE *f = fopen( argv[1], "r" );
  if ( f == NULL ) {
    fprintf( stderr, "Unable to open \"%s\" ;-( Does this file exist?\n", argv[1] );
    exit(-1);
  }

  yyin = f;
  yylex();
  return 0;
}
