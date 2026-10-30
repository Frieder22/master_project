# Project for Masters Thesis "A More Efficient Set Union Operation on Distributed Memory Machines"

This repository includes all executables, data and tex-files of the authors masters thesis. To get an overview of the topic we give here the Abstract of this thesis:   

Distributed memory computing is an important branch in the research field of high
performance computing and is the foundation for building large, scalable clusters and
supercomputers. To compare performance of these compute-systems different established
benchmarks are available. Next to renowned benchmarks (e.g., HPL, HPCG and NPB)
there is the Graph500 benchmark which uses the execution time of a BFS (Breadth First
Search) as the main performance measure. BFS and other graph traversal algorithms
are needed in real-world applications such as social network analysis, simulation or
infrastructure planing. A base building block of such graph traversal algorithms is
computing the union of sets that are distributed over the different processes. For finite
graphs the proposed sets and therefore the resulting union are subsets of a universe set
of finite size. It is of interest, and also the aim of this thesis, to optimize such a union
operation.
It is already known that such a union can be calculated by representing the sets as
bitmaps and performing an Allreduce using a bitwise OR. Another strategy is to represent
the sets as sparse lists of elements and concatenate these distributed lists by an Allgather.
The execution time of the first method scales with the size of the universe, and the
execution time of the latter method depends on the size of the sets submitted by the
different processes. If the size of the submitted sets is large, the first strategy is expected
to be faster and for small sets the second variant is expected to be more efficient.
We propose in this thesis two new algorithms that take advantage of both (sparse and
dense) representations and switch dynamically between them to calculate a union of
distributed sets. One is optimized (but also constrained to) compute-systems that have a
number of processors and a hierarchical structure that is a power of 2. The other one
works for an arbitrary amount of processes. Using a linear cost model the execution
time of these algorithms is estimated by T ≤ κ min(TAllred, TAllgath) with κ being some
constant which is close to 1.
The performance of the newly proposed algorithms is tested and compared against the
naive versions. This is done on the Hydra cluster of the Parallel Computing department
at TU Wien. The evaluation confirms the expected run time, with the new algorithms
matching (up to constant) the performance of the respective faster naive implementation.
Moreover, the new algorithms are used as a building block for an BFS where they lead to
a performance increase for large graphs (at least 106 vertices) of up to 10%. For disjoint
graphs there are performance increases of nearly 70% possible.

The complete PDF of the thesis discussing can be found in `tex/thesis.pdf`.

## Folder structure

* `bfs/`    
Holds C- and Header-files for different BFS implementations (using different union operations). There is also a small python script that executes a BFS for sanity check. `make run_local` executes a BFS using an example graph on 4 processors.

* `build/`    
Build folder containing compiled executables.

* `graphData/`    
Contains the edge-files of the tested graphs

* `graphDataRaw/`    
Contains the original data of different graphs provided by the 
[SNAP](https://snap.stanford.edu/snap/) database. In this folder is also the tooling to preprocess the original data.

* `performance/`    
Tools and setups to execute run time experiments and validation runs. The setups are ordered and saved as JSON files in `performance/experiments`. An experiments gets scheduled by putting the corresponding filepath into `performance/experiments/sheduledExperiment.txt`. By executing `make run_local` the experiment is executed locally using 3 processors. Moreover, the Makefile contains commands to execute code on a cluster using SLURM as a scheduler. 

* `data/`    
Contains all measured data.

* `slurm/`    
Tooling to execute code on a cluster using SLURM.

* `tex/`    
Contains tex-files that build the PDF of the thesis.

* `VertexSet/`    
Holds the C- and Header-files that are needed for the new Union Operations. The files for performance and correctness testing are also in this repository. 








