GCC = mpic++
target = test_mpi
src = src
build = build

test:
	$(GCC) $(src)/$(target).cpp -o $(build)/$(target).o

run: test 
	mpirun -np 4 $(build)/$(target).o

clean:
	rm -f build