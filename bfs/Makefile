GCC = mpicc
CFLAGS = -O3 -I ../vertexSet 
VSFOLDER = ../vertexSet/


SOURCES = $(VSFOLDER)vertexSet.c $(VSFOLDER)vertexSetIterator.c bfs_custom.c csr_custom.c
HEADERS = $(VSFOLDER)vertexSet.h $(VSFOLDER)bitmap_custom.h $(VSFOLDER)vertexSetIterator.h $(VSFOLDER)testStructure.h bfs_custom.h
 
build = ../build

main: main.c $(SOURCES) $(HEADERS)
	$(GCC) $(CFLAGS) -o $(build)/main.o main.c $(SOURCES) $(HEADERS)

run_local: main
	mpirun -np 4 $(build)/main.o -f graphData/12831_clean.edges

init_cluster:
	ssh -t hydra 'bash --login -i -c "mkdir -p build && mkdir -p data && mkdir -p performance"'
	scp -r cJSON hydra:~/performance/.
	scp executeExperimentRuns.c hydra:~/performance/.
	scp Makefile hydra:~/performance/.

copy_to_cluster:
	scp -r ../vertexSet/ hydra:~/.
	scp run.job hydra:~/performance/
	scp runs.json hydra:~/performance/
	scp executeExperimentRuns.c hydra:~/performance/

run: 
	python setupExperiment.py
	make copy_to_cluster
	ssh -t hydra 'bash --login -i -c "cd performance && sbatch run.job"'


.PHONY: clean_Data

clean_Data:
	echo Not implemented