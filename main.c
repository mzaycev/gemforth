#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <conio.h>
#include <time.h>

#include "forth.h"


enum app_codes {
	BYE = APP_PRIM(0),
	DOT,
	DOTX,
	EMIT,
	PRINT,
	KEY,
	CLOCK,
	DOTQUOTE,
	
	APP_PRIM_MAX
};


void app_primitives(int prim)
{
	switch (prim) {
		case BYE:
			exit(EXIT_SUCCESS);
			break;
		case DOT:
			printf("%d ", fth_pop());
			break;
		case DOTX:
			printf("%X ", fth_pop());
			break;
		case EMIT:
			putchar(fth_pop());
			break;
		case PRINT:
			printf("%s", fth_area(fth_pop(), 1));
			break;
		case KEY:
			fth_push(getch());
			break;
		case CLOCK:
			fth_push(clock());
			break;
#ifndef FORTH_ONLY_VM
		case DOTQUOTE:
			fth_execute("\"");
			fth_interpret("PRINT");
			break;
#endif
		
		default:
			fth_error("invalid opcode: %d", prim);
			break;
	}
}


#ifndef FORTH_ONLY_VM
primitive_word_t app_words[] = {
	{"BYE",			BYE,		0},
	{".",			DOT,		0},
	{".X",			DOTX,		0},
	{"EMIT",		EMIT,		0},
	{"PRINT",		PRINT,		0},
	{"KEY",			KEY,		0},
	{"CLOCK",		CLOCK,		0},
	{".\"",			DOTQUOTE,	1},
	
	{NULL,			0,		0}
};
#endif


int main(int argc, char *argv[])
{
	char tib[256];
	jmp_buf err;
	
	fth_init(app_primitives, &err);
#ifndef FORTH_ONLY_VM
	fth_library(app_words);
	
	while (fgets(tib, 256, stdin))
		if (strlen(tib) > 1) {
			if (setjmp(err) == 0) {
				fth_interpret(tib);
				if (!fth_getstate())
					printf(" OK\n");
			} else {
				int lineno, linelen, intp, i;
				const char *errline;
				
				fprintf(stderr, "Error: %s\n", fth_geterror());
				errline = fth_geterrorline(&linelen, &intp, &lineno);
				fprintf(stderr, "<stdin>:%d\n", lineno);
				fprintf(stderr, "%.*s\n", linelen, errline);
				fprintf(stderr, "%*s\n", intp + 1, "^");
				
				if (fth_gettracedepth() > 0) {
					fprintf(stderr, "Traceback:\n");
					for (i = fth_gettracedepth() - 1; i >= 0; i--) {
						fprintf(stderr, "\t%s\n", fth_gettrace(i));
					}
				}
				
				fprintf(stderr, "Stack: ");
				for (i = 0; i < fth_getdepth(); i++)
					fprintf(stderr, "%d ", fth_getstack(i));
				if (i == 0)
					fprintf(stderr, "empty");
				fprintf(stderr, "\n");
				
				fth_reset();
			}
		}
#else
	if (argc < 2) {
		fprintf(stderr, "Usage: %s PROGRAM.BIN\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	if (setjmp(err) == 0) {
		fth_runprogram(argv[1]);
	} else {
		int i;
		
		fprintf(stderr, "%s\n", fth_geterror());
				
		fprintf(stderr, "Stack: ");
		for (i = 0; i < fth_getdepth(); i++)
			fprintf(stderr, "%d ", fth_getstack(i));
		fprintf(stderr, "\n");
	}
#endif
	
	return 0;
}