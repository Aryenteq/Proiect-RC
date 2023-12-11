struct Edge {
    int destination;     // Vertex at the other end of the edge
    int length;
    std::string streetName;
    int speedLimit;
};


class Graph {
private:
    int vertices;
    std::vector<std::vector<Edge>> adjacencyList;
public:
    Graph(int V) : vertices(V), adjacencyList(V) {}
    void addEdge(int source, int destination, int length, std::string streetName, int speedLimit);
    void displayGraph();
};