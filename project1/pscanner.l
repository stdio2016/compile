%{
#define LIST  mystrcat(yytext)
#define token(t)            {LIST; if (Opt_T) printf("<%s>\n", #t);}
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
\n  {
      if (Opt_S)
        printf("%d: %s\n", linenum, buf);
      linenum++;
      buf[0] = '\0';
      buf_ptr = buf;
    }
.   { LIST; }
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
