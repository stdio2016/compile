ALL = parser
OBJS = y.tab.o lex.yy.o MyHash.o symtable.o ast.o errReport.o semcheck.o
LIBS = -lfl
CC = gcc
YACC = yacc
LEX = lex

all: $(ALL)
parser: $(OBJS)
	$(CC) $+ -o $@ $(LIBS)

y.tab.c y.tab.h: parser.y
	$(YACC) -d $<
lex.yy.c: tokens.l
	$(LEX) $<
y.tab.o: y.tab.c symtable.h ast.h errReport.h semcheck.h
lex.yy.o: lex.yy.c y.tab.h ast.h

MyHash.o: MyHash.c MyHash.h
symtable.o: symtable.c symtable.h MyHash.h ast.h errReport.h
ast.o: ast.c ast.h
errReport.o: errReport.c errReport.h
semcheck.o: semcheck.c semcheck.h symtable.h ast.h errReport.h

.PHONY: clean
clean:
	-rm $(OBJS) $(ALL)
	-rm y.tab.c y.tab.h lex.yy.c
