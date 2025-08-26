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
Algorithm 
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