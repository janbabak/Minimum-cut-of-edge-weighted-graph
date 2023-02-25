#include <iostream>

using namespace std;

struct Vertex {
    Vertex(int id) : id(id), visited(false), isGroupX(false) {
        neighborsSize = 0;
        neighborsAllocatedSize = 1;
        neighbors = new int[neighborsAllocatedSize];
    };

    Vertex() : id(0), visited(false), isGroupX(false) {
        neighbors = new int[0];
        neighborsSize = 0;
        neighborsAllocatedSize = 1;
        neighbors = new int[neighborsAllocatedSize];
    };

    // copy constructor
    Vertex(const Vertex &source)
        : id(source.id),
          visited(source.visited),
          isGroupX(source.isGroupX),
          neighborsSize(source.neighborsSize),
          neighborsAllocatedSize(source.neighborsAllocatedSize) {
        neighbors = new int[source.neighborsAllocatedSize];
        for (int i = 0; i < neighborsSize; i++) {
            neighbors[i] = source.neighbors[i];
        }
    }

    ~Vertex() {
        if (neighborsAllocatedSize > 0) {
            delete[] neighbors;
        }
    }

    // operator =
    Vertex &operator=(const Vertex &source) {
        if (&source == this) {
            return *this;
        }
        id = source.id;
        visited = source.visited;
        isGroupX = source.isGroupX;
        neighborsSize = source.neighborsSize;
        neighborsAllocatedSize = source.neighborsAllocatedSize;
        neighbors = new int[source.neighborsAllocatedSize];
        for (int i = 0; i < neighborsSize; i++) {
            neighbors[i] = source.neighbors[i];
        }
        return *this;
    }

    void addNeighbor(int neighbor) {
        for (int i = 0; i < neighborsSize; i++) {
            if (neighbors[i] == neighbor) {
                return;  // already contains neighbour
            }
        }

        if (neighborsSize == neighborsAllocatedSize) {
            neighborsAllocatedSize *= 2;
            int *tmp = new int[neighborsAllocatedSize];
            for (int i = 0; i < neighborsSize; i++) {
                tmp[i] = neighbors[i];
            }
            delete[] neighbors;
            neighbors = tmp;
        }

        // add edge to the end
        neighbors[neighborsSize] = neighbor;
        neighborsSize++;
    }

    int id;
    bool visited;
    bool isGroupX;
    int *neighbors;
    int neighborsSize;
    int neighborsAllocatedSize;
};