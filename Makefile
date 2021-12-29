CC=cc
SRC=main.c forth.c
EXESUFFIX=.exe
EXE=forth$(EXESUFFIX)

all: $(EXE)

$(EXE): $(SRC) forth.h
	$(CC) -g -Wall -o $(EXE) $(SRC)

run: $(EXE)
	./$(EXE)

gdb: $(EXE)
	gdb $(EXE)

release: $(SRC) forth.h
	$(CC) -s -O3 -o $(EXE) $(SRC)

clean:
	rm -f $(EXE)

work_blob:
	7za a blobs/newforth_`date +%Y%m%d`w.zip $(SRC) forth.h $(EXE) Makefile README.txt internals.txt

home_blob:
	7za a blobs/newforth_`date +%Y%m%d`h.zip $(SRC) forth.h $(EXE) Makefile README.txt internals.txt
