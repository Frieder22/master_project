GCC = mpicc
CFLAGS = -O3

target = main
SOURCES = vertexSet.c vertexSetIterator.c
HEADERS = vertexSet.h bitmap_custom.h vertexSetIterator.h
 
build = ../build

test: main.c $(SOURCES) $(HEADERS)
	$(GCC) $(CFLAGS) -o $(build)/$(target).o $(target).c vertexSet.c

run: test 
	mpirun -np 4 $(build)/$(target).o

clean:
	rm -rf build