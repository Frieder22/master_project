#ifndef HELPER_FUN
#define HELPER_FUN
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_map>

#pragma once

/**
 * Class to represent an undirected Graph. Vertex major ordering is used.
 * Neighbors of a node can be found fast. 
 */
template <typename NodeType>
class Graph {
    std::unordered_map<NodeType, std::vector<NodeType>> nodes;
    NodeType nEdges;
    NodeType nNodes;

    public:
    Graph() {
        this->nEdges = 0;
        this->nNodes = 0;
    };
    
    ~Graph() = default;

    void addEdge(NodeType start, NodeType end){
        // check if start node is already in graph
        bool exists = (nodes.find(start) != nodes.end());
        if (exists) {
            // append the end Node to the corresponding edgeList
            nodes.at(start).push_back(end);            
        } else {
            // create new edgeList containing only the end Node
            std::vector<NodeType> edgeList = {end};
            nodes.insert(std::pair<NodeType, std::vector<NodeType>>(start, edgeList));

            // increase node counter
            nNodes++;
        }
        // increase edge counter
        nEdges++;
    };

    std::vector<NodeType> getOutNeighbors(NodeType node){
        std::vector<NodeType> edgeList = nodes.at(node);
        return edgeList;
    };

    std::vector<NodeType> getInNeighbors(NodeType node){
        return std::move(this->getOutNeighbors(node));
    };
};

/**
 * Function to read the corresponding subgraph from a tab separated (tsv) 
 * or comma separated (csv) file.
 * @param rank      rank of the process
 * @param size      size of communicator 
 * @param filepath  path to tsv file
 */
Graph<int> readGraph(int rank, int size, std::string filepath = "graph_data/cit-HepPh_adj.tsv",
                    std::string delimiter = "\t"){
    std::string line;
    int startNode, endNode;
    int delimiter_pos, counter;

    // open file
    std::ifstream file(filepath);
    if (!file.is_open()){
        std::cerr << "file at " << filepath <<
        " can not be opened!";
    }

    // get a line
    counter = 0;
    Graph<int> subgraph; 
    while (std::getline(file, line)) {   
        // find first and second entry as string
        delimiter_pos = line.find(delimiter);
        std::string node1_s = line.substr(0, delimiter_pos);
        line.erase(0, delimiter_pos + 1);
        delimiter_pos = line.find(delimiter);
        std::string node2_s = line.substr(0, delimiter_pos);
        
        if (counter%size == rank) {
            // create new Edge object
            endNode= std::stoi(node2_s); 
            startNode = std::stoi(node1_s);

            // add edge to subgraph
            subgraph.addEdge(startNode, endNode);
        }

        //increase counter
        counter++;
    }
    return subgraph;
};


#endif