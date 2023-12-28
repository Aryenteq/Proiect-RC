#include "map.h"
#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_print.hpp"

#include <queue>
#include <limits>
#include <algorithm> // reverse

Graph mapGraph(1);

void Graph::addEdge(int source, int destination, int length, std::string streetName, int speedLimit, int averageSpeed)
{
    if(source>destination)
        std::swap(source,destination);
    if(destination>this->vertices-1)
    {
        this->vertices=destination+1;
        this->adjacencyList.resize(this->vertices);
    }
    Edge edge = {source, length, streetName, speedLimit, averageSpeed};
    adjacencyList[destination].push_back(edge);
    edge = {destination, length, streetName, speedLimit, averageSpeed};
    adjacencyList[source].push_back(edge);
}

void Graph::displayGraph()
{
    for (int i = 0; i < vertices; ++i) 
    {
        printf("Vertex %d: ", i);
        for (const Edge &edge : adjacencyList[i]) 
        {
            printf("(Dest: %d, Length: %d, Street: %s, Speed: %d, Avg. speed: %d) ",
            edge.destination, edge.length, edge.streetName.c_str(), edge.speedLimit, edge.averageSpeed);
        }
        printf("\n");
    }
}


double Graph::Dijkstra(int startNode, int endNode, std::vector<int>& shortestPath) {
    shortestPath.clear();
    // Priority queue to store vertices with minimum estimated times
    std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, std::greater<std::pair<double, int>>> pq;

    // Vector to store estimated times to reach each node
    std::vector<double> estimatedTime(vertices, std::numeric_limits<double>::infinity());

    // Vector to store parent vertex for each vertex in the shortest path
    std::vector<int> parent(vertices, -1);

    // Initialize starting node with estimated time 0
    pq.push(std::make_pair(0.0, startNode));
    estimatedTime[startNode] = 0.0;

    // Dijkstra's algorithm
    while (!pq.empty()) {
        int u = pq.top().second;
        pq.pop();

        for (const Edge& edge : adjacencyList[u]) 
        {
            int v = edge.destination;
            double edgeTime = static_cast<double>(edge.length) / edge.averageSpeed;

            if (estimatedTime[v] > estimatedTime[u] + edgeTime) 
            {
                estimatedTime[v] = estimatedTime[u] + edgeTime;
                parent[v] = u;  // Update parent for vertex v
                pq.push(std::make_pair(estimatedTime[v], v));
            }
        }
    }
    // Reconstruct the shortest path from endNode to startNode
    int currentVertex = endNode;
    while (currentVertex != -1)
    {
        shortestPath.push_back(currentVertex);
        currentVertex = parent[currentVertex];
    }

    // Reverse the path to get it from startNode to endNode
    std::reverse(shortestPath.begin(), shortestPath.end());

    // The estimated time to reach the end node
    return estimatedTime[endNode];
}

std::string Graph::getEdgeName(int from, int to)
{
    if(from>vertices || to>vertices)
        return "";
    for (const Edge& edge : adjacencyList[from]) 
    {
        if (edge.destination == to)
            return edge.streetName;
    }
    return "";
}


void createGraph()
{
    // Load the XML file into memory
    rapidxml::file<> xmlFile("map.xml");
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    // Root node
    rapidxml::xml_node<>* root = doc.first_node("graph");

    for (rapidxml::xml_node<>* edgeNode = root->first_node("edge"); edgeNode; edgeNode = edgeNode->next_sibling("edge")) {
        // Access all the attributes
        rapidxml::xml_node<>* fromNode = edgeNode->first_node("from");
        rapidxml::xml_node<>* toNode = edgeNode->first_node("to");
        rapidxml::xml_node<>* nameNode = edgeNode->first_node("name");
        rapidxml::xml_node<>* lengthNode = edgeNode->first_node("length");
        rapidxml::xml_node<>* speedlimitNode = edgeNode->first_node("speedlimit");
        rapidxml::xml_node<>* averagespeedNode = edgeNode->first_node("averagespeed");

        // Add the nodes from XML in the graph
        if (fromNode && toNode && nameNode && lengthNode && speedlimitNode && averagespeedNode) {
            mapGraph.addEdge(
                atoi(fromNode->value()),
                atoi(toNode->value()),
                atoi(lengthNode->value()),
                nameNode->value(),
                atoi(speedlimitNode->value()),
                atoi(averagespeedNode->value())   
            );
        }
    }
    
    // Display the graph
    mapGraph.displayGraph();
}
