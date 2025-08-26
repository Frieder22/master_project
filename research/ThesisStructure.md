# Title
## Abstract
## Table of Contents
## (Abbreviations)
## Introduction
* Motivation
    * BFS on HPC cluster
    * Do edge centric distribution of graph
    * expected even computation distribution on procs
    * Need for fast computation of the Union
* Idea:
    * Writing all marked elements in bitmap is memory efficient, but
    * when few elements are marked, bitmap is wasteful (many 0s).
    * In this case a list with explicit integer is more powerfull.
    * $\Rightarrow$ Create flexible reduce algorithm.
## Algorithms
### Description of Alforithms
* Algo 1:
    * Set union is logic OR on bitmap
    * Can be implemented by Reducescatter followed by an Allgather (optimal in α: number of iterations is minimal)
    * Reducescatter can be implemented in Butterfly-scheme.
    * Set union of lists can be represented concatination of all lists to all procs (disregarding duplicates).
    * This is implemented by Allgather_v
    * Allgather_v can also be realised using Butterfly scheme.
    * $\Rightarrow$ we can use sparse representation first and can then switch to dense if needed.
    * Bonus: we don't have to do allgather phase afterwards.
    * $\Rightarrow$ This needs proof.
    * We need simple (fast) logic for when switching to dense variant.
    * Pseudo Code and explanation of the code.
* Algorithm 2:
    * Problem: Butterfly works only on a $2^k$ Number of procs
    * Solution: Algorithm from Träff using approx Halfing and reordering of blocks
    * Small explanation of scheme
    * (At least one) Reordering can be smartly omitted by stting bits directly in correct blocks
    * We use again an Reducescatter phase and an Allgather Phase.
    * We can again stop after Reducescatter phase, when only using sparse variant
    * $\Rightarrow$ proof for this statement
    * Similar logic for when decide to switch
    * Pseudo Code and explanation of the code
* Algorithm 3/4:
    * Include another representation for very dense sets
    * Transformation and creation can only happen from dense
    * Rely on fact, that list of missing values is allways in ascending order. So merging two lists is also quite fast.
    * Allgather phase can not be skipped. The difference here is, that we switch in logic OR only from sparse to dense, but we can switch from dense to superdense.
    * In Reducescatter phase: When we switch from dense to super dense, we only have information of the current sent block(s) and not from the whole Sets provided by the subtree of processors.
    * But: Maybe faster Allgather phase possible
### Runtime Complexities of Algorithms
* Model runtime complexities of the algorithms
## Performance Results
### Tests to perform:
* Number of processors
* Distribution of processors (eg. $(1\times 32)$ vs $(32\times 1)$ )
* Different Set sizes.
* Different fillings
    * all equally filled
    * only one filled
    * All sparsly filled
    * All densly filled
    * all different filled (cascading up)
* Different MPI implementations (mostly for reference performance)
    * openMPI
    * mpich
    * intel-mpi

## Application in the BFS
* come back to original Motivation
* testing for different graphs.
    * different sizes
    * different topologies:
        * narrow graphs
        * wide graphs
    * real world graph examples
    * $2^k$- and not $2^k$ number of procs
* Validation of BFS
    * 5 steps of Validation
    * argue for completeness of Validation
    * argue correctness of Allreduce

## Conlusion
* general performance of algo 1
* general performance of algo 2
* at which densities the algos are benificial
* what is the cost of flexibility for dense sets
* Which distributions are beneficial
* performance of the application (BFS)

## Outlook
* optimization of implementation in C
* Extension to other Reduce operations like logic AND or logic XOR
* Propose maybe an algoritm for logic OR starting where super dense and sparse have inverted roles