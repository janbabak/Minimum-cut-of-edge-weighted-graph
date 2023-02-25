#include <iostream>

#include "Graph.h"

using namespace std;

int main() {
    Graph graph = Graph();

    graph.loadFromFile("graf_mro/graf_10_5.txt");

    cout << graph;

    return 0;
}
