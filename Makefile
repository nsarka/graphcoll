# Simple build
# Requires: flex, bison, gcc (or clang)

all: testbench

#parser.tab.c parser.tab.h: parser.y
#	bison -Wall -Wcounterexamples -d parser.y

#lexer.c: lexer.l parser.tab.h
#	flex -o lexer.c lexer.l

# -lfl
graphcoll: graph.cpp
	mpicxx -o libgraphcoll.so graph.cpp -fPIC -shared

testbench: graphcoll main.cpp
	mpicxx -o testbench main.cpp -L. -lgraphcoll

clean:
	rm -f testbench libgraphcoll.so

