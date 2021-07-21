#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

#include "forth.h"


// ================================ Macros ====================================

// aliases
#define push(x)		fth_push(x)
#define pop()		fth_pop()
#define fetch(a)	fth_fetch(a)
#define store(a, x)	fth_store((a), (x))
#define cfetch(a)	fth_cfetch(a)
#define cstore(a, c)	fth_cstore((a), (c))
#define reset()		fth_reset()

// error handling
#define error(...)	fth_error(__VA_ARGS__)
#define check(cond, ...) \
			if (cond) error(__VA_ARGS__)
#define invaliddataaddr(a) \
			((a) <= 0 || (a) >= DATA_SIZE)
#define checkdata(a, s)	check(invaliddataaddr(a) || invaliddataaddr((a) + (s)), "invalid data area %d (%d bytes)", (a), (s))
#define checkcode(a)	check((a) <= 0 || (a) >= CODE_SIZE, "invalid code address %d", (a))

// parsing
#ifndef FORTH_ONLY_VM
#  define SOURCELEFT	((intp) < strlen(source))
#  define CURCHAR	(source[(intp)])
#  define ISSEP(sep, c)	(((sep) == (c)) || ((sep) == ' ' && strchr(" \t\n\r", (c))))
#endif

// dictionary

// flags
#ifndef FORTH_ONLY_VM
#  define FLAG(x)	(1 << (x))
#  define IMMEDIATE	FLAG(0)
#  define SMUDGED	FLAG(1)
#  define SET(x, flag)	((x) |= (flag))
#  define CLR(x, flag)	((x) &= ~(flag))
#  define ISSET(x, flag) \
			(((x) & (flag)) != 0)
#endif

// save file marks
#define SYSTEM_MARK	'S'
#define PROGRAM_MARK	'P'
#define DATA_MARK	'D'


// ================================= Types ====================================

#ifndef FORTH_ONLY_VM
typedef struct word {
	int link;
	int xt;
	int name;
	char flags;
} word_t;
#endif


// ================================== Data ====================================

enum core_codes {
	// control flow
	LIT = 0,
	ENTER,
	EXIT,
	BRANCH,
	QBRANCH,
	DODO,
	DOQDO,
	DOLOOP,
	DOADDLOOP,
	DO,
	QDO,
	LOOP,
	ADDLOOP,
	IF,
	ELSE,
	THEN,
	BEGIN,
	UNTIL,
	AGAIN,
	WHILE,
	REPEAT,
	LEAVE,
	I,
	J,
	COLON,
	SEMICOLON,
	EXECUTE,
	DOTRY,
	TRY,
	ERROR,
	
	// arithmetic
	ADD,
	SUB,
	MUL,
	DIV,
	MOD,
	DIVMOD,
	NEGATE,
	ONEADD,
	ONESUB,
	CELL,
	CELLS,
	CELLADD,
	CELLSUB,
	MIN,
	MAX,
	ABS,
	
	// stack
	SWAP,
	DUP,
	DROP,
	ROT,
	MROT,
	TUCK,
	OVER,
	NIP,
	DDUP,
	DDROP,
	QDUP,
	
	// logic
	AND,
	OR,
	NOT,
	XOR,
	LESS,
	LESSEQUAL,
	GREATER,
	GREATEREQUAL,
	EQUAL,
	NOTEQUAL,
	ZEROLESS,
	ZEROGREATER,
	ZEROEQUAL,
	ZERONOTEQUAL,
	FALSE,
	TRUE,
	WITHIN,
	BETWEEN,
	
	// data
	DOCONSTANT,
	DOVARIABLE,
	CONSTANT,
	VARIABLE,
	DODOES,
	FETCH,
	STORE,
	CFETCH,
	CSTORE,
	COMMA,
	CCOMMA,
	CREATE,
	DOES,
	ADDSTORE,
	DOVALUE,
	VALUE,
	TO,
	HERE,
	ALLOT,
	TODATA,
	MOVE,
	FILL,
	ERASE,
	
	// compilation
	CODECOMMA,
	COMPILE,
	COMPILENOW,
	TICK,
	TICKNOW,
	MAKEIMMEDIATE,
	STATE,
	COMPON,
	COMPOFF,
	BLOCKSTART,
	BLOCKEND,
	LENCODE,
	LENDICT,
	LENNAMES,
	
	// parsing, strings and tools
	BLOCKCOMMENT,
	LINECOMMENT,
	CHAR,
	QUOTE,
	DEPTH,
	LENGTH,
	COUNT,
	BL,
	STRING,
	WORD,
	VOCABULARY,
	DOVOCABULARY,
	DEFINITIONS,
	SAVE,
	LOAD,
	SAVEPROGRAM,
	SAVEDATA,
	LOADDATA,
	
	NUM_CORE_PRIM
};


