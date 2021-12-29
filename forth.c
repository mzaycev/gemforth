#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>

#ifndef FORTH_NO_SAVES
#  include <stdio.h>
#endif

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
#define F		forth

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
#  define SOURCELEFT	(F.intp < strlen(F.source))
#  define CURCHAR	(F.source[F.intp])
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



// ================================== Data ====================================

enum core_codes {
	// control flow
	LIT = CORE_PRIM_FIRST,
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

forth_t F;

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
	check(F.rsp >= RSTACK_SIZE, "return stack overflow");
	F.rstack[F.rsp].ip = F.ip;
	F.rstack[F.rsp].xt = F.running;
	F.rsp++;
}


static void rpop(void)
{
	check(F.rsp <= 0, "return stack underflow");
	--F.rsp;
	F.ip = F.rstack[F.rsp].ip;
	F.running = F.rstack[F.rsp].xt;
}


static void lpush(int index, int limit, int leave)
{
	check(F.lsp >= LSTACK_SIZE, "loop stack overflow");
	F.lstack[F.lsp].index = index;
	F.lstack[F.lsp].limit = limit;
	F.lstack[F.lsp].leave = leave;
	F.lstack[F.lsp].xt = F.running;
	F.lsp++;
}


static void lpop(void)
{
	check(F.lsp <= 0, "loop stack underflow");
	F.lsp--;
}


static void cfpush(enum cftype type, int ref)
{
	check(F.cfsp >= CFSTACK_SIZE, "too nested control structures");
	F.cfstack[F.cfsp].type = type;
	F.cfstack[F.cfsp].ref = ref;
	F.cfsp++;
}


static int cfpop(enum cftype required)
{
	check(F.cfsp <= 0, "unbalanced control structure");
	F.cfsp--;
	check(F.cfstack[F.cfsp].type != required, "unbalanced control structure");
	return F.cfstack[F.cfsp].ref;
}


static enum cftype cfpeek(void)
{
	check(F.cfsp <= 0, "unbalanced control structure");
	return F.cfstack[F.cfsp - 1].type;
}


static void compile(int x)
{
	check(F.cp >= CODE_SIZE, "code area overflow");
	F.code[F.cp++] = x;
}


static void dcompile(int x)
{
	checkdata(F.dp, sizeof(int));
	*(int *)&F.data[F.dp] = x;
	F.dp += sizeof(int);
}


static void ccompile(int c)
{
	checkdata(F.dp, 1);
	F.data[F.dp++] = c;
}


#ifndef FORTH_ONLY_VM
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
#endif


static void execute(int xt)
{
	int prim = F.code[xt];
	int orsp = F.rsp;
	
	core_prims(prim, xt + 1);
	
	while (orsp < F.rsp) {
		xt = F.code[F.ip++];
		prim = F.code[xt];
		core_prims(prim, xt + 1);
	}
}


#ifndef FORTH_ONLY_VM
static void create(const char *name, int flags, int prim)
{
	int name_size = strlen(name) + 1;
	
	check(F.dictp >= DICT_SIZE - 1,
		"dictionary overflow while creating %s", name);
	check(F.namesp + name_size >= NAMES_SIZE,
		"name area overflow while creating %s", name);
	F.dict[F.dictp].link = F.code[F.current];
	F.code[F.current] = F.dictp;
	F.dict[F.dictp].flags = flags;
	F.dict[F.dictp].xt = F.cp;
	compile(prim);
	F.dict[F.dictp].name = F.namesp;
	strcpy(&F.names[F.namesp], name);
	F.namesp += name_size;
	F.dictp++;
}


static int getword(char sep)
{
	char *wordp = F.word;
	
	while (SOURCELEFT && ISSEP(sep, CURCHAR))
		F.intp++;
	
	if (!SOURCELEFT)
		return 0;
	
	while (SOURCELEFT && !ISSEP(sep, CURCHAR)) {
		if (wordp < F.word + WORD_MAX - 1)
			*wordp++ = CURCHAR;
		F.intp++;
	}
	
	*wordp = '\0';
	
	if (SOURCELEFT)
		F.intp++;
	
	return 1;
}


static int parse(char sep, int *pstart, int *plength)
{
	int start = F.intp, length = 0;
	int escape = 0;
	
	if (!SOURCELEFT)
		return 0;
	
	while (SOURCELEFT) {
		if (CURCHAR == '\\') {
			F.intp++, length++;
			escape = !escape;
		} else if (ISSEP(sep, CURCHAR)) {
			if (escape) {
				F.intp++, length++;
				escape = 0;
			} else {
				break;
			}
		} else {
			F.intp++, length++;
			escape = 0;
		}
	}
	
	if (!SOURCELEFT)
		return 0;
	
	F.intp++;
		
	if (pstart)
		*pstart = start;
	if (plength)
		*plength = length;
	
	return 1;
}


