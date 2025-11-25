#!/bin/sh
# Copyright (C) 2014-2025 by Thomas Auzinger <thomas@auzinger.name>

CLASS=vutinfth
# Get the document base path from the first argument.
# This functionality is used by the continuous integration tests.
# Use shell parameter expansion: 
# If $1 is unset or null, assign the value "example" to SOURCE.
SOURCE=${1:-thesis}

# Build vutinfth documentation
pdflatex $CLASS.dtx
pdflatex $CLASS.dtx
makeindex -s gglo.ist -o $CLASS.gls $CLASS.glo
makeindex -s gind.ist -o $CLASS.ind $CLASS.idx
pdflatex $CLASS.dtx
pdflatex $CLASS.dtx

# Build the vutinfth class file
pdflatex $CLASS.ins

# Build the vutinfth example document
pdflatex $SOURCE
bibtex   $SOURCE
pdflatex $SOURCE
pdflatex $SOURCE
makeindex -t $SOURCE.glg -s $SOURCE.ist -o $SOURCE.gls $SOURCE.glo
makeindex -t $SOURCE.alg -s $SOURCE.ist -o $SOURCE.acr $SOURCE.acn
makeindex -t $SOURCE.ilg -o $SOURCE.ind $SOURCE.idx
pdflatex $SOURCE
pdflatex $SOURCE

echo
echo
echo Class file and $SOURCE document compiled.
