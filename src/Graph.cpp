#include "Graph.h"

using namespace std;

Graph::Graph() : vertexesCount(0), edgesSize(0), edgesAllocatedSize(1) {
    this->edges = new Edge[edgesAllocatedSize];
}

// copy constructor
Graph::Graph(const Graph &source)
    : vertexesCount(source.vertexesCount),
      edgesSize(source.edgesSize),
      edgesAllocatedSize(source.edgesAllocatedSize) {
    this->edges = new Edge[source.edgesAllocatedSize];
    for (int i = 0; i < source.edgesSize; i++) {
        this->edges[i] = source.edges[i];
    }
}

Graph::~Graph() { delete[] edges; }

// operator ==
Graph &Graph::operator=(const Graph &source) {
    if (&source == this) {
        return *this;
    }
    vertexesCount = source.vertexesCount;
    edgesSize = source.edgesSize;
    edgesAllocatedSize = source.edgesAllocatedSize;
    this->edges = new Edge[source.edgesAllocatedSize];
    for (int i = 0; i < source.edgesSize; i++) {
        this->edges[i] = source.edges[i];
    }
    return *this;
}

bool Graph::loadFromFile(const string &path) {
    ifstream file;
    file.open(path);

    if (!file.is_open()) {
        cerr << "Can't open input file." << endl;
        return false;
    }

    int number;

    file >> vertexesCount;

    int edgeFromVertexId = 0;
    int edgeToVertexId = 0;

    while (file >> number) {
        if (number != 0) {
            addEdge(Edge(edgeFromVertexId, edgeToVertexId, number));
        }
        edgeToVertexId = (edgeToVertexId + 1) % vertexesCount;
        if (edgeToVertexId == 0) {
            edgeFromVertexId++;
        }
    }

    return edgeFromVertexId == vertexesCount && edgeToVertexId == 0;
}

void Graph::addEdge(Edge edge) {
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

ostream &operator<<(ostream &os, const Graph &graph) {
    os << "vertexes count: " << graph.vertexesCount << endl;
    os << "edges:" << endl;
    for (int i = 0; i < graph.edgesSize; i++) {
        os << "vId1: " << graph.edges[i].vertexId1
           << ", vId2: " << graph.edges[i].vertexId2
           << ", weight: " << graph.edges[i].weight << endl;
    }
    return os;
}