static word_t *find(const char *word)
{
	int p;		// dict address of next word to check
	int voc = F.context;	// pfa of vocabulary
	
	do {
		for (p = F.code[voc]; p; p = F.dict[p].link)
			if (!ISSET(F.dict[p].flags, SMUDGED) && strcasecmp(&F.names[F.dict[p].name], word) == 0)
				return &F.dict[p];
		voc = F.code[voc + 1];
	} while (voc);
	
	/* Uncomment to enable CURRENT-vocabulary search
	if (F.context == F.current)
		return NULL;
	
	voc = F.current;
	do {
		for (p = F.code[voc]; p; p = F.dict[p].link)
			if (!ISSET(F.dict[p].flags, SMUDGED) && strcasecmp(&F.names[F.dict[p].name], word) == 0)
				return &F.dict[p];
		voc = F.code[voc + 1];
	} while (voc);
	//*/
	
	return NULL;
}


static int toliteral(int *n)
{
	int ret, read;
	
	if (sscanf(F.word, "%d%n", &ret, &read) == 1 && read == strlen(F.word)) {
		if (n)
			*n = ret;
		return 1;
	}
	
	if (sscanf(F.word, "0%*[Xx]%x%n", &ret, &read) == 1 && read == strlen(F.word)) {
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
		w = find(F.word);
		if (w) {
			if (F.state == 0 || ISSET(w->flags, IMMEDIATE))
				execute(w->xt);
			else
				compile(w->xt);
		} else if (F.app_notfound && F.app_notfound(F.word)) {
			// app_notfound() has already done the job
		} else if (toliteral(&n)) {
			if (F.state) {
				compile(F.lit_xt);
				compile(n);
			} else {
				push(n);
			}
		} else {
			error("%s ?", F.word);
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
	check(fwrite(&F.cp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(F.code, sizeof(int), F.cp, f) < F.cp, "save error: %s", strerror(errno));
	check(fwrite(&F.dp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(F.data, 1, F.dp, f) < F.dp, "save error: %s", strerror(errno));
	
	check(fwrite(&F.lit_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.exit_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.branch_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.qbranch_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.dodo_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.doqdo_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.doloop_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.doaddloop_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.codecomma_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.store_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.dotry_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	
	fclose(f);
}
#endif


static void markfwd(enum cftype type)
{
	cfpush(type, F.cp);
	compile(0);
}


static void resolvefwd(enum cftype required)
{
	F.code[cfpop(required)] = F.cp;
}


static void markback(enum cftype type)
{
	cfpush(type, F.cp);
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
			push(F.code[F.ip++]);
			break;
		case ENTER:
			rpush();
			F.running = pfa - 1;
			F.ip = pfa;
			break;
		case EXIT:
			while (F.lsp > 0 && F.lstack[F.lsp - 1].xt == F.running)
				lpop();
			rpop();
			break;
		case BRANCH:
			F.ip = F.code[F.ip];
			break;
		case QBRANCH:
			if (pop())
				F.ip++;
			else
				F.ip = F.code[F.ip];
			break;
		case DODO: {
			int index = pop(), limit = pop(), leave = F.code[F.ip++];
			lpush(index, limit, leave);
			break;
		}
		case DOQDO: {
			int index = pop(), limit = pop(), leave = F.code[F.ip++];
			if (index != limit)
				lpush(index, limit, leave);
			else
				F.ip = leave;
			break;
		}
		case DOLOOP:
			check(F.lsp <= 0, "usage of LOOP outside any loop");
			if (++F.lstack[F.lsp - 1].index == F.lstack[F.lsp - 1].limit) {
				F.ip++;
				lpop();
			} else {
				F.ip = F.code[F.ip];
			}
			break;
		case DOADDLOOP: {
			int step = pop(), index, limit;
			check(F.lsp <= 0, "usage of +LOOP outside any loop");
			index = F.lstack[F.lsp - 1].index;
			limit = F.lstack[F.lsp - 1].limit;
			if ((index < limit) == (index + step < limit)) {
				F.lstack[F.lsp - 1].index += step;
				F.ip = F.code[F.ip];
			} else {
				F.ip++;
				lpop();
			}
			break;
		}
		case DO:
			compile(F.dodo_xt);
			markfwd(CFDO);
			markback(CFLOOP);
			break;
		case QDO:
			compile(F.doqdo_xt);
			markfwd(CFDO);
			markback(CFLOOP);
			break;
		case LOOP:
			compile(F.doloop_xt);
			resolveback(CFLOOP);
			resolvefwd(CFDO);
			break;
		case ADDLOOP:
			compile(F.doaddloop_xt);
			resolveback(CFLOOP);
			resolvefwd(CFDO);
			break;
		case IF:
			compile(F.qbranch_xt);
			markfwd(CFIF);
			break;
		case ELSE: {
			int ifbranch;
			check(cfpeek() != CFIF, "unbalanced control structure");
			ifbranch = cfpop(CFIF);
			compile(F.branch_xt);
			markfwd(CFELSE);
			cfpush(CFIF, ifbranch);
			resolvefwd(CFIF);
			break;
		}
		case THEN: {
			enum cftype type = cfpeek();
			check(type != CFIF && type != CFELSE, "unbalanced control structure");
			resolvefwd(type);
			break;
		}
		case BEGIN:
			markback(CFBEGIN);
			break;
		case UNTIL:
			compile(F.qbranch_xt);
			resolveback(CFBEGIN);
			break;
		case AGAIN:
			compile(F.branch_xt);
			resolveback(CFBEGIN);
			break;
		case WHILE: {
			int beginbranch = cfpop(CFBEGIN);
			compile(F.qbranch_xt);
			markfwd(CFWHILE);
			cfpush(CFBEGIN, beginbranch);
			break;
		}
		case REPEAT:
			compile(F.branch_xt);
			resolveback(CFBEGIN);
			resolvefwd(CFWHILE);
			break;
		case LEAVE:
			check(F.lsp <= 0, "attempt to use LEAVE outside any loop");
			check(F.lstack[F.lsp - 1].xt != F.running, "LEAVE called from nested definition");
			F.ip = F.lstack[F.lsp - 1].leave;
			lpop();
			break;
		case I:
			check(F.lsp <= 0, "attempt to use I outside any loop");
			push(F.lstack[F.lsp - 1].index);
			break;
		case J:
			check(F.lsp <= 1, "attempt to use J without outer loop");
			push(F.lstack[F.lsp - 2].index);
			break;
#ifndef FORTH_ONLY_VM
		case COLON:
			check(getword(' ') == 0, "word required for :");
			create(F.word, SMUDGED, ENTER);
			F.state = FORTH_BOOL(1);
			break;
		case SEMICOLON:
			check(F.state == 0, "; is used outside any definition");
			check(F.cfsp > 0, "unbalanced control structure");
			compile(F.exit_xt);
			CLR(F.dict[F.code[F.current]].flags, SMUDGED);
			F.state = 0;
			break;
#endif
		case EXECUTE: {
			int xt = pop();
			checkcode(xt);
			execute(xt);
			break;
		}
		case DOTRY: {
			jmp_buf ojmp;
			int osp = F.sp, orsp = F.rsp, olsp = F.lsp, oip = F.ip, orunning = F.running;
#ifndef FORTH_ONLY_VM
			const char *osource = F.source;
			int ointp = F.intp, ostate = F.state;
#endif
			memcpy(ojmp, F.errjmp, sizeof(jmp_buf));
			F.errhandlers++;
			if (setjmp(F.errjmp) == 0) {
				int xt = F.code[F.ip++];
				checkcode(xt);
				execute(xt);
				memcpy(F.errjmp, ojmp, sizeof(jmp_buf));
				F.errhandlers--;
				push(~0);
			} else {
				F.sp = osp, F.rsp = orsp, F.lsp = olsp, F.ip = oip, F.running = orunning;
				if (F.running)
					F.ip++;
#ifndef FORTH_ONLY_VM
				F.source = osource;
				F.intp = ointp, F.state = ostate;
#endif
				memcpy(F.errjmp, ojmp, sizeof(jmp_buf));
				F.errhandlers--;
				push(0);
			}
			break;
		}
#ifndef FORTH_ONLY_VM
		case TRY: {
			word_t *w;
			check(getword(' ') == 0, "word required for TRY");
			w = find(F.word);
			check(!w, "%s ?", F.word);
			
			if (F.state) {
				compile(F.dotry_xt);
				compile(w->xt);
			} else {
				jmp_buf ojmp;
				int osp = F.sp, orsp = F.rsp, olsp = F.lsp, oip = F.ip, orunning = F.running;
				const char *osource = F.source;
				int ointp = F.intp, ostate = F.state;
				memcpy(ojmp, F.errjmp, sizeof(jmp_buf));
				F.errhandlers++;
				if (setjmp(F.errjmp) == 0) {
					checkcode(w->xt);
					execute(w->xt);
					memcpy(F.errjmp, ojmp, sizeof(jmp_buf));
					F.errhandlers--;
					push(~0);
				} else {
					F.sp = osp, F.rsp = orsp, F.lsp = olsp, F.ip = oip, F.running = orunning;
					if (F.running)
						F.ip++;
					F.source = osource;
					F.intp = ointp, F.state = ostate;
					memcpy(F.errjmp, ojmp, sizeof(jmp_buf));
					F.errhandlers--;
					push(0);
				}
			}
			break;
		}
#endif
		case ERROR:
			fth_error("%s", fth_area(fth_pop(), 1));
			break;
			
		// arithmetic
		case ADD: {
			int b = pop(), a = pop();
			push(a + b);
			break;
		}
		case SUB: {
			int b = pop(), a = pop();
			push(a - b);
			break;
		}
		case MUL: {
			int b = pop(), a = pop();
			push(a * b);
			break;
		}
		case DIV: {
			int b = pop(), a = pop();
			check(b == 0, "division by zero");
			push(a / b);
			break;
		}
		case MOD: {
			int b = pop(), a = pop();
			check(b == 0, "division by zero");
			push(a % b);
			break;
		}
		case DIVMOD: {
			int b = pop(), a = pop();
			check(b == 0, "division by zero");
			push(a % b);
			push(a / b);
			break;
		}
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
			break;
		}
		case MAX: {
			int b = pop(), a = pop();
			push(a > b ? a : b);
			break;
		}
		case ABS: {
			int a = pop();
			push(a < 0 ? -a : a);
			break;
		}
		
		// stack
		case SWAP: {
			int b = pop(), a = pop();
			push(b);
			push(a);
			break;
		}
		case DUP: {
			int a = pop();
			push(a);
			push(a);
			break;
		}
		case DROP:
			(void)pop();
			break;
		case ROT: {
			int c = pop(), b = pop(), a = pop();
			push(b);
			push(c);
			push(a);
			break;
		}
		case MROT: {
			int c = pop(), b = pop(), a = pop();
			push(c);
			push(a);
			push(b);
			break;
		}
		case TUCK: {
			int b = pop(), a = pop();
			push(b);
			push(a);
			push(b);
			break;
		}
		case OVER: {
			int b = pop(), a = pop();
			push(a);
			push(b);
			push(a);
			break;
		}
		case NIP: {
			int b = pop();
			(void)pop();
			push(b);
			break;
		}
		case DDUP: {
			int b = pop(), a = pop();
			push(a);
			push(b);
			push(a);
			push(b);
			break;
		}
		case DDROP:
			(void)pop();
			(void)pop();
			break;
		case QDUP: {
			int a = pop();
			if (a)
				push(a);
			push(a);
			break;
		}
		
		// logic
		case AND: {
			int b = pop(), a = pop();
			push(a & b);
			break;
		}
		case OR: {
			int b = pop(), a = pop();
			push(a | b);
			break;
		}
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
			break;
		}
		case LESSEQUAL: {
			int b = pop(), a = pop();
			push(FORTH_BOOL(a <= b));
			break;
		}
		case GREATER: {
			int b = pop(), a = pop();
			push(FORTH_BOOL(a > b));
			break;
		}
		case GREATEREQUAL: {
			int b = pop(), a = pop();
			push(FORTH_BOOL(a >= b));
			break;
		}
		case EQUAL: {
			int b = pop(), a = pop();
			push(FORTH_BOOL(a == b));
			break;
		}
		case NOTEQUAL: {
			int b = pop(), a = pop();
			push(FORTH_BOOL(a != b));
			break;
		}
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
			break;
		}
		case BETWEEN: {
			int b = pop(), a = pop(), x = pop();
			push(FORTH_BOOL(a <= x && x <= b));
			break;
		}
		
		// data
		case DOCONSTANT:
			push(F.code[pfa]);
			break;
		case DOVARIABLE:
			push(F.code[pfa]);		// is variable a constant !?!!
			break;
#ifndef FORTH_ONLY_VM
		case CONSTANT:
			check(getword(' ') == 0, "word required for CONSTANT");
			create(F.word, 0, DOCONSTANT);
			compile(pop());
			break;
		case VARIABLE:
			check(getword(' ') == 0, "word required for VARIABLE");
			create(F.word, 0, DOVARIABLE);
			compile(F.dp);
			compile(0);			// xt of DOES>-part
			dcompile(0);
			break;
#endif
		case DODOES:
			push(F.code[pfa]);
			rpush();
			F.running = pfa - 1;
			F.ip = F.code[pfa + 1];
			break;
		case FETCH:
			push(fetch(pop()));
			break;
		case STORE: {
			int a = pop(), x = pop();
			store(a, x);
			break;
		}
		case CFETCH:
			push(cfetch(pop()) & 0xFF);
			break;
		case CSTORE: {
			int a = pop(), x = pop();
			cstore(a, x);
			break;
		}
		case COMMA:
			dcompile(pop());
			break;
		case CCOMMA:
			ccompile(pop());
			break;
#ifndef FORTH_ONLY_VM
		case CREATE:
			check(getword(' ') == 0, "word required for CREATE");
			create(F.word, 0, DOVARIABLE);
			compile(F.dp);
			compile(0);			// xt of DOES>-part
			break;
		case DOES:
			check(F.code[F.dict[F.code[F.current]].xt] != DOVARIABLE, "%s is not CREATEd", &F.names[F.dict[F.code[F.current]].name]);
			F.code[F.dict[F.code[F.current]].xt] = DODOES;
			if (F.running) {
				F.code[F.dict[F.code[F.current]].xt + 2] = F.ip;
				rpop();
			} else {
				F.code[F.dict[F.code[F.current]].xt + 2] = F.cp;
				F.state = FORTH_BOOL(1);
			}
			break;
#endif
		case ADDSTORE: {
			int a = pop(), x = pop();
			checkdata(a, sizeof(int));
			*(int *)&F.data[a] += x;
			break;
		}
		case DOVALUE:
			push(fetch(F.code[pfa]));
			break;
#ifndef FORTH_ONLY_VM
		case VALUE:
			check(getword(' ') == 0, "word required for VALUE");
			create(F.word, 0, DOVALUE);
			compile(F.dp);
			dcompile(0);
			break;
		case TO: {
			word_t *w;
			check(getword(' ') == 0, "word required for TO");
			w = find(F.word);
			check(w == NULL, "%s ?", F.word);
			check(F.code[w->xt] != DOVALUE, "%s is not a VALUE", F.word);
			if (F.state) {
				compile(F.lit_xt);
				compile(F.code[w->xt + 1]);
				compile(F.store_xt);
			} else {
				store(F.code[w->xt + 1], pop());
			}
			break;
		}
#endif
		case HERE:
			push(F.dp);
			break;
		case ALLOT: {
			int size = pop();
			checkdata(F.dp, size);
			F.dp += size;
			break;
		}
		case TODATA: {
			int xt = pop();
			checkcode(xt);
			check(F.code[xt] != DOVARIABLE && F.code[xt] != DODOES, "attempt to get data address of something, that is not CREATEd");
			push(F.code[xt + 1]);
			break;
		}
		case MOVE: {
			int size = pop(), b = pop(), a = pop();
			checkdata(a, size);
			checkdata(b, size);
			memmove(&F.data[b], &F.data[a], size);
			break;
		}
		case FILL: {
			int c = pop(), size = pop(), a = pop();
			checkdata(a, size);
			memset(&F.data[a], c, size);
			break;
		}
		case ERASE: {
			int size = pop(), a = pop();
			checkdata(a, size);
			memset(&F.data[a], 0, size);
			break;
		}
		
		// compilation
		case CODECOMMA:
			compile(pop());
			break;
#ifndef FORTH_ONLY_VM
		case COMPILE: {
			word_t *w;
			check(getword(' ') == 0, "word required for COMPILE");
			w = find(F.word);
			check(w == NULL, "%s ?", F.word);
			compile(F.lit_xt);
			compile(w->xt);
			compile(F.codecomma_xt);
			break;
		}
		case COMPILENOW: {
			word_t *w;
			check(getword(' ') == 0, "word required for [COMPILE]");
			w = find(F.word);
			check(w == NULL, "%s ?", F.word);
			compile(w->xt);
			break;
		}
		case TICK: {
			word_t *w;
			check(getword(' ') == 0, "word required for '");
			w = find(F.word);
			check(w == NULL, "%s ?", F.word);
			push(w->xt);
			break;
		}
		case TICKNOW: {
			word_t *w;
			check(getword(' ') == 0, "word required for [']");
			w = find(F.word);
			check(w == NULL, "%s ?", F.word);
			compile(F.lit_xt);
			compile(w->xt);
			break;
		}
		case MAKEIMMEDIATE:
			SET(F.dict[F.code[F.current]].flags, IMMEDIATE);
			break;
		case STATE:
			push(FORTH_BOOL(F.state));
			break;
		case COMPON:
			F.state = ~0;
			break;
		case COMPOFF:
			F.state = 0;
			break;
		case BLOCKSTART:
			F.state = ~0;
			push(F.cp);
			compile(ENTER);
			break;
		case BLOCKEND:
			check(F.state == 0, "attempt to use } outside any definition");
			check(F.cfsp > 0, "unbalanced control structure");
			F.state = 0;
			compile(F.exit_xt);
			execute(pop());
			break;
#endif
		case LENCODE:
			push(F.cp);
			break;
#ifndef FORTH_ONLY_VM
		case LENDICT:
			push(F.dictp);
			break;
		case LENNAMES:
			push(F.namesp);
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
			if (F.state) {
				compile(F.lit_xt);
				compile(F.word[0]);
			} else {
				push(F.word[0]);
			}
			break;
		case QUOTE: {
			int start, length;
			check(parse('"', &start, &length) == 0, "unmatched \"");
			if (F.state) {
				compile(F.lit_xt);
				compile(F.dp);
			} else {
				push(F.dp);
			}
			scompile(&F.source[start], length);
			break;
		}
#endif
		case DEPTH:
			push(F.sp);
			break;
		case LENGTH: {
			int a = pop();
			checkdata(a, 1);
			push(strlen(&F.data[a]));
			break;
		}
		case COUNT: {
			int a = pop();
			checkdata(a, 1);
			push(a);
			push(strlen(&F.data[a]));
			break;
		}
		case BL:
			push(' ');
			break;
#ifndef FORTH_ONLY_VM
		case STRING: {
			int sep = pop(), start, length;
			check(!parse(sep, &start, &length), "string separated by `%c' required for STRING", sep);
			push(F.dp);
			scompile(&F.source[start], length);
			break;
		}
		case WORD:
			check(getword(pop()) == 0, "word required for WORD");
			checkdata(F.dp, strlen(F.word) + 1);
			strcpy(&F.data[F.dp], F.word);
			push(F.dp);
			break;
		case VOCABULARY:
			check(getword(' ') == 0, "word required for VOCABULARY");
			create(F.word, 0, DOVOCABULARY);
			compile(0);			// dict address of latest definition in this voc
			compile(F.current);		// link to parent voc
			break;
		case DOVOCABULARY:
			F.context = pfa;
			break;
		case DEFINITIONS:
			F.current = F.context;
			break;
#  ifndef FORTH_NO_SAVES
		case SAVE: {
			int a = pop();
			checkdata(a, 1);
			fth_savesystem(&F.data[a]);
			break;
		}
		case LOAD: {
			int a = pop();
			checkdata(a, 1);
			fth_loadsystem(&F.data[a]);
			break;
		}
#  endif	/* FORTH_NO_SAVES */
#endif		/* FORTH_ONLY_VM */
#ifndef FORTH_NO_SAVES
		case SAVEPROGRAM: {
			int entry = pop();
			int a = pop();
			checkcode(entry);
			checkdata(a, 1);
			saveprogram(&F.data[a], entry);
			break;
		}
		case SAVEDATA: {
			int a = pop();
			checkdata(a, 1);
			fth_savedata(&F.data[a]);
			break;
		}
		case LOADDATA: {
			int a = pop();
			checkdata(a, 1);
			fth_loaddata(&F.data[a]);
			break;
		}
#endif
		
		default:
			F.app_prims(prim);
			break;
	}
}


// ================================== API =====================================

// ... for application primitives
void fth_error(const char *fmt, ...)
{
	va_list args;
	
	if (fmt != F.errormsg) {		// for re-throwing error messages
		va_start(args, fmt);
		vsnprintf(F.errormsg, ERROR_MAX, fmt, args);
		va_end(args);
	}
	
	if (F.errhandlers)
		longjmp(F.errjmp, 1);
	else
		abort();
}


void fth_push(int x)
{
	check(F.sp >= STACK_SIZE, "stack overflow");
	F.stack[F.sp++] = x;
}


int fth_pop(void)
{
	check(F.sp <= 0, "stack underflow");
	return F.stack[--F.sp];
}


int fth_fetch(int a)
{
	checkdata(a, sizeof(int));
	return *(int *)&F.data[a];
}


void fth_store(int a, int x)
{
	checkdata(a, sizeof(int));
	*(int *)&F.data[a] = x;
}


char fth_cfetch(int a)
{
	checkdata(a, 1);
	return F.data[a];
}


void fth_cstore(int a, char x)
{
	checkdata(a, 1);
	F.data[a] = x;
}


char *fth_area(int a, int size)
{
	checkdata(a, size);
	return &F.data[a];
}


void fth_init(primitives_f app_primitives, notfound_f app_notfnd)
{
	F.app_prims = app_primitives;
	F.app_notfound = app_notfnd;
	F.data[DATA_SIZE] = '\0';
	reset();
	F.cp = F.dp = 1;		// 0 is an "invalid" address
	F.errhandlers = 0;
	
	// initial vocabulary
#ifndef FORTH_ONLY_VM
	F.namesp = 0;
	F.dictp = 1;
	F.context = F.current = 0;
	F.code[0] = 0;
	create("FORTH", 0, DOVOCABULARY);
	F.context = F.current = F.forth_voc = F.cp;
	compile(1);
	compile(0);
	
	// core xt-s
	F.lit_xt = F.cp;		compile(LIT);
	F.branch_xt = F.cp;		compile(BRANCH);
	F.qbranch_xt = F.cp;		compile(QBRANCH);
	F.dodo_xt = F.cp;		compile(DODO);
	F.doqdo_xt = F.cp;		compile(DOQDO);
	F.doloop_xt = F.cp;		compile(DOLOOP);
	F.doaddloop_xt = F.cp;		compile(DOADDLOOP);
	F.dotry_xt = F.cp;		compile(DOTRY);
	
	fth_library(core_words);
	
	F.exit_xt = find("EXIT")->xt;
	F.codecomma_xt = find("CODE,")->xt;
	F.store_xt = find("!")->xt;
#endif
}


#ifndef FORTH_ONLY_VM
void fth_primitive(const char *name, int code, int immediate)
{
	create(name, immediate ? IMMEDIATE : 0, code);
}


int fth_interpret(const char *s)
{
	const char *osource = F.source;
	int ointp = F.intp;
	jmp_buf oerr;
	int ret;
	
	memcpy(oerr, F.errjmp, sizeof(jmp_buf));
	F.errhandlers++;
	if (setjmp(F.errjmp) == 0) {
		F.source = s;
		F.intp = 0;
		do_interpret();
		ret = 1;
		F.intp = ointp;
		F.source = osource;
	} else {
		ret = 0;
	}
	memcpy(F.errjmp, oerr, sizeof(jmp_buf));
	F.errhandlers--;
	return ret;
}


int fth_execute(const char *w)
{
	word_t *pw;
	jmp_buf oerr;
	int ret;
	
	memcpy(oerr, F.errjmp, sizeof(jmp_buf));
	F.errhandlers++;
	if (setjmp(F.errjmp) == 0) {
		pw = find(w);
		check(pw == NULL, "%s ?", w);
		execute(pw->xt);
		ret = 1;
	} else {
		ret = 0;
	}
	
	memcpy(F.errjmp, oerr, sizeof(jmp_buf));
	F.errhandlers--;
	return ret;
}


void fth_library(primitive_word_t *lib)
{
	int i;
	
	for (i = 0; lib[i].name; i++)
		fth_primitive(lib[i].name, lib[i].code, lib[i].immediate);
}


int fth_getstate(void)
{
	return F.state;
}
#endif


void fth_reset(void)
{
	F.sp = F.rsp = F.lsp = F.cfsp = 0;
	F.running = 0;
	F.errormsg[0] = 0;
	F.errhandlers = 0;
#ifndef FORTH_ONLY_VM
	F.state = 0;
	F.context = F.current = F.forth_voc;
#endif
}


const char *fth_geterror(void)
{
	return F.errormsg;
}


int fth_getdepth(void)
{
	return F.sp;
}


int fth_getstack(int idx)
{
	if (idx >= 0 && idx < F.sp)
		return F.stack[idx];
	else
		return 0;
}


#ifndef FORTH_ONLY_VM
const char *fth_geterrorline(int *plen, int *pintp, int *plineno)
{
	int line = 1, beg = 0, end, i;
	
	if (!F.source)
		return NULL;
	
	if (F.intp > 0)
		F.intp--;
	
	while (F.intp > 0 && (F.intp == strlen(F.source) || ISSEP(' ', CURCHAR)))
		F.intp--;
	
	for (i = 0; i < F.intp; i++)
		if (F.source[i] == '\n') {
			line++;
			beg = i + 1;
		}
	
	for (; i < strlen(F.source); i++)
		if (F.source[i] == '\n')
			break;
	end = i;
	
	if (plen)
		*plen = end - beg;
	if (pintp)
		*pintp = F.intp - beg;
	if (plineno)
		*plineno = line;
	return &F.source[beg];
}


int fth_gettracedepth(void)
{
	return F.rsp;
}


const char *fth_gettrace(int idx)
{
	int xt, pw, voc;
	
	if (idx < 0 || idx >= F.rsp)
		return "<invalid backtrace index>";
	
	if (idx == F.rsp - 1)
		xt = F.running;
	else
		xt = F.rstack[idx + 1].xt;
	
	voc = F.context;
	do {
		for (pw = F.code[voc]; pw; pw = F.dict[pw].link)
			if (F.dict[pw].xt == xt)
				return &F.names[F.dict[pw].name];
		voc = F.code[voc + 1];
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
	check(fwrite(&F.cp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(F.code, sizeof(int), F.cp, f) < F.cp, "save error: %s", strerror(errno));
	check(fwrite(&F.dp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(F.data, 1, F.dp, f) < F.dp, "save error: %s", strerror(errno));
	check(fwrite(&F.dictp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(F.dict, sizeof(word_t), F.dictp, f) < F.dictp, "save error: %s", strerror(errno));
	check(fwrite(&F.namesp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(F.names, 1, F.namesp, f) < F.namesp, "save error: %s", strerror(errno));
	check(fwrite(&F.forth_voc, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	
	check(fwrite(&F.lit_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.exit_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.branch_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.qbranch_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.dodo_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.doqdo_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.doloop_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.doaddloop_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.codecomma_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.store_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(&F.dotry_xt, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	
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
	
	check(fread(&F.cp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(F.cp > CODE_SIZE, "saved code (%d B) is larger than code area (%d B)", F.cp, CODE_SIZE);
	check(fread(F.code, sizeof(int), F.cp, f) < F.cp, "load error: %s", strerror(errno));
	check(fread(&F.dp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(F.dp > DATA_SIZE, "saved data (%d B) is larger than data area (%d B)", F.dp, DATA_SIZE);
	check(fread(F.data, 1, F.dp, f) < F.dp, "load error: %s", strerror(errno));
	check(fread(&F.dictp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(F.dictp > DICT_SIZE, "saved dictionary (%d words) is larger than dictionary area (%d B)", F.dictp, DICT_SIZE);
	check(fread(F.dict, sizeof(word_t), F.dictp, f) < F.dictp, "load error: %s", strerror(errno));
	check(fread(&F.namesp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(F.namesp > NAMES_SIZE, "saved word names (%d B) is larger than names area (%d B)", F.namesp, NAMES_SIZE);
	check(fread(F.names, 1, F.namesp, f) < F.namesp, "load error: %s", strerror(errno));
	check(fread(&F.forth_voc, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	
	check(fread(&F.lit_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.exit_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.branch_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.qbranch_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.dodo_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.doqdo_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.doloop_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.doaddloop_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.codecomma_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.store_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.dotry_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	
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
int fth_runprogram(const char *fname)
{
	char sig[4];
	int entry;
	FILE *f = fopen(fname, "rb");
	jmp_buf oerr;
	int ret;
	
	check(!f, "load error: %s", strerror(errno));
	
	check(fread(sig, 1, 4, f) < 4, "load error: %s", strerror(errno));
	check(sig[0] != PROGRAM_MARK, "load error: invalid program mark: %c", sig[0]);
	check(sig[1] != endian(), "program is saved for different data endianness: %d (we have %d)", sig[1], endian());
	check(sig[2] != sizeof(int), "program is saved for different cell size: %d (we have %d)", sig[2], sizeof(int));
	check(sig[3] != 0, "signature reserved byte is non-zero");
	
	check(fread(&entry, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.cp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(F.cp > CODE_SIZE, "saved code (%d B) is larger than code area (%d B)", F.cp, CODE_SIZE);
	check(fread(F.code, sizeof(int), F.cp, f) < F.cp, "load error: %s", strerror(errno));
	check(fread(&F.dp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(F.dp > DATA_SIZE, "saved data (%d B) is larger than data area (%d B)", F.dp, DATA_SIZE);
	check(fread(F.data, 1, F.dp, f) < F.dp, "load error: %s", strerror(errno));
	
	check(fread(&F.lit_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.exit_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.branch_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.qbranch_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.dodo_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.doqdo_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.doloop_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.doaddloop_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.codecomma_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.store_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(fread(&F.dotry_xt, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	
	fclose(f);
	fth_reset();
	
	memcpy(oerr, F.errjmp, sizeof(jmp_buf));
	F.errhandlers++;
	if (setjmp(F.errjmp) == 0) {
		checkcode(entry);
		execute(entry);
		ret = 1;
	} else {
		ret = 0;
	}
	memcpy(F.errjmp, oerr, sizeof(jmp_buf));
	F.errhandlers--;
	return ret;
}


void fth_savedata(const char *fname)
{
	char sig[4] = {DATA_MARK, endian(), sizeof(int), 0};
	FILE *f;
	
	f = fopen(fname, "wb");
	check(!f, "save error: %s", strerror(errno));
	
	check(fwrite(sig, 1, 4, f) < 4, "save error: %s", strerror(errno));
	check(fwrite(&F.dp, sizeof(int), 1, f) == 0, "save error: %s", strerror(errno));
	check(fwrite(F.data, 1, F.dp, f) < F.dp, "save error: %s", strerror(errno));
	
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
	
	check(fread(&F.dp, sizeof(int), 1, f) == 0, "load error: %s", strerror(errno));
	check(F.dp > DATA_SIZE, "saved data (%d B) is larger than data area (%d B)", F.dp, DATA_SIZE);
	check(fread(F.data, 1, F.dp, f) < F.dp, "load error: %s", strerror(errno));
	
	fclose(f);
}
#endif






