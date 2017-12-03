ALL = parser
OBJS = y.tab.o lex.yy.o MyHash.o symtable.o ast.o
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
y.tab.o: y.tab.c symtable.h ast.h
lex.yy.o: lex.yy.c y.tab.h ast.h

MyHash.o: MyHash.c MyHash.h
symtable.o: symtable.c symtable.h MyHash.h ast.h
ast.o: ast.c ast.h

.PHONY: clean
clean:
	-rm $(OBJS) $(ALL)
	-rm y.tab.c y.tab.h lex.yy.c
