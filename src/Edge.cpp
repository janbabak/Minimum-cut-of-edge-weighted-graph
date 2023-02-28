struct Edge {
    int vertexId1;
    int vertexId2;
    int weight;

    Edge(int vertexId1, int vertexId2, int weight)
        : vertexId1(vertexId1), vertexId2(vertexId2), weight(weight){};

    Edge() : vertexId1(0), vertexId2(0), weight(0){};

    bool operator==(const Edge& rhs) const {
        return this->vertexId1 == rhs.vertexId1 && this->vertexId2 == rhs.vertexId2 &&
               this->weight == rhs.weight;
    }
};