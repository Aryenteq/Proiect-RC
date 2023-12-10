#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"

void createGraph()
{
    // Graph to represent the map
    Graph mapGraph(1);

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
        rapidxml::xml_node<>* lenghtNode = edgeNode->first_node("lenght");
        rapidxml::xml_node<>* speedlimitNode = edgeNode->first_node("speedlimit");

        // Add the nodes from XML in the graph
        if (fromNode && toNode && nameNode && lenghtNode && speedlimitNode) {
            mapGraph.addEdge(
                atoi(fromNode->value()),
                atoi(toNode->value()),
                atoi(lenghtNode->value()),
                nameNode->value(),
                atoi(speedlimitNode->value())   
            );
        }
    }
    
    // Display the graph
    mapGraph.displayGraph();
}

void processCommand()
{
    printf("hei\n");
}