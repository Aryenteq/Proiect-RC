#include "map.h"

void Graph::addEdge(int source, int destination, int length, std::string streetName, int speedLimit)
{
    if(source>destination)
        swap(source,destination);
    if(destination>this->vertices)
    {
        this->vertices=destination+1;
        this->adjacencyList.resize(this->vertices);
    }
    Edge edge = {destination, length, streetName, speedLimit};
    adjacencyList[source].push_back(edge);
}

void Graph::displayGraph()
{
    for (int i = 0; i < vertices; ++i) 
    {
        printf("Vertex %d: ", i);
        for (const Edge &edge : adjacencyList[i]) 
        {
            //printf("(Dest: %d, Length: %.2f, Street: %s, Speed: %d) ",
            printf("(Dest: %d, Length: %d, Street: %s, Speed: %d) ",
            edge.destination, edge.length, edge.streetName.c_str(), edge.speedLimit);
        }
        printf("\n");
    }
}
