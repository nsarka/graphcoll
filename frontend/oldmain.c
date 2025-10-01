#include <stdio.h>
#include <stdlib.h>

int yyparse(void);

/* Expose the arrays from parser.y */
extern struct {
    char *name;
    int   process;
    size_t size_bytes;
} *sources;
extern size_t nsources;

struct StrList {
    char *name;
    struct StrList *next;
};

extern struct {
    char *name;
    int   process;
    struct StrList *inputs;
} *dests;
extern size_t ndests;

static void print_results(void) {
    printf("=== Source Buffers (%zu) ===\n", nsources);
    for (size_t i = 0; i < nsources; ++i) {
        printf("  %s: process %d, size %zu bytes (%.2f MB)\n",
               sources[i].name, sources[i].process, sources[i].size_bytes,
               sources[i].size_bytes / (1024.0*1024.0));
    }

    printf("\n=== Destination Buffers (%zu) ===\n", ndests);
    for (size_t i = 0; i < ndests; ++i) {
        printf("  %s: process %d, inputs [", dests[i].name, dests[i].process);
        struct StrList *p = dests[i].inputs;
        int first = 1;
        while (p) {
            if (!first) printf(", ");
            printf("%s", p->name);
            first = 0;
            p = p->next;
        }
        printf("]\n");
    }
}

int main(int argc, char **argv) {
    FILE *in = stdin;
    if (argc > 1) {
        in = fopen(argv[1], "r");
        if (!in) {
            perror("fopen");
            return 1;
        }
        /* Flex reads from yyin if set */
        extern FILE *yyin;
        yyin = in;
    }

    if (yyparse() == 0) {
        print_results();
    } else {
        fprintf(stderr, "Failed to parse input.\n");
        return 1;
    }

    if (in != stdin) fclose(in);
    return 0;
}


