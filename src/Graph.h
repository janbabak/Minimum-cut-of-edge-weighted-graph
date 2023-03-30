#pragma once

#include <fstream>
#include <iostream>

#include "Edge.cpp"

using namespace std;

struct Graph {
    int vertexesCount;
    int edgesSize;
    int edgesAllocatedSize;
    Edge *edges;

    Graph();

    // copy constructor
    Graph(const Graph &source);

    ~Graph();

    // operator ==
    Graph &operator=(const Graph &source);

    bool loadFromFile(const string &path);

    void addEdge(Edge edge);

    friend ostream &operator<<(ostream &os, const Graph &graph);
};