/* scanner, Daniel Achleitner (0926807) */
%option noyywrap

%{	
	static void  error(void);
	static long long to_int(const char* s, int base);
%}

KEY	end|return|goto|if|then|var|not|and
OP	"=<"|[\-\;\(\)\,\:\=\*\+\#]
ID	[a-zA-Z_][a-zA-Z0-9_]*
HEX	[0-9][0-9a-fA-F]*
DEC	&[0-9]+

%%

"(*"([^\*]|\*[^\)])*"*)"	; /* eat comments */

{KEY}	printf("%s\n", yytext);
{OP}	printf("%s\n", yytext);
{DEC}	printf("num %lu\n", to_int(yytext + 1, 10));
{HEX}	printf("num %lu\n", to_int(yytext, 16));
{ID}	printf("ident %s\n", yytext);

[ \t\n]	; /* whitespace be-gone */
.	error(); /* everything else */

%%

int main()
{
	yylex();
}

static long long to_int(const char* s, int base)
{
	return strtoll(s, NULL, base);
}

static void error(void)
{
	fprintf(stderr, "error: line %d: %s\n", yylineno, yytext);
	exit(1);
}
