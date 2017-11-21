ALL = parser
OBJS = y.tab.o lex.yy.o
LIBS = -ll
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
y.tab.o: y.tab.c
lex.yy.o: lex.yy.c y.tab.h

.PHONY: clean
clean:
	-rm $(OBJS) $(ALL)
	-rm y.tab.c y.tab.h lex.yy.c
