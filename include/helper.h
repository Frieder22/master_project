#ifndef HELPER_FUN
#define HELPER_FUN
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#pragma once

struct Edge {
    int start;
    int end;
};

struct NodeIdx {
    int node;
    int pos;
    
}


void testPrint(void){
    printf("TEST");
};

void testRead(){
    char filepath[] = "graph_data/cit-HepPh_adj.tsv";
    std::string line;
    std::vector<Edge> edges_list;
    std::vector<>
    int endNode;
    int delimiter_pos;

    // open file
    std::ifstream file(filepath);
    if (!file.is_open()){
        std::cerr << "file at " << filepath <<
        " can not be opened!";
    }

    // get a line
    while (std::getline(file, line)) {   
        // find first and second entry as string
        delimiter_pos = line.find("\t");
        std::string node1_s = line.substr(0, delimiter_pos);
        line.erase(0, delimiter_pos + 1);
        delimiter_pos = line.find("\t");
        std::string node2_s = line.substr(0, delimiter_pos);
        
        // create new Edge object
        endNode= std::stoi(node2_s); 
        Edge edge = {std::stoi(node1_s), endNode};

        // add edge to list
        edges_list.push_back(edge);
    }
    std::cout << edges_list[100].start << " " << edges_list[100].end << std::endl;

    std::cout << edges_list.data()[0].start << std::endl;


};


#endif