#ifndef FORTH_ONLY_VM
primitive_word_t core_words[] = {
	// control flow
	{"EXIT",		EXIT,			0},
	{"DO",			DO,			1},
	{"?DO",			QDO,			1},
	{"LOOP",		LOOP,			1},
	{"+LOOP",		ADDLOOP,		1},
	{"IF",			IF,			1},
	{"ELSE",		ELSE,			1},
	{"THEN",		THEN,			1},
	{"BEGIN",		BEGIN,			1},
	{"UNTIL",		UNTIL,			1},
	{"AGAIN",		AGAIN,			1},
	{"WHILE",		WHILE,			1},
	{"REPEAT",		REPEAT,			1},
	{"LEAVE",		LEAVE,			0},
	{"I",			I,			0},
	{"J",			J,			0},
	{":",			COLON,			0},
	{";",			SEMICOLON,		1},
	{"EXECUTE",		EXECUTE,		0},
	{"TRY",			TRY,			1},
	{"ERROR",		ERROR,			0},
	
	// arithmetic
	{"+",			ADD,			0},
	{"-",			SUB,			0},
	{"*",			MUL,			0},
	{"/",			DIV,			0},
	{"MOD",			MOD,			0},
	{"/MOD",		DIVMOD,			0},
	{"NEGATE",		NEGATE,			0},
	{"1+",			ONEADD,			0},
	{"1-",			ONESUB,			0},
	{"CELL",		CELL,			0},
	{"CELLS",		CELLS,			0},
	{"CELL+",		CELLADD,		0},
	{"CELL-",		CELLSUB,		0},
	{"MIN",			MIN,			0},
	{"MAX",			MAX,			0},
	{"ABS",			ABS,			0},
	
	// stack
	{"SWAP",		SWAP,			0},
	{"DUP",			DUP,			0},
	{"DROP",		DROP,			0},
	{"ROT",			ROT,			0},
	{"-ROT",		MROT,			0},
	{"TUCK",		TUCK,			0},
	{"OVER",		OVER,			0},
	{"NIP",			NIP,			0},
	{"2DUP",		DDUP,			0},
	{"2DROP",		DDROP,			0},
	{"?DUP",		QDUP,			0},
	
	// logic
	{"AND",			AND,			0},
	{"OR",			OR,			0},
	{"NOT",			NOT,			0},
	{"XOR",			XOR,			0},
	{"<",			LESS,			0},
	{"<=",			LESSEQUAL,		0},
	{">",			GREATER,		0},
	{">=",			GREATEREQUAL,		0},
	{"=",			EQUAL,			0},
	{"<>",			NOTEQUAL,		0},
	{"0<",			ZEROLESS,		0},
	{"0>",			ZEROGREATER,		0},
	{"0=",			ZEROEQUAL,		0},
	{"0<>",			ZERONOTEQUAL,		0},
	{"FALSE",		FALSE,			0},
	{"TRUE",		TRUE,			0},
	{"WITHIN",		WITHIN,			0},
	{"BETWEEN",		BETWEEN,		0},
	
	// data
	{"CONSTANT",		CONSTANT,		0},
	{"VARIABLE",		VARIABLE,		0},
	{"@",			FETCH,			0},
	{"!",			STORE,			0},
	{"C@",			CFETCH,			0},
	{"C!",			CSTORE,			0},
	{",",			COMMA,			0},
	{"C,",			CCOMMA,			0},
	{"CREATE",		CREATE,			0},
	{"DOES>",		DOES,			0},
	{"+!",			ADDSTORE,		0},
	{"VALUE",		VALUE,			0},
	{"TO",			TO,			1},
	{"HERE",		HERE,			0},
	{"ALLOT",		ALLOT,			0},
	{">DATA",		TODATA,			0},
	{"MOVE",		MOVE,			0},
	{"FILL",		FILL,			0},
	{"ERASE",		ERASE,			0},
	
	// compilation
	{"CODE,",		CODECOMMA,		0},
	{"COMPILE",		COMPILE,		1},
	{"[COMPILE]",		COMPILENOW,		1},
	{"'",			TICK,			0},
	{"[']",			TICKNOW,		1},
	{"IMMEDIATE",		MAKEIMMEDIATE,		0},
	{"STATE",		STATE,			0},
	{"]",			COMPON,			0},
	{"[",			COMPOFF,		1},
	{"{",			BLOCKSTART,		0},
	{"}",			BLOCKEND,		1},
	{"#CODE",		LENCODE,		0},
	{"#DICT",		LENDICT,		0},
	{"#NAMES",		LENNAMES,		0},
	
	// parsing, strings and tools
	{"(",			BLOCKCOMMENT,		1},
	{"\\",			LINECOMMENT,		1},
	{"CHAR",		CHAR,			1},
	{"\"",			QUOTE,			1},
	{"DEPTH",		DEPTH,			0},
	{"LENGTH",		LENGTH,			0},
	{"COUNT",		COUNT,			0},
	{"BL",			BL,			0},
	{"STRING",		STRING,			0},
	{"WORD",		WORD,			0},
	{"VOCABULARY",		VOCABULARY,		0},
	{"DEFINITIONS",		DEFINITIONS,		0},
#  ifndef FORTH_NO_SAVES
	{"SAVE",		SAVE,			0},
	{"LOAD",		LOAD,			0},
	{"SAVE-PROGRAM",	SAVEPROGRAM,		0},
	{"SAVE-DATA",		SAVEDATA,		0},
	{"LOAD-DATA",		LOADDATA,		0},
#  endif
	
	{NULL,			0,			0}
};
#endif


// ============================== Forth state =================================

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
	enum cftype {
		CFIF,
		CFELSE,
		CFTHEN,
		CFBEGIN,
		CFWHILE,
		CFDO,
		CFLOOP
	} type;
	int ref;
} cfstack[CFSTACK_SIZE];
int cfsp;

// error handling
char errormsg[ERROR_MAX];
jmp_buf *errjmp;

// app-specific primitives handler
primitives_f app_prims;

// code area
int code[CODE_SIZE];
int cp;

// data area
char data[DATA_SIZE + 1];	// with trailing 0
int dp;

#ifndef FORTH_ONLY_VM
// dictionary area
word_t dict[DICT_SIZE];
int dictp;
int context, current, forth_voc;

// names area
char names[NAMES_SIZE];
int namesp;
#endif

// state
int ip;
int running;
#ifndef FORTH_ONLY_VM
int state;
const char *source;
int intp;
char word[WORD_MAX];
#endif

// core xt
int lit_xt, exit_xt, branch_xt, qbranch_xt, dodo_xt, doqdo_xt, doloop_xt, doaddloop_xt, codecomma_xt, store_xt, dotry_xt;


// ============================== Prototypes ==================================

static void core_prims(int prim, int pfa);


// =============================== Functions ==================================

static int endian(void)
{
	int x = 1;
	return *(char *)&x == 1 ? -1 : 1;
}

static void rpush(void)
{
	check(rsp >= RSTACK_SIZE, "return stack overflow");
	rstack[rsp].ip = ip;
	rstack[rsp].xt = running;
	rsp++;
}


static void rpop(void)
{
	check(rsp <= 0, "return stack underflow");
	--rsp;
	ip = rstack[rsp].ip;
	running = rstack[rsp].xt;
}


static void lpush(int index, int limit, int leave)
{
	check(lsp >= LSTACK_SIZE, "loop stack overflow");
	lstack[lsp].index = index;
	lstack[lsp].limit = limit;
	lstack[lsp].leave = leave;
	lstack[lsp].xt = running;
	lsp++;
}


