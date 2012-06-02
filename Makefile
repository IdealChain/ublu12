# comp Makefile
# Daniel Achleitner (0926807)

MODE = debug

ifeq ($(MODE), debug)
	DEBUG    = -g
else
	DEBUG    = -O3
	CPPFLAGS = -DNDEBUG
endif

TARGET = codeb

CFLAGS  += $(DEBUG) -Wall 
LDFLAGS += $(DEBUG)

# turn off warnings for ox-generated lexer/parser
LEX_NOWARN = -Wno-unused-variable -Wno-unused-function
PAR_NOWARN = -Wno-unused-variable -Wno-implicit -Wno-format -Wno-parentheses

# not generated and generated C source files
SRCS = common.c symtable.c exprtree.c stmttree.c codegen.c codegen_reg.c codegen_label.c output.c
GENSRCS = lex.yy.c y.tab.c matcher.c

ALLSRCS = $(SRCS) $(GENSRCS)
OBJS    = $(SRCS:%.c=%.o)
ALLOBJS = $(ALLSRCS:%.c=%.o)
DEPS    = $(ALLSRCS:%.c=%.dep)

$(TARGET): $(ALLOBJS)
	$(CC) -o $@ $^

oxout.y oxout.l: parser.y scanner.l
	ox $^

lex.yy.c: oxout.l y.tab.h common.h symtable.h
	$(LEX) $<

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) $(LEX_NOWARN) -c $<

y.tab.c y.tab.h: oxout.y
	$(YACC) -d --verbose $<

y.tab.o: y.tab.c common.h symtable.h
	$(CC) $(CFLAGS) $(PAR_NOWARN) -c $<

matcher.c: matcher.bfe
	bfe matcher.bfe | iburg > matcher.c

%.dep: %.c
	gcc -MM $< > $@
	
-include $(DEPS)

.PHONY: clean test
clean:
	$(RM) $(TARGET) $(GENSRCS) $(DEPS) $(ALLOBJS) *.output oxout.* y.tab.h func func.1.o func.2.o
	
test: $(TARGET)
	/usr/ftp/pub/ubvl/test/$(TARGET)/test

.PRECIOUS: func
func: func.1.o func.2.o
	$(CC) -o $@ $^
	./$@

func.1.o: func.0 $(TARGET)
	./$(TARGET) -v < $<
	./$(TARGET) < $< | as -o $@

func.2.o: func.c
	$(CC) -c -o $@ $<
