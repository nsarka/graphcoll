# Simple build
# Requires: flex, bison, gcc (or clang)

all: collparse

parser.tab.c parser.tab.h: parser.y
	bison -Wall -Wcounterexamples -d parser.y

lexer.c: lexer.l parser.tab.h
	flex -o lexer.c lexer.l

collparse: parser.tab.c lexer.c main.c
	$(CC) -o collparse parser.tab.c lexer.c main.c -lfl

clean:
	rm -f collparse parser.tab.* lexer.c