static void lpop(void)
{
	check(lsp <= 0, "loop stack underflow");
	lsp--;
}


static void cfpush(enum cftype type, int ref)
{
	check(cfsp >= CFSTACK_SIZE, "too nested control structures");
	cfstack[cfsp].type = type;
	cfstack[cfsp].ref = ref;
	cfsp++;
}


static int cfpop(enum cftype required)
{
	check(cfsp <= 0, "unbalanced control structure");
	cfsp--;
	check(cfstack[cfsp].type != required, "unbalanced control structure");
	return cfstack[cfsp].ref;
}


static enum cftype cfpeek(void)
{
	check(cfsp <= 0, "unbalanced control structure");
	return cfstack[cfsp - 1].type;
}


static void compile(int x)
{
	check(cp >= CODE_SIZE, "code area overflow");
	code[cp++] = x;
}


static void dcompile(int x)
{
	checkdata(dp, sizeof(int));
	*(int *)&data[dp] = x;
	dp += sizeof(int);
}


static void ccompile(int c)
{
	checkdata(dp, 1);
	data[dp++] = c;
}


static void scompile(const char *s, int size)
{
	int escape = 0;
	
	while (size) {
		switch (*s) {
			case '\\':
				if (escape) {
					ccompile(*s);
					escape = 0;
				} else {
					escape = 1;
				}
				break;
			case 'n':
				if (escape) {
					ccompile('\n');
					escape = 0;
				} else {
					ccompile(*s);
				}
				break;
			case 't':
				if (escape) {
					ccompile('\t');
					escape = 0;
				} else {
					ccompile(*s);
				}
				break;
			case 'r':
				if (escape) {
					ccompile('\r');
					escape = 0;
				} else {
					ccompile(*s);
				}
				break;
			case 'b':
				if (escape) {
					ccompile('\b');
					escape = 0;
				} else {
					ccompile(*s);
				}
				break;
			default:
				ccompile(*s);
				escape = 0;
				break;
		}
		s++, size--;
	}
	ccompile('\0');
}


static void execute(int xt)
{
	int prim = code[xt];
	int orsp = rsp;
	
	core_prims(prim, xt + 1);
	
	while (orsp < rsp) {
		xt = code[ip++];
		prim = code[xt];
		core_prims(prim, xt + 1);
	}
}


#ifndef FORTH_ONLY_VM
static void create(const char *name, int flags, int prim)
{
	int name_size = strlen(name) + 1;
	
	check(dictp >= DICT_SIZE - 1,
		"dictionary overflow while creating %s", name);
	check(namesp + name_size >= NAMES_SIZE,
		"name area overflow while creating %s", name);
	dict[dictp].link = code[current];
	code[current] = dictp;
	dict[dictp].flags = flags;
	dict[dictp].xt = cp;
	compile(prim);
	dict[dictp].name = namesp;
	strcpy(&names[namesp], name);
	namesp += name_size;
	dictp++;
}


static int getword(char sep)
{
	char *wordp = word;
	
	while (SOURCELEFT && ISSEP(sep, CURCHAR))
		intp++;
	
	if (!SOURCELEFT)
		return 0;
	
	while (SOURCELEFT && !ISSEP(sep, CURCHAR)) {
		if (wordp < word + WORD_MAX - 1)
			*wordp++ = CURCHAR;
		intp++;
	}
	
	*wordp = '\0';
	
	if (SOURCELEFT)
		intp++;
	
	return 1;
}


static int parse(char sep, int *pstart, int *plength)
{
	int start = intp, length = 0;
	int escape = 0;
	
	if (!SOURCELEFT)
		return 0;
	
	while (SOURCELEFT) {
		if (CURCHAR == '\\') {
			intp++, length++;
			escape = !escape;
		} else if (ISSEP(sep, CURCHAR)) {
			if (escape) {
				intp++, length++;
				escape = 0;
			} else {
				break;
			}
		} else {
			intp++, length++;
			escape = 0;
		}
	}
	
	if (!SOURCELEFT)
		return 0;
	
	intp++;
		
	if (pstart)
		*pstart = start;
	if (plength)
		*plength = length;
	
	return 1;
}


static word_t *find(const char *word)
{
	int p;		// dict address of next word to check
	int voc = context;	// pfa of vocabulary
	
	do {
		for (p = code[voc]; p; p = dict[p].link)
			if (!ISSET(dict[p].flags, SMUDGED) && strcasecmp(&names[dict[p].name], word) == 0)
				return &dict[p];
		voc = code[voc + 1];
	} while (voc);
	
	/* Uncomment to enable CURRENT-vocabulary search
	if (context == current)
		return NULL;
	
	voc = current;
	do {
		for (p = code[voc]; p; p = dict[p].link)
			if (!ISSET(dict[p].flags, SMUDGED) && strcasecmp(&names[dict[p].name], word) == 0)
				return &dict[p];
		voc = code[voc + 1];
	} while (voc);
	//*/
	
	return NULL;
}


static int toliteral(int *n)
{
	int ret, read;
	
	if (sscanf(word, "%d%n", &ret, &read) == 1 && read == strlen(word)) {
		if (n)
			*n = ret;
		return 1;
	}
	
	if (sscanf(word, "0%*[Xx]%x%n", &ret, &read) == 1 && read == strlen(word)) {
		if (n)
			*n = ret;
		return 1;
	}
	
	return 0;
}


static void do_interpret(void)
{
	int n;
	word_t *w;
	
	while (SOURCELEFT) {
		if (!getword(' '))
			break;
		w = find(word);
		if (w) {
			if (state == 0 || ISSET(w->flags, IMMEDIATE))
				execute(w->xt);
			else
				compile(w->xt);
		} else {
			if (toliteral(&n)) {
				if (state) {
					compile(lit_xt);
					compile(n);
				} else {
					push(n);
				}
			} else {
				error("%s ?", word);
			}
		}
	}
}
#endif		/* #ifndef FORTH_ONLY_VM */

