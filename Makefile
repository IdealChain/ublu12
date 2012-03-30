# scanner Makefile
# Daniel Achleitner (0926807)
LEX=flex

all: scanner

test: all
	/usr/ftp/pub/ubvl/test/scanner/test

scanner: scanner.o

scanner.o: scanner.l

.PHONY: clean
clean:
	$(RM) scanner *.o
