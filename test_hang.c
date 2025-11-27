/* Minimal mex_main test */
#define MEX_INIT
#define MAX_INCL_VER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prog.h"
#include "max.h"
#include "mex.h"

/* Extern declarations that mex_main uses */
extern int yyparse(void);

int yydebug;
int sdebug = 0;
int mdebug = 0;

int main(int argc, char **argv) {
    fprintf(stderr, "Starting mex test\n");
    
    if (argc < 2) {
        fprintf(stderr, "Usage: test_hang <file>\n");
        return 0;
    }
    
    fprintf(stderr, "Would compile: %s\n", argv[1]);
    return 0;
}
