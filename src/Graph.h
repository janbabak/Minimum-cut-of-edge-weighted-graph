#include <fstream>
#include <iostream>

#include "Edge.h"
#include "Vertex.h"

using namespace std;

class Graph {
   public:
    Graph() : vertexesSize(0), edgesSize(0), edgesAllocatedSize(0) {
        this->vertexes = new Vertex[vertexesSize];
        this->edges = new Edge[edgesAllocatedSize];
    }

    bool loadFromFile(const string &path) {
        ifstream file;
        file.open(path);

        if (!file.is_open()) {
            cerr << "Can't open input file." << endl;
            return false;
        }

        int number;

        // create vertexes
        file >> vertexesSize;
        vertexes = new Vertex[vertexesSize];
        for (int i = 0; i < vertexesSize; i++) {
            vertexes[i].id = i;
        }

        int edgeFromVertexId = 0;
        int edgeToVertexId = 0;

        while (file >> number) {
            if (number != 0) {
                addEdge(Edge(edgeFromVertexId, edgeToVertexId, number));
            }
            edgeToVertexId = (edgeToVertexId + 1) % vertexesSize;
            if (edgeToVertexId == 0) {
                edgeFromVertexId++;
            }
        }

        return edgeFromVertexId == vertexesSize && edgeToVertexId == 0;
    }

    void addEdge(Edge edge) {
        // realloc if needed
        if (edgesSize == edgesAllocatedSize) {
            edgesAllocatedSize *= 2;
            Edge *tmp = new Edge[edgesAllocatedSize];
            for (int i = 0; i < edgesSize; i++) {
                tmp[i] = edges[i];
            }
            delete[] edges;
            edges = tmp;
        }

        // add edge to the end
        edges[edgesSize] = edge;
        edgesSize++;
    }

    friend ostream &operator<<(ostream &os, const Graph &graph) {
        os << "vertexes:" << endl;
        for (int i = 0; i < graph.vertexesSize; i++) {
            os << "id: " << graph.vertexes[i].id
               << ", visited: " << graph.vertexes[i].visited
               << ", isGroupX: " << graph.vertexes[i].isGroupX
               << endl;
        }
        os << "edges:" << endl;
        for (int i = 0; i < graph.edgesSize; i++) {
            os << "vId1: " << graph.edges[i].vertexId1
               << ", vId2: " << graph.edges[i].vertexId2
               << ", weight: " << graph.edges[i].weight
               << endl;
        }
        return os;
    }

   private:
    Vertex *vertexes;
    int vertexesSize;
    Edge *edges;
    int edgesSize;
    int edgesAllocatedSize;
};