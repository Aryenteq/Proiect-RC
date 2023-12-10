// Structure to represent an edge
struct Edge {
    int destination;     // Vertex at the other end of the edge
    int length;       // Length of the street
    std::string streetName;   // Name of the street
    int speedLimit;      // Speed limit on the street
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