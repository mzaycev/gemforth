CC=cc
SRC=main.c forth.c
EXESUFFIX=.exe
EXE=forth$(EXESUFFIX)
VMEXE=forthvm$(EXESUFFIX)

all: $(EXE) $(VMEXE)

$(EXE): $(SRC) forth.h
	$(CC) -g -Wall -o $(EXE) $(SRC)

$(VMEXE): $(SRC) forth.h
	$(CC) -DFORTH_ONLY_VM -g -Wall -o $(VMEXE) $(SRC)

run: $(EXE)
	./$(EXE)

gdb: $(EXE)
	gdb $(EXE)

release: $(SRC) forth.h
	$(CC) -s -O3 -o $(EXE) $(SRC)
	$(CC) -DFORTH_ONLY_VM -s -O3 -o $(VMEXE) $(SRC)

clean:
	rm -f $(EXE) $(VMEXE)

work_blob:
	7za a blobs/newforth_`date +%Y%m%d`w.zip $(SRC) forth.h $(EXE) $(VMEXE) Makefile README.txt internals.txt

home_blob:
	7za a blobs/newforth_`date +%Y%m%d`h.zip $(SRC) forth.h $(EXE) $(VMEXE) Makefile README.txt internals.txt
