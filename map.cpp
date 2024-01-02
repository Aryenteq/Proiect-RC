#include "map.h"
#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_print.hpp"

#include <queue>
#include <limits>
#include <algorithm> // reverse

Graph mapGraph(1);
std::vector<int> shortestPath;

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


double Graph::Dijkstra(int startNode, int endNode, std::vector<int>& shortestPath)
{
    shortestPath.clear();
    std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, std::greater<std::pair<double, int>>> pq;
    std::vector<double> estimatedTime(vertices, std::numeric_limits<double>::infinity());
    std::vector<int> parent(vertices, -1);
    
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
                parent[v] = u; 
                pq.push(std::make_pair(estimatedTime[v], v));
            }
        }
    }
    // Reconstruct shortest path
    int currentVertex = endNode;
    while (currentVertex != -1)
    {
        shortestPath.push_back(currentVertex);
        currentVertex = parent[currentVertex];
    }
    std::reverse(shortestPath.begin(), shortestPath.end());
    return estimatedTime[endNode];
}

int Graph::lastNode() const 
{
    int maxNodeNumber = -1;
    for (const auto& edges : adjacencyList)
        for (const auto& edge : edges)
            maxNodeNumber = std::max({maxNodeNumber, edge.destination});
    return maxNodeNumber;
}

bool Graph::verifySpeed(int from, int to, int speed)
{
    for (const auto& edge : adjacencyList[from]) 
    {
        if (edge.destination == to)
        {
            std::cout<<speed<<" "<<edge.speedLimit;
            return speed > edge.speedLimit;
        }
    }
    return false; 
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

// Returns 1 if the averageSpeed was changed
// 0 otherwise (the speed was already too low to change furthermore)
bool changeAverageSpeed(int from, int to, int delta)
{
    rapidxml::file<> xmlFile("map.xml");
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    rapidxml::xml_node<> *root = doc.first_node();
    for (rapidxml::xml_node<> *edge = root->first_node("edge"); edge; edge = edge->next_sibling("edge"))
    {
        int from_value = std::atoi(edge->first_node("from")->value());
        int to_value = std::atoi(edge->first_node("to")->value());

        if (from_value == from && to_value == to)
        {
            int avgSpeed = std::atoi(edge->first_node("averagespeed")->value()) + delta;

            // Don't do any changes if the value becomes too small
            if(avgSpeed>=10)
            {
                edge->first_node("averagespeed")->first_node()->value(doc.allocate_string(std::to_string(avgSpeed).c_str()));
                std::ofstream outputFile("map.xml");
                outputFile << doc;
                outputFile.close();
                return 1;
            }
            return 0;
        }
    }
    // Edge not found
    return 0;
}