#ifndef FORTH_NO_SAVES
static void saveprogram(const char *fname, int entry)
{
	char sig[4] = {PROGRAM_MARK, endian(), sizeof(int), 0};
	FILE *f;
	
	f = fopen(fname, "wb");
	check(!f, "save error: %s", strerror(errno));
	
	check(fwrite(sig, 1, 4, f) < 4, "save error: %s", strerror(errno));
	check(fwrite(&entry, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&cp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(code, sizeof(int), cp, f) < cp, "save error: %s", strerror(errno));
	check(fwrite(&dp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(data, 1, dp, f) < dp, "save error: %s", strerror(errno));
	
	check(fwrite(&lit_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&exit_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&branch_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&qbranch_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&dodo_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&doqdo_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&doloop_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&doaddloop_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&codecomma_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&store_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	
	fclose(f);
}
#endif


static void markfwd(enum cftype type)
{
	cfpush(type, cp);
	compile(0);
}


static void resolvefwd(enum cftype required)
{
	code[cfpop(required)] = cp;
}


static void markback(enum cftype type)
{
	cfpush(type, cp);
}


static void resolveback(enum cftype required)
{
	compile(cfpop(required));
}


static void core_prims(int prim, int pfa)
{
	switch (prim) {
		// control flow
		case LIT:
			push(code[ip++]);
			break;
		case ENTER:
			rpush();
			running = pfa - 1;
			ip = pfa;
			break;
		case EXIT:
			while (lsp > 0 && lstack[lsp - 1].xt == running)
				lpop();
			rpop();
			break;
		case BRANCH:
			ip = code[ip];
			break;
		case QBRANCH:
			if (pop())
				ip++;
			else
				ip = code[ip];
			break;
		case DODO: {
			int index = pop(), limit = pop(), leave = code[ip++];
			lpush(index, limit, leave);
			} break;
		case DOQDO: {
			int index = pop(), limit = pop(), leave = code[ip++];
			if (index != limit)
				lpush(index, limit, leave);
			else
				ip = leave;
			} break;
		case DOLOOP:
			check(lsp <= 0, "usage of LOOP outside any loop");
			if (++lstack[lsp - 1].index == lstack[lsp - 1].limit) {
				ip++;
				lpop();
			} else {
				ip = code[ip];
			} break;
		case DOADDLOOP: {
			int step = pop(), index, limit;
			check(lsp <= 0, "usage of +LOOP outside any loop");
			index = lstack[lsp - 1].index;
			limit = lstack[lsp - 1].limit;
			if ((index < limit) == (index + step < limit)) {
				lstack[lsp - 1].index += step;
				ip = code[ip];
			} else {
				ip++;
				lpop();
			}
			} break;
		case DO:
			compile(dodo_xt);
			markfwd(CFDO);
			markback(CFLOOP);
			break;
		case QDO:
			compile(doqdo_xt);
			markfwd(CFDO);
			markback(CFLOOP);
			break;
		case LOOP:
			compile(doloop_xt);
			resolveback(CFLOOP);
			resolvefwd(CFDO);
			break;
		case ADDLOOP:
			compile(doaddloop_xt);
			resolveback(CFLOOP);
			resolvefwd(CFDO);
			break;
		case IF:
			compile(qbranch_xt);
			markfwd(CFIF);
			break;
		case ELSE: {
			int ifbranch;
			check(cfpeek() != CFIF, "unbalanced control structure");
			ifbranch = cfpop(CFIF);
			compile(branch_xt);
			markfwd(CFELSE);
			cfpush(CFIF, ifbranch);
			resolvefwd(CFIF);
			} break;
		case THEN: {
			enum cftype type = cfpeek();
			check(type != CFIF && type != CFELSE, "unbalanced control structure");
			resolvefwd(type);
			} break;
		case BEGIN:
			markback(CFBEGIN);
			break;
		case UNTIL:
			compile(qbranch_xt);
			resolveback(CFBEGIN);
			break;
		case AGAIN:
			compile(branch_xt);
			resolveback(CFBEGIN);
			break;
		case WHILE: {
			int beginbranch = cfpop(CFBEGIN);
			compile(qbranch_xt);
			markfwd(CFWHILE);
			cfpush(CFBEGIN, beginbranch);
			} break;
		case REPEAT:
			compile(branch_xt);
			resolveback(CFBEGIN);
			resolvefwd(CFWHILE);
			break;
		case LEAVE:
			check(lsp <= 0, "attempt to use LEAVE outside any loop");
			check(lstack[lsp - 1].xt != running, "LEAVE called from nested definition");
			ip = lstack[lsp - 1].leave;
			lpop();
			break;
		case I:
			check(lsp <= 0, "attempt to use I outside any loop");
			push(lstack[lsp - 1].index);
			break;
		case J:
			check(lsp <= 1, "attempt to use J without outer loop");
			push(lstack[lsp - 2].index);
			break;
#ifndef FORTH_ONLY_VM
		case COLON:
			check(getword(' ') == 0, "word required for :");
			create(word, SMUDGED, ENTER);
			state = FORTH_BOOL(1);
			break;
		case SEMICOLON:
			check(state == 0, "; is used outside any definition");
			check(cfsp > 0, "unbalanced control structure");
			compile(exit_xt);
			CLR(dict[code[current]].flags, SMUDGED);
			state = 0;
			break;
#endif
		case EXECUTE: {
			int xt = pop();
			checkcode(xt);
			execute(xt);
			} break;
		case DOTRY: {
			jmp_buf ojmp;
			int osp = sp, orsp = rsp, olsp = lsp, oip = ip, orunning = running;
#ifndef FORTH_ONLY_VM
			const char *osource = source;
			int ointp = intp, ostate = state;
#endif
			memcpy(ojmp, *errjmp, sizeof(jmp_buf));
			if (setjmp(*errjmp) == 0) {
				int xt = code[ip++];
				checkcode(xt);
				execute(xt);
				memcpy(*errjmp, ojmp, sizeof(jmp_buf));
				push(~0);
			} else {
				sp = osp, rsp = orsp, lsp = olsp, ip = oip, running = orunning;
				if (running)
					ip++;
#ifndef FORTH_ONLY_VM
				source = osource;
				intp = ointp, state = ostate;
#endif
				memcpy(*errjmp, ojmp, sizeof(jmp_buf));
				push(0);
			}
			} break;
#ifndef FORTH_ONLY_VM
		case TRY: {
			word_t *w;
			check(getword(' ') == 0, "word required for TRY");
			w = find(word);
			check(!w, "%s ?", word);
			
			if (state) {
				compile(dotry_xt);
				compile(w->xt);
			} else {
				jmp_buf ojmp;
				int osp = sp, orsp = rsp, olsp = lsp, oip = ip, orunning = running;
				const char *osource = source;
				int ointp = intp, ostate = state;
				memcpy(ojmp, *errjmp, sizeof(jmp_buf));
				if (setjmp(*errjmp) == 0) {
					checkcode(w->xt);
					execute(w->xt);
					memcpy(*errjmp, ojmp, sizeof(jmp_buf));
					push(~0);
				} else {
					sp = osp, rsp = orsp, lsp = olsp, ip = oip, running = orunning;
					if (running)
						ip++;
					source = osource;
					intp = ointp, state = ostate;
					memcpy(*errjmp, ojmp, sizeof(jmp_buf));
					push(0);
				}
			}
			} break;
#endif
		case ERROR:
			fth_error("%s", fth_area(fth_pop(), 1));
			break;
			
		// arithmetic
		case ADD: {
			int b = pop(), a = pop();
			push(a + b);
			} break;
		case SUB: {
			int b = pop(), a = pop();
			push(a - b);
			} break;
		case MUL: {
			int b = pop(), a = pop();
			push(a * b);
			} break;
		case DIV: {
			int b = pop(), a = pop();
			check(b == 0, "division by zero");
			push(a / b);
			} break;
		case MOD: {
			int b = pop(), a = pop();
			check(b == 0, "division by zero");
			push(a % b);
			} break;
		case DIVMOD: {
			int b = pop(), a = pop();
			check(b == 0, "division by zero");
			push(a % b);
			push(a / b);
			} break;
		case NEGATE:
			push(-pop());
			break;
		case ONEADD:
			push(pop() + 1);
			break;
		case ONESUB:
			push(pop() - 1);
			break;
		case CELL:
			push(sizeof(int));
			break;
		case CELLS:
			push(pop() * sizeof(int));
			break;
		case CELLADD:
			push(pop() + sizeof(int));
			break;
		case CELLSUB:
			push(pop() - sizeof(int));
			break;
		case MIN: {
			int b = pop(), a = pop();
			push(a < b ? a : b);
			} break;
		case MAX: {
			int b = pop(), a = pop();
			push(a > b ? a : b);
			} break;
		case ABS: {
			int a = pop();
			push(a < 0 ? -a : a);
			} break;
		
		// stack
		case SWAP: {
			int b = pop(), a = pop();
			push(b);
			push(a);
			} break;
		case DUP: {
			int a = pop();
			push(a);
			push(a);
			} break;
		case DROP:
			(void)pop();
			break;
		case ROT: {
			int c = pop(), b = pop(), a = pop();
			push(b);
			push(c);
			push(a);
			} break;
		case MROT: {
			int c = pop(), b = pop(), a = pop();
			push(c);
			push(a);
			push(b);
			} break;
		case TUCK: {
			int b = pop(), a = pop();
			push(b);
			push(a);
			push(b);
			} break;
		case OVER: {
			int b = pop(), a = pop();
			push(a);
			push(b);
			push(a);
			} break;
		case NIP: {
			int b = pop();
			(void)pop();
			push(b);
			} break;
		case DDUP: {
			int b = pop(), a = pop();
			push(a);
			push(b);
			push(a);
			push(b);
			} break;
		case DDROP:
			(void)pop();
			(void)pop();
			break;
		case QDUP: {
			int a = pop();
			if (a)
				push(a);
			push(a);
			} break;
		
		// logic
		case AND: {
			int b = pop(), a = pop();
			push(a & b);
			} break;
		case OR: {
			int b = pop(), a = pop();
			push(a | b);
			} break;
		case NOT:
			push(~pop());
			break;
		case XOR: {
			int b = pop(), a = pop();
			push(a ^ b);
			} break;
		case LESS: {
			int b = pop(), a = pop();
			push(FORTH_BOOL(a < b));
			} break;
		case LESSEQUAL: {
			int b = pop(), a = pop();
			push(FORTH_BOOL(a <= b));
			} break;
		case GREATER: {
			int b = pop(), a = pop();
			push(FORTH_BOOL(a > b));
			} break;
		case GREATEREQUAL: {
			int b = pop(), a = pop();
			push(FORTH_BOOL(a >= b));
			} break;
		case EQUAL: {
			int b = pop(), a = pop();
			push(FORTH_BOOL(a == b));
			} break;
		case NOTEQUAL: {
			int b = pop(), a = pop();
			push(FORTH_BOOL(a != b));
			} break;
		case ZEROLESS:
			push(FORTH_BOOL(pop() < 0));
			break;
		case ZEROGREATER:
			push(FORTH_BOOL(pop() > 0));
			break;
		case ZEROEQUAL:
			push(FORTH_BOOL(pop() == 0));
			break;
		case ZERONOTEQUAL:
			push(FORTH_BOOL(pop()));
			break;
		case FALSE:
			push(0);
			break;
		case TRUE:
			push(~0);
			break;
		case WITHIN: {
			int b = pop(), a = pop(), x = pop();
			push(FORTH_BOOL(a <= x && x < b));
			} break;
		case BETWEEN: {
			int b = pop(), a = pop(), x = pop();
			push(FORTH_BOOL(a <= x && x <= b));
			} break;
		
		// data
		case DOCONSTANT:
			push(code[pfa]);
			break;
		case DOVARIABLE:
			push(code[pfa]);		// is variable a constant !?!!
			break;
#ifndef FORTH_ONLY_VM
		case CONSTANT:
			check(getword(' ') == 0, "word required for CONSTANT");
			create(word, 0, DOCONSTANT);
			compile(pop());
			break;
		case VARIABLE:
			check(getword(' ') == 0, "word required for VARIABLE");
			create(word, 0, DOVARIABLE);
			compile(dp);
			compile(0);			// xt of DOES>-part
			dcompile(0);
			break;
#endif
		case DODOES:
			push(code[pfa]);
			rpush();
			running = pfa - 1;
			ip = code[pfa + 1];
			break;
		case FETCH:
			push(fetch(pop()));
			break;
		case STORE: {
			int a = pop(), x = pop();
			store(a, x);
			} break;
		case CFETCH:
			push(cfetch(pop()) & 0xFF);
			break;
		case CSTORE: {
			int a = pop(), x = pop();
			cstore(a, x);
			} break;
		case COMMA:
			dcompile(pop());
			break;
		case CCOMMA:
			ccompile(pop());
			break;
#ifndef FORTH_ONLY_VM
		case CREATE:
			check(getword(' ') == 0, "word required for CREATE");
			create(word, 0, DOVARIABLE);
			compile(dp);
			compile(0);			// xt of DOES>-part
			break;
		case DOES:
			check(code[dict[code[current]].xt] != DOVARIABLE, "%s is not CREATEd", &names[dict[code[current]].name]);
			code[dict[code[current]].xt] = DODOES;
			if (running) {
				code[dict[code[current]].xt + 2] = ip;
				rpop();
			} else {
				code[dict[code[current]].xt + 2] = cp;
				state = FORTH_BOOL(1);
			}
			break;
#endif
		case ADDSTORE: {
			int a = pop(), x = pop();
			checkdata(a, sizeof(int));
			*(int *)&data[a] += x;
			} break;
		case DOVALUE:
			push(fetch(code[pfa]));
			break;
#ifndef FORTH_ONLY_VM
		case VALUE:
			check(getword(' ') == 0, "word required for VALUE");
			create(word, 0, DOVALUE);
			compile(dp);
			dcompile(0);
			break;
		case TO: {
			word_t *w;
			check(getword(' ') == 0, "word required for TO");
			w = find(word);
			check(w == NULL, "%s ?", word);
			check(code[w->xt] != DOVALUE, "%s is not a VALUE", word);
			if (state) {
				compile(lit_xt);
				compile(code[w->xt + 1]);
				compile(store_xt);
			} else {
				store(code[w->xt + 1], pop());
			}
			} break;
#endif
		case HERE:
			push(dp);
			break;
		case ALLOT: {
			int size = pop();
			checkdata(dp, size);
			dp += size;
			} break;
		case TODATA: {
			int xt = pop();
			checkcode(xt);
			check(code[xt] != DOVARIABLE && code[xt] != DODOES, "attempt to get data address of something, that is not CREATEd");
			push(code[xt + 1]);
			} break;
		case MOVE: {
			int size = pop(), b = pop(), a = pop();
			checkdata(a, size);
			checkdata(b, size);
			memmove(&data[b], &data[a], size);
			} break;
		case FILL: {
			int c = pop(), size = pop(), a = pop();
			checkdata(a, size);
			memset(&data[a], c, size);
			} break;
		case ERASE: {
			int size = pop(), a = pop();
			checkdata(a, size);
			memset(&data[a], 0, size);
			} break;
		
		// compilation
		case CODECOMMA:
			compile(pop());
			break;
#ifndef FORTH_ONLY_VM
		case COMPILE: {
			word_t *w;
			check(getword(' ') == 0, "word required for COMPILE");
			w = find(word);
			check(w == NULL, "%s ?", word);
			compile(lit_xt);
			compile(w->xt);
			compile(codecomma_xt);
			} break;
		case COMPILENOW: {
			word_t *w;
			check(getword(' ') == 0, "word required for [COMPILE]");
			w = find(word);
			check(w == NULL, "%s ?", word);
			compile(w->xt);
			} break;
		case TICK: {
			word_t *w;
			check(getword(' ') == 0, "word required for '");
			w = find(word);
			check(w == NULL, "%s ?", word);
			push(w->xt);
			} break;
		case TICKNOW: {
			word_t *w;
			check(getword(' ') == 0, "word required for [']");
			w = find(word);
			check(w == NULL, "%s ?", word);
			compile(lit_xt);
			compile(w->xt);
			} break;
		case MAKEIMMEDIATE:
			SET(dict[code[current]].flags, IMMEDIATE);
			break;
		case STATE:
			push(FORTH_BOOL(state));
			break;
		case COMPON:
			state = ~0;
			break;
		case COMPOFF:
			state = 0;
			break;
		case BLOCKSTART:
			state = ~0;
			push(cp);
			compile(ENTER);
			break;
		case BLOCKEND:
			check(state == 0, "attempt to use } outside any definition");
			check(cfsp > 0, "unbalanced control structure");
			state = 0;
			compile(exit_xt);
			execute(pop());
			break;
#endif
		case LENCODE:
			push(cp);
			break;
#ifndef FORTH_ONLY_VM
		case LENDICT:
			push(dictp);
			break;
		case LENNAMES:
			push(namesp);
			break;
#endif
		
		// parsing, strings and tools
#ifndef FORTH_ONLY_VM
		case BLOCKCOMMENT:
			check(parse(')', NULL, NULL) == 0, "unmatched (");
			break;
		case LINECOMMENT:
			parse('\n', NULL, NULL);
			break;
		case CHAR:
			check(getword(' ') == 0, "word required for CHAR");
			if (state) {
				compile(lit_xt);
				compile(word[0]);
			} else {
				push(word[0]);
			}
			break;
		case QUOTE: {
			int start, length;
			check(parse('"', &start, &length) == 0, "unmatched \"");
			if (state) {
				compile(lit_xt);
				compile(dp);
			} else {
				push(dp);
			}
			scompile(&source[start], length);
			} break;
#endif
		case DEPTH:
			push(sp);
			break;
		case LENGTH: {
			int a = pop();
			checkdata(a, 1);
			push(strlen(&data[a]));
			} break;
		case COUNT: {
			int a = pop();
			checkdata(a, 1);
			push(a);
			push(strlen(&data[a]));
			} break;
		case BL:
			push(' ');
			break;
#ifndef FORTH_ONLY_VM
		case STRING: {
			int sep = pop(), start, length;
			check(!parse(sep, &start, &length), "string separated by `%c' required for STRING", sep);
			push(dp);
			scompile(&source[start], length);
			} break;
		case WORD:
			check(getword(pop()) == 0, "word required for WORD");
			checkdata(dp, strlen(word) + 1);
			strcpy(&data[dp], word);
			push(dp);
			break;
		case VOCABULARY:
			check(getword(' ') == 0, "word required for VOCABULARY");
			create(word, 0, DOVOCABULARY);
			compile(0);			// dict address of latest definition in this voc
			compile(current);		// link to parent voc
			break;
		case DOVOCABULARY:
			context = pfa;
			break;
		case DEFINITIONS:
			current = context;
			break;
#  ifndef FORTH_NO_SAVES
		case SAVE: {
			int a = pop();
			checkdata(a, 1);
			fth_savesystem(&data[a]);
			} break;
		case LOAD: {
			int a = pop();
			checkdata(a, 1);
			fth_loadsystem(&data[a]);
			} break;
#  endif	/* FORTH_NO_SAVES */
#endif		/* FORTH_ONLY_VM */
#ifndef FORTH_NO_SAVES
		case SAVEPROGRAM: {
			int entry = pop();
			int a = pop();
			checkcode(entry);
			checkdata(a, 1);
			saveprogram(&data[a], entry);
			} break;
		case SAVEDATA: {
			int a = pop();
			checkdata(a, 1);
			fth_savedata(&data[a]);
			} break;
		case LOADDATA: {
			int a = pop();
			checkdata(a, 1);
			fth_loaddata(&data[a]);
			} break;
#endif
		
		default:
			app_prims(prim);
			break;
	}
}


// ================================== API =====================================

// ... for application primitives
void fth_error(const char *fmt, ...)
{
	va_list args;
	
	if (fmt != errormsg) {		// for re-throwing error messages
		va_start(args, fmt);
		vsnprintf(errormsg, ERROR_MAX, fmt, args);
		va_end(args);
	}
	
	longjmp(*errjmp, 1);
}


void fth_push(int x)
{
	check(sp >= STACK_SIZE, "stack overflow");
	stack[sp++] = x;
}


int fth_pop(void)
{
	check(sp <= 0, "stack underflow");
	return stack[--sp];
}


int fth_fetch(int a)
{
	checkdata(a, sizeof(int));
	return *(int *)&data[a];
}


void fth_store(int a, int x)
{
	checkdata(a, sizeof(int));
	*(int *)&data[a] = x;
}


char fth_cfetch(int a)
{
	checkdata(a, 1);
	return data[a];
}


void fth_cstore(int a, char x)
{
	checkdata(a, 1);
	data[a] = x;
}


char *fth_area(int a, int size)
{
	checkdata(a, size);
	return &data[a];
}


void fth_init(primitives_f app_primitives, jmp_buf *errhandler)
{
	app_prims = app_primitives;
	errjmp = errhandler;
	data[DATA_SIZE] = '\0';
	reset();
	cp = dp = 1;		// 0 is an "invalid" address
	
	// initial vocabulary
#ifndef FORTH_ONLY_VM
	namesp = 0;
	dictp = 1;
	context = current = 0;
	code[0] = 0;
	create("FORTH", 0, DOVOCABULARY);
	context = current = forth_voc = cp;
	compile(1);
	compile(0);
	
	// core xt-s
	lit_xt = cp;		compile(LIT);
	branch_xt = cp;		compile(BRANCH);
	qbranch_xt = cp;	compile(QBRANCH);
	dodo_xt = cp;		compile(DODO);
	doqdo_xt = cp;		compile(DOQDO);
	doloop_xt = cp;		compile(DOLOOP);
	doaddloop_xt = cp;	compile(DOADDLOOP);
	dotry_xt = cp;		compile(DOTRY);
	
	fth_library(core_words);
	
	exit_xt = find("EXIT")->xt;
	codecomma_xt = find("CODE,")->xt;
	store_xt = find("!")->xt;
#endif
}


#ifndef FORTH_ONLY_VM
void fth_primitive(const char *name, int code, int immediate)
{
	create(name, immediate ? IMMEDIATE : 0, code);
}


void fth_interpret(const char *s)
{
	const char *osource = source;
	int ointp = intp;
	source = s;
	intp = 0;
	do_interpret();
	intp = ointp;
	source = osource;
}


void fth_execute(const char *w)
{
	word_t *pw = find(w);
	check(pw == NULL, "%s ?", w);
	execute(pw->xt);
}


void fth_library(primitive_word_t *lib)
{
	int i;
	
	for (i = 0; lib[i].name; i++)
		fth_primitive(lib[i].name, lib[i].code, lib[i].immediate);
}


int fth_getstate(void)
{
	return state;
}
#endif


void fth_reset(void)
{
	sp = rsp = lsp = cfsp = 0;
	running = 0;
	errormsg[0] = 0;
#ifndef FORTH_ONLY_VM
	state = 0;
	context = current = forth_voc;
#endif
}


const char *fth_geterror(void)
{
	return errormsg;
}


int fth_getdepth(void)
{
	return sp;
}


int fth_getstack(int idx)
{
	if (idx >= 0 && idx < sp)
		return stack[idx];
	else
		return 0;
}


#ifndef FORTH_ONLY_VM
const char *fth_geterrorline(int *plen, int *pintp, int *plineno)
{
	int line = 1, beg = 0, end, i;
	
	if (!source)
		return NULL;
	
	if (intp > 0)
		intp--;
	
	while (intp > 0 && (intp == strlen(source) || ISSEP(' ', CURCHAR)))
		intp--;
	
	for (i = 0; i < intp; i++)
		if (source[i] == '\n') {
			line++;
			beg = i + 1;
		}
	
	for (; i < strlen(source); i++)
		if (source[i] == '\n')
			break;
	end = i;
	
	if (plen)
		*plen = end - beg;
	if (pintp)
		*pintp = intp - beg;
	if (plineno)
		*plineno = line;
	return &source[beg];
}


int fth_gettracedepth(void)
{
	return rsp;
}


const char *fth_gettrace(int idx)
{
	int xt, pw, voc;
	
	if (idx < 0 || idx >= rsp)
		return "<invalid backtrace index>";
	
	if (idx == rsp - 1)
		xt = running;
	else
		xt = rstack[idx + 1].xt;
	
	voc = context;
	do {
		for (pw = code[voc]; pw; pw = dict[pw].link)
			if (dict[pw].xt == xt)
				return &names[dict[pw].name];
		voc = code[voc + 1];
	} while (voc);
	
	return "<unknown>";
}


#  ifndef FORTH_NO_SAVES
void fth_savesystem(const char *fname)
{
	char sig[4] = {SYSTEM_MARK, endian(), sizeof(int), 0};
	FILE *f = fopen(fname, "wb");
	
	check(!f, "save error: %s", strerror(errno));

	check(fwrite(sig, 1, 4, f) < 4, "save error: %s", strerror(errno));
	check(fwrite(&cp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(code, sizeof(int), cp, f) < cp, "save error: %s", strerror(errno));
	check(fwrite(&dp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(data, 1, dp, f) < dp, "save error: %s", strerror(errno));
	check(fwrite(&dictp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(dict, sizeof(word_t), dictp, f) < dictp, "save error: %s", strerror(errno));
	check(fwrite(&namesp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(names, 1, namesp, f) < namesp, "save error: %s", strerror(errno));
	check(fwrite(&forth_voc, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	
	check(fwrite(&lit_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&exit_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&branch_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&qbranch_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&dodo_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&doqdo_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&doloop_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&doaddloop_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&codecomma_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&store_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	
	fclose(f);
}


void fth_loadsystem(const char *fname)
{
	char sig[4];
	FILE *f = fopen(fname, "rb");
	
	check(!f, "load error: %s", strerror(errno));
	
	check(fread(sig, 1, 4, f) < 4, "load error: %s", strerror(errno));
	check(sig[0] != SYSTEM_MARK, "load error: invalid system mark: %c", sig[0]);
	check(sig[1] != endian(), "system is saved for different data endianness: %d (we have %d)", sig[1], endian());
	check(sig[2] != sizeof(int), "system is saved for different cell size: %d (we have %d)", sig[2], sizeof(int));
	check(sig[3] != 0, "signature reserved byte is non-zero");
	
	check(fread(&cp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(cp > CODE_SIZE, "saved code (%d B) is larger than code area (%d B)", cp, CODE_SIZE);
	check(fread(code, sizeof(int), cp, f) < cp, "load error: %s", strerror(errno));
	check(fread(&dp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(dp > DATA_SIZE, "saved data (%d B) is larger than data area (%d B)", dp, DATA_SIZE);
	check(fread(data, 1, dp, f) < dp, "load error: %s", strerror(errno));
	check(fread(&dictp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(dictp > DICT_SIZE, "saved dictionary (%d words) is larger than dictionary area (%d B)", dictp, DICT_SIZE);
	check(fread(dict, sizeof(word_t), dictp, f) < dictp, "load error: %s", strerror(errno));
	check(fread(&namesp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(namesp > NAMES_SIZE, "saved word names (%d B) is larger than names area (%d B)", namesp, NAMES_SIZE);
	check(fread(names, 1, namesp, f) < namesp, "load error: %s", strerror(errno));
	check(fread(&forth_voc, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	
	check(fread(&lit_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&exit_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&branch_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&qbranch_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&dodo_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&doqdo_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&doloop_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&doaddloop_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&codecomma_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&store_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	
	fclose(f);
	fth_reset();
}


void fth_saveprogram(const char *fname, const char *entry)
{
	word_t *w = find(entry);
	
	check(w == NULL, "%s ?", entry);
	saveprogram(fname, w->xt);
}
#  endif	/* FORTH_NO_SAVES */
#endif		/* FORTH_ONLY_VM */


#ifndef FORTH_NO_SAVES
void fth_runprogram(const char *fname)
{
	char sig[4];
	int entry;
	FILE *f = fopen(fname, "rb");
	
	check(!f, "load error: %s", strerror(errno));
	
	check(fread(sig, 1, 4, f) < 4, "load error: %s", strerror(errno));
	check(sig[0] != PROGRAM_MARK, "load error: invalid program mark: %c", sig[0]);
	check(sig[1] != endian(), "program is saved for different data endianness: %d (we have %d)", sig[1], endian());
	check(sig[2] != sizeof(int), "program is saved for different cell size: %d (we have %d)", sig[2], sizeof(int));
	check(sig[3] != 0, "signature reserved byte is non-zero");
	
	check(fread(&entry, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&cp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(cp > CODE_SIZE, "saved code (%d B) is larger than code area (%d B)", cp, CODE_SIZE);
	check(fread(code, sizeof(int), cp, f) < cp, "load error: %s", strerror(errno));
	check(fread(&dp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(dp > DATA_SIZE, "saved data (%d B) is larger than data area (%d B)", dp, DATA_SIZE);
	check(fread(data, 1, dp, f) < dp, "load error: %s", strerror(errno));
	
	check(fread(&lit_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&exit_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&branch_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&qbranch_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&dodo_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&doqdo_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&doloop_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&doaddloop_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&codecomma_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&store_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	
	fclose(f);
	fth_reset();
	
	checkcode(entry);
	execute(entry);
}


void fth_savedata(const char *fname)
{
	char sig[4] = {DATA_MARK, endian(), sizeof(int), 0};
	FILE *f;
	
	f = fopen(fname, "wb");
	check(!f, "save error: %s", strerror(errno));
	
	check(fwrite(sig, 1, 4, f) < 4, "save error: %s", strerror(errno));
	check(fwrite(&dp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(data, 1, dp, f) < dp, "save error: %s", strerror(errno));
	
	fclose(f);
}


void fth_loaddata(const char *fname)
{
	char sig[4];
	FILE *f = fopen(fname, "rb");
	
	check(!f, "load error: %s", strerror(errno));
	
	check(fread(sig, 1, 4, f) < 4, "load error: %s", strerror(errno));
	check(sig[0] != DATA_MARK, "load error: invalid data mark: %c", sig[0]);
	check(sig[1] != endian(), "program is saved for different data endianness: %d (we have %d)", sig[1], endian());
	check(sig[2] != sizeof(int), "program is saved for different cell size: %d (we have %d)", sig[2], sizeof(int));
	check(sig[3] != 0, "signature reserved byte is non-zero");
	
	check(fread(&dp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(dp > DATA_SIZE, "saved data (%d B) is larger than data area (%d B)", dp, DATA_SIZE);
	check(fread(data, 1, dp, f) < dp, "load error: %s", strerror(errno));
	
	fclose(f);
}
#endif






