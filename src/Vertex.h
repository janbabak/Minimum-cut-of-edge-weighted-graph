struct Vertex {
    Vertex(int id) : id(id), visited(false), isGroupX(false){};

    Vertex() : id(0), visited(false), isGroupX(false){};

    int id;
    bool visited;
    bool isGroupX;
};