%{
#define LIST  mystrcat(yytext)
#define token(t)            {LIST; if (Opt_T) printf("<%s>\n", #t);}
#define tokenMultiChar(t)   {LIST; if (Opt_T) printf("<%s>\n", (t));}
#define tokenChar(t)        {LIST; if (Opt_T) printf("<%c>\n", (t));}
#define tokenInteger(t, i)  {LIST; if (Opt_T) printf("<%s: %d>\n", #t, (i));}
#define tokenString(t, s)   {LIST; if (Opt_T) printf("<%s: %s>\n", #t, (s));}
#define MAX_LINE_LENG  256

int Opt_S = 1;
int Opt_T = 1;
int linenum = 1;
char buf[MAX_LINE_LENG ];
char *buf_ptr = buf;
void mystrcat(char *text);
%}
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

\n  {
      if (Opt_S)
        printf("%d: %s\n", linenum, buf);
      linenum++;
      buf[0] = '\0';
      buf_ptr = buf;
    }
.   { printf("\e[31mInvalid character \e[37m\"%s\"\e[31m at line %d\e[0m\n", yytext, linenum); exit(-100); LIST; }
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
