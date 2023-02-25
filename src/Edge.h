struct Edge {
    Edge(int vertexId1, int vertexId2, int weight)
        : vertexId1(vertexId1), vertexId2(vertexId2), weight(weight){};

    Edge() : vertexId1(0), vertexId2(0), weight(0){};

    int vertexId1;
    int vertexId2;
    int weight;
};