#ifndef _FORTH_H_
#define _FORTH_H_

#ifdef __cplusplus
extern "C" {
#endif

// Settings
// Uncomment to disable Forth system saving and loading capabilities via stdio
// #define FORTH_NO_SAVES	1

#define STACK_SIZE		32
#define RSTACK_SIZE		32
#define LSTACK_SIZE		16
#define CFSTACK_SIZE		16
#define CODE_INITIAL_SIZE	256		// cells
#define DATA_INITIAL_SIZE	1024		// bytes
#define DICT_INITIAL_SIZE	1024		// words
#define NAMES_INITIAL_SIZE	4096		// bytes
#define WORD_MAX	32			// bytes


// Includes
#include <setjmp.h>


// Macros
#define CORE_PRIM_FIRST	1000
#define ERROR_MAX	256
#define FORTH_BOOL(x)	((x) ? ~0 : 0)


// Types
typedef struct primitive_word {
	const char *name;
	int code;
	int immediate;
} primitive_word_t;

typedef struct word {
	int link;
	int xt;
	int name;
	char flags;
} word_t;

enum cftype {
	CFIF,
	CFELSE,
	CFTHEN,
	CFBEGIN,
	CFWHILE,
	CFDO,
	CFLOOP
};

typedef void (*primitives_f)(int prim);
typedef int (*notfound_f)(const char *word);

typedef struct forth {
	// data stack
	int stack[STACK_SIZE];
	int sp;

	// return stack
	struct {
		int ip;
		int xt;
	} rstack[RSTACK_SIZE];
	int rsp;

	// loop stack
	struct {
		int index, limit;
		int leave;
		int xt;
	} lstack[LSTACK_SIZE];
	int lsp;

	// control flow stack
	struct {
		enum cftype type;
		int ref;
	} cfstack[CFSTACK_SIZE];
	int cfsp;

	// error handling
	char errormsg[ERROR_MAX];
	jmp_buf errjmp;
	int errhandlers;

	// app-specific primitives handler
	primitives_f app_prims;
	// app-specific word parsing
	notfound_f app_notfound;

	// code area
	int *code;
	int cp, codecap;

	// data area
	char *data;
	int dp, datacap;

	// dictionary area
	word_t *dict;
	int dictp, dictcap;
	int context, current, forth_voc;

	// names area
	char *names;
	int namesp, namescap;

	// state
	int ip;
	int running;
	int state;
	const char *source;
	int intp;
	char word[WORD_MAX];

	// core xt
	int lit_xt, exit_xt, branch_xt, qbranch_xt, dodo_xt, doqdo_xt, doloop_xt, doaddloop_xt, codecomma_xt, store_xt, dotry_xt;
} forth_t;


// Data
extern forth_t forth;


// API

void fth_error(const char *fmt, ...);
void fth_push(int x);
int fth_pop(void);
int fth_fetch(int a);
void fth_store(int a, int x);
char fth_cfetch(int a);
void fth_cstore(int a, char x);
char *fth_area(int a, int size);

void fth_init(primitives_f app_primitives, notfound_f app_notfnd);
void fth_free(void);
int fth_interpret(const char *s);
int fth_execute(const char *w);
void fth_primitive(const char *name, int code, int immediate);
void fth_library(primitive_word_t *lib);

void fth_reset(void);
const char *fth_geterror(void);
int fth_getdepth(void);
int fth_getstack(int idx);
int fth_getstate(void);
const char *fth_geterrorline(int *plen, int *pintp, int *plineno);
int fth_gettracedepth(void);
const char *fth_gettrace(int idx);

#ifndef FORTH_NO_SAVES
void fth_savesystem(const char *fname);
void fth_loadsystem(const char *fname);

void fth_saveprogram(const char *fname, const char *entry);

int fth_runprogram(const char *fname);

void fth_savedata(const char *fname);
void fth_loaddata(const char *fname);
#endif


#ifdef __cplusplus
}
#endif


#endif


