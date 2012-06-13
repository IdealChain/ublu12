ub2012
======

Übersetzerbau / Compiler construction VU

*SS2012, TU Wien*

Required build tools
--------------------

* gcc
* make
* lex/flex (lexical analyzer)
* yacc (LALR parser generator)
* ox (attribute grammar compiler)
* bfe/iburg (code generator generator)

Tool collection: http://www.complang.tuwien.ac.at/ubvl/tools/

Building
--------

Run `make` to generate the compiler executable `gesamt`.

Testing
-------

* Show generated asm64 code

    ./gesamt -v < func.0 | ./color.py

* Generate `func` executable from `func.0` and `func.c` source code files (`make func`)

    ./gesamt < func.0 | as -o func.1.o
    cc -c -o func.2.o func.c
    cc -o func func.1.o func.2.o