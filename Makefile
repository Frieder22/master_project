GCC = gcc
target = test_mpi

test:
	$(GCC) $(target).cpp -o $(target).o

run: test 
	./$(target).o

clean:
	rm -f $(target).o