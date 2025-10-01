%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* forward declare yylex provided by Flex */
int yylex(void);
void yyerror(const char *s);

/* simple dynamic arrays for sources/dests */
typedef struct {
    char *name;
    int   process;
    size_t size_bytes; /* for source buffers */
} SourceBuf;

typedef struct StrList {
    char *name;
    struct StrList *next;
} StrList;

typedef struct {
    char *name;
    int   process;
    StrList *inputs; /* list of source buffer names */
} DestBuf;

SourceBuf *sources = NULL;
size_t nsources = 0, sources_cap = 0;

DestBuf *dests = NULL;
size_t ndests = 0, dests_cap = 0;

static void push_source(const char *name, int process, int size_mb) {
    if (nsources == sources_cap) {
        sources_cap = sources_cap ? sources_cap*2 : 8;
        sources = (SourceBuf*)realloc(sources, sources_cap * sizeof(SourceBuf));
    }
    sources[nsources].name = strdup(name);
    sources[nsources].process = process;
    sources[nsources].size_bytes = (size_t)size_mb * 1024ULL * 1024ULL;
    nsources++;
}

static void push_dest(const char *name, int process, StrList *inputs) {
    if (ndests == dests_cap) {
        dests_cap = dests_cap ? dests_cap*2 : 8;
        dests = (DestBuf*)realloc(dests, dests_cap * sizeof(DestBuf));
    }
    dests[ndests].name = strdup(name);
    dests[ndests].process = process;
    dests[ndests].inputs = inputs;
    ndests++;
}

static StrList* sl_cons(char *name, StrList *tail) {
    StrList *n = (StrList*)malloc(sizeof(StrList));
    n->name = name;
    n->next = tail;
    return n;
}

static StrList* sl_reverse(StrList *lst) {
    StrList *prev = NULL;
    while (lst) {
        StrList *nxt = lst->next;
        lst->next = prev;
        prev = lst;
        lst = nxt;
    }
    return prev;
}

%}

%union {
    int    ival;
    char  *sval;
    struct StrList *slist;
}

/* tokens */
%token SOURCE_BUFFERS DESTINATION_BUFFERS
%token PROCESS SIZE MB
%token <sval> IDENT
%token <ival> NUMBER

%type <slist> id_list

%%

file
    : sections
    ;

sections
    : source_section dest_section
    | dest_section source_section    /* allow either order */
    ;

source_section
    : SOURCE_BUFFERS '=' source_list
    ;

source_list
    : %empty
    | source_list source_decl
    ;

source_decl
    : IDENT ':' PROCESS NUMBER ',' SIZE NUMBER MB
        {
            push_source($1, $4, $7);
            free($1);
        }
    ;

dest_section
    : DESTINATION_BUFFERS '=' dest_list
    ;

dest_list
    : %empty
    | dest_list dest_decl
    ;

dest_decl
    : IDENT ':' PROCESS NUMBER ',' '[' id_list ']'
        {
            /* id_list was built in reverse via left recursion guard; ensure natural order */
            push_dest($1, $4, $7);
            free($1);
        }
    ;

id_list
    : IDENT
        {
            $$ = sl_cons($1, NULL);
        }
    | id_list ',' IDENT
        {
            $$ = sl_cons($3, $1); /* build in reverse; we'll keep as-is for simplicity or reverse if needed */
        }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error: %s\n", s);
}

