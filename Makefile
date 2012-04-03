# parser Makefile
# Daniel Achleitner (0926807)
LEX=flex

MODE = debug

ifeq ($(MODE), debug)
	DEBUG    = -g
else
	DEBUG    = -O3
	CPPFLAGS = -DNDEBUG
endif

CFLAGS  += $(DEBUG) -Wall 
LDFLAGS += $(DEBUG)

parser: y.tab.o lex.yy.o
	$(CC) $(CFLAGS) -o parser y.tab.o lex.yy.o

y.tab.c y.tab.h: parser.y
	$(YACC) -d --verbose $<

y.tab.o: y.tab.c
	$(CC) $(CFLAGS) -c y.tab.c

lex.yy.o: lex.yy.c y.tab.h 
	$(CC) $(CFLAGS) -c lex.yy.c

lex.yy.c: scanner.l y.tab.h
	$(LEX) scanner.l

test: parser
	/usr/ftp/pub/ubvl/test/parser/test

.PHONY: clean
clean:
	$(RM) *.o *.c *.output y.tab.h parser
