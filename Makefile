# comp Makefile
# Daniel Achleitner (0926807)

MODE = debug

ifeq ($(MODE), debug)
	DEBUG    = -g
else
	DEBUG    = -O3
	CPPFLAGS = -DNDEBUG
endif

TARGET = ag

CFLAGS  += $(DEBUG) -Wall 
LDFLAGS += $(DEBUG)

all: $(TARGET)

oxout.y oxout.l: parser.y scanner.l
	ox $^

lex.yy.c: oxout.l y.tab.h common.h symtable.h
	$(LEX) $<

lex.yy.o: lex.yy.c y.tab.h
	$(CC) $(CFLAGS) -c $<

y.tab.c y.tab.h: oxout.y
	$(YACC) -d --verbose $<

y.tab.o: y.tab.c common.h symtable.h
	$(CC) $(CFLAGS) -c $<

symtable.o: symtable.c symtable.h
	$(CC) $(CFLAGS) -c $<

common.o: common.c common.h
	$(CC) $(CFLAGS) -c $<

$(TARGET): y.tab.o lex.yy.o symtable.o common.o
	$(CC) $(CFLAGS) -o $(TARGET) $^

test: ag
	/usr/ftp/pub/ubvl/test/ag/test

.PHONY: clean
clean:
	$(RM) $(TARGET) *.o *.output oxout* y.tab.* lex.yy.* 
