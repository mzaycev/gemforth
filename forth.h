#ifndef _FORTH_H_
#define _FORTH_H_


#include <setjmp.h>
#include <stdio.h>


// Settings
// Uncomment to build Forth system without text interpreter and dictionary
// #define FORTH_ONLY_VM	1
// Uncomment to disable Forth system saving and loading capabilities
// #define FORTH_NO_SAVES	1
// Do not uncomment both of above at one time

#define STACK_SIZE	32
#define RSTACK_SIZE	32
#define LSTACK_SIZE	16
#define CFSTACK_SIZE	16
#define CODE_SIZE	32768			// cells
#define DATA_SIZE	32768			// bytes
#ifndef FORTH_ONLY_VM
#  define DICT_SIZE	1024			// words
#  define NAMES_SIZE	8192			// bytes
#  define WORD_MAX	32			// bytes
#endif


// Macros
#define CORE_PRIM_MAX	1000
#define ERROR_MAX	256
#define APP_PRIM(x)	(CORE_PRIM_MAX + (x))
#define FORTH_BOOL(x)	((x) ? ~0 : 0)


// Types
#ifndef FORTH_ONLY_VM
typedef struct primitive_word {
	const char *name;
	int code;
	int immediate;
} primitive_word_t;
#endif

typedef void (*primitives_f)(int prim);


// API

void fth_error(const char *fmt, ...);
void fth_push(int x);
int fth_pop(void);
int fth_fetch(int a);
void fth_store(int a, int x);
char fth_cfetch(int a);
void fth_cstore(int a, char x);
char *fth_area(int a, int size);

void fth_init(primitives_f app_primitives, jmp_buf *errhandler);
#ifndef FORTH_ONLY_VM
void fth_interpret(const char *s);
void fth_execute(const char *w);
void fth_primitive(const char *name, int code, int immediate);
void fth_library(primitive_word_t *lib);
#endif

void fth_reset(void);
const char *fth_geterror(void);
int fth_getdepth(void);
int fth_getstack(int idx);
#ifndef FORTH_ONLY_VM
int fth_getstate(void);
const char *fth_geterrorline(int *plen, int *pintp, int *plineno);
int fth_gettracedepth(void);
const char *fth_gettrace(int idx);
#endif

#ifndef FORTH_NO_SAVES
#  ifndef FORTH_ONLY_VM
void fth_savesystem(const char *fname);
void fth_loadsystem(const char *fname);

void fth_saveprogram(const char *fname, const char *entry);
#  endif

void fth_runprogram(const char *fname);

void fth_savedata(const char *fname);
void fth_loaddata(const char *fname);
#endif


#endif


