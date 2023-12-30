#include <algorithm> // remove_if
#include <mutex>
#include <chrono>
#include <thread>
#include <future>

std::mutex mtx; 
struct userInformation
{
  bool shouldStop;
  bool showHazards;
  bool showWeather;
  bool showSpeedLimit;
  bool admin;
  char* connectedUsername; // Deallocation not taken care of! Could create problems?
  Car individualCar;
};

typedef struct thData{
	int idThread;
	int cl;
    userInformation userInfo;
}thData;

// Removed the pointer because the passed thData* is local, being deleted prematurly
// Therefore, the activeThreads pointers are invalid
// ATM doing a copy of the thData* td, not efficient but works
std::vector<thData> activeThreads;

std::mutex weatherMutex;
std::string globalWeather("Sunny");


#include "users.cpp"

void initialiseThread(thData* td, int &i, int &client)
{
    td->idThread=i++;
	td->cl=client;
    td->userInfo.shouldStop = false;
    td->userInfo.showHazards = false;
    td->userInfo.showWeather = false;
    td->userInfo.showSpeedLimit = false;
    td->userInfo.admin = false;
    td->userInfo.connectedUsername = nullptr;

    thData user;
    user.idThread = td->idThread;
    user.cl = client;
    user.userInfo.shouldStop = false;
    user.userInfo.showHazards = false;
    user.userInfo.showWeather = false;
    user.userInfo.showSpeedLimit = false;
    user.userInfo.admin = false;
    user.userInfo.connectedUsername = nullptr;
    activeThreads.push_back(user);
}

void broadcastMessage(const std::string& messageText, const std::string& type, int senderID)
{
    std::lock_guard<std::mutex> lock(mtx);
    for (thData td : activeThreads)
    {
        // Skip the sender thread
        if (td.idThread == senderID)
            continue;
        if(type=="hazard")
        {
            if(td.userInfo.showHazards)
                if (write(td.cl, messageText.c_str(), messageText.size() + 1) <= 0)
                    std::cerr << "Error writing to client: " << strerror(errno) << std::endl;
        } else if(type=="weather")
        {
             if(td.userInfo.showWeather)
                if (write(td.cl, messageText.c_str(), messageText.size() + 1) <= 0)
                    std::cerr << "Error writing to client: " << strerror(errno) << std::endl;
        } else if(type=="priority") // New gas station - everyone should know this
        {
            if (write(td.cl, messageText.c_str(), messageText.size() + 1) <= 0)
                    std::cerr << "Error writing to client: " << strerror(errno) << std::endl;
        }
    }
}

// Functions with parameters
void logIn(char*, thData&);
void signUp(char*, thData&);
void createHazard(char*, thData&);
void removeHazard(char*, thData &);
void createGasStation(char*, thData&);
void changeAccountSettings(char*, thData&);
void changeWeather(char*, thData&);

// Function without parameters
void showWeather(char*, thData&);
void showActiveThreads(char*, thData&);
void showHazards(char*, thData&);
void showGasStations(char*, thData&);
void userStatus(char*, thData&);
void closeClient(char*, thData&);
void defaultOption(char*, thData&);

void processCommand(char* buf, thData &tdL) 
{
    // Define an array of function pointers
    void (*functionArray[])(char*, thData&) = {logIn, 
                                                signUp,
                                                createHazard,
                                                removeHazard,
                                                createGasStation,
                                                changeAccountSettings,
                                                changeWeather,
                                                showWeather,
                                                showActiveThreads,
                                                showHazards,
                                                showGasStations,
                                                userStatus,
                                                closeClient,
                                                defaultOption};
    const char* inputPossibilities[] = {"log in", 
                                        "sign up", 
                                        "hazard",
                                        "remove",
                                        "gas",
                                        "set",
                                        "weather",
                                        "meteo",
                                        "threads",
                                        "dangers",
                                        "stations",
                                        "status",
                                        "exit"};

    size_t noOfFunctions = sizeof(functionArray) / sizeof(functionArray[0]);
    size_t index;
    
    for (index = 0; index < noOfFunctions; ++index)
    {
        if(index == noOfFunctions - 1)
            break;
        if (index <= 6) // To change when adding a new function with parameters
        {
            if (strncmp(buf, inputPossibilities[index], strlen(inputPossibilities[index])) == 0 && 
            strlen(buf) > strlen(inputPossibilities[index]) && 
            buf[strlen(inputPossibilities[index])] == ' ')
                break;
        }
        else
        {
            // For other commands (w/o parameters)
            if (strcmp(buf, inputPossibilities[index]) == 0)
                break;
        }
    }
    (*functionArray[index])(buf, tdL);
}

void logIn(char* buf, thData &tdL) 
{
    strcpy(buf, buf+7); // Get rid of "log in "
    char* p = strtok(buf, " ");
    if(p == nullptr)
    {
        strcpy(buf, "Sintax: log in <name> <password>");
        return;
    }
    else
    {
        std::string username(p);
        p = strtok(nullptr, " ");
        if(p == nullptr)
        {
            strcpy(buf, "Sintax: log in <name> <password>");
            return;
        }
        else
        {
            std::string password(p);
            connectUser(buf, tdL, username, password);
        } 
    }
}

void signUp(char* buf, thData &tdL)
{
    strcpy(buf, buf+8); // Get rid of "sign up "
    char* p = strtok(buf, " ");
    if(p == nullptr)
    {
        strcpy(buf, "Sintax: sign up <name> <password>");
        return;
    }
    else
    {
        std::string username(p);
        p = strtok(nullptr, " ");
        if(p == nullptr)
        {
            strcpy(buf, "Sintax: sign up <name> <password>");
            return;
        }
        else
        {
            std::string password(p);
            int errors = 0;
            createUser(buf, tdL, username, password, errors);
            if(errors==0)
                connectUser(buf, tdL, username, password);
        }
    }
}

void createHazard(char* buf, thData& tdL)
{
    strcpy(buf, buf+7); // Get rid of "hazard "
    std::vector<std::string> parameters;
    char* p = strtok(buf, " ");

    while (p != nullptr) 
    {
        parameters.push_back(p);
        p = strtok(nullptr, " ");
    }
    if(parameters.size() != 3)
    {
        strcpy(buf, "Syntax: hazard <from> <to> <type>");
        return;
    }
    if(parameters[2]!="pathole" && parameters[2]!="accident" && 
       parameters[2]!="construction" && parameters[2]!="other")
    {
        strcpy(buf, "Available hazards: pathole, accident, construction, other");
        return;
    }
    if(parameters[0]>parameters[1])
        std::swap(parameters[0], parameters[1]);
    if(isValidInteger(buf, parameters[0]) && isValidInteger(buf, parameters[1]))
    {
        int from = std::stoi(parameters[0]);
        int to = std::stoi(parameters[1]);
        std::string edgeName = mapGraph.getEdgeName(from, to);
        if(edgeName == "")
        {
            strcpy(buf, "Edge doesn't exist");
            return;
        }
        try 
        {
            rapidxml::file<> xmlFile("hazards.xml");
            rapidxml::xml_document<> doc;
            doc.parse<0>(xmlFile.data());

            rapidxml::xml_node<>* newHazardNode = doc.allocate_node(rapidxml::node_element, "hazard");
            rapidxml::xml_node<>* fromNode = doc.allocate_node(rapidxml::node_element, "from", doc.allocate_string(parameters[0].c_str()));
            rapidxml::xml_node<>* toNode = doc.allocate_node(rapidxml::node_element, "to", doc.allocate_string(parameters[1].c_str()));
            rapidxml::xml_node<>* typeNode = doc.allocate_node(rapidxml::node_element, "type", doc.allocate_string(parameters[2].c_str()));
                        
            newHazardNode->append_node(fromNode);
            newHazardNode->append_node(toNode);
            newHazardNode->append_node(typeNode);
            doc.first_node()->append_node(newHazardNode);

            std::ofstream outputFile("hazards.xml");
            outputFile << doc;
            outputFile.close();

            // Edit the map.xml file to reduce the speedlimit
            // "pathole" and "other" won't affect the driving speed
            bool speedChanged = 0;
            if(parameters[2]=="accident")
                speedChanged = changeAverageSpeed(from, to, -30);
            else if(parameters[2]=="construction")
                speedChanged = changeAverageSpeed(from, to, -20);

            std::string messageText("New hazard: ");
            messageText += parameters[2];
            messageText += " on ";
            messageText += edgeName;
            messageText += ".";
            broadcastMessage(messageText, std::string("hazard"), tdL.idThread);
            strcpy(buf, "Hazard added");
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
}

void removeHazard(char* buf, thData &tdL)
{
    strcpy(buf, buf+7); // Get rid of "remove "
    std::vector<std::string> parameters;
    char* p = strtok(buf, " ");

    while (p != nullptr)
    {
        parameters.push_back(p);
        p = strtok(nullptr, " ");
    }
    if(parameters.size() != 3)
    {
        strcpy(buf, "Syntax: remove <from> <to> <type>");
        return;
    }
    if(parameters[2]!="pathole" && parameters[2]!="accident" && 
       parameters[2]!="construction" && parameters[2]!="other")
    {
        strcpy(buf, "Available hazards: pathole, accident, construction, other");
        return;
    }
    if(parameters[0]>parameters[1])
        std::swap(parameters[0], parameters[1]);
    if(isValidInteger(buf, parameters[0]) && isValidInteger(buf, parameters[1]))
    {
        int from = std::stoi(parameters[0]);
        int to = std::stoi(parameters[1]);
        std::string edgeName = mapGraph.getEdgeName(from, to);
        if(edgeName == "")
        {
            strcpy(buf, "Edge doesn't exist");
            return;
        }
        try 
        {
        rapidxml::file<> xmlFile("hazards.xml");
        rapidxml::xml_document<> doc;
        doc.parse<0>(xmlFile.data());

        // Loooong searching process
        rapidxml::xml_node<>* hazardsNode = doc.first_node("hazards");
        if (hazardsNode) 
        {
        for (rapidxml::xml_node<>* hazardNode = doc.first_node("hazards")->first_node("hazard"); hazardNode; hazardNode = hazardNode->next_sibling("hazard"))
        {
            rapidxml::xml_node<>* fromNode = hazardNode->first_node("from");
            rapidxml::xml_node<>* toNode = hazardNode->first_node("to");
            rapidxml::xml_node<>* typeNode = hazardNode->first_node("type");

            if (fromNode && fromNode->value() == parameters[0] &&
                toNode && toNode->value() == parameters[1] &&
                typeNode && typeNode->value() == parameters[2]) 
            { // Hazard found
                doc.first_node()->remove_node(hazardNode);
                std::ofstream outputFile("hazards.xml");
                outputFile << doc;
                outputFile.close();
                
                bool speedChanged = 0;
                if(parameters[2]=="accident")
                    speedChanged = changeAverageSpeed(from, to, 30);
                else if(parameters[2]=="construction")
                    speedChanged = changeAverageSpeed(from, to, 20);

                std::string messageText("Hazard removed: ");
                messageText += parameters[2];
                messageText += " on ";
                messageText += edgeName;
                messageText += ".";
                broadcastMessage(messageText, std::string("hazard"), tdL.idThread);
                strcpy(buf, "Hazard removed");
                return;
            }
        }
        strcpy(buf, "Hazard doesn't exist");
        }
    } catch (const std::exception& e) 
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    }
}

void createGasStation(char* buf, thData& tdL)
{
    if(!tdL.userInfo.admin)
    {
        strcpy(buf, "You do not have admin permissions!");
        return;
    }
    strcpy(buf, buf+4); // Get rid of "gas "
    std::vector<std::string> parameters;
    char* p = strtok(buf, " ");

    while (p != nullptr) 
    {
        parameters.push_back(p);
        p = strtok(nullptr, " ");
    }
    if(parameters.size() != 5)
    {
        strcpy(buf, "Syntax: gas <name> <from> <to> <gas price> <diesel price>");
        return;
    }
    if(parameters[1]>parameters[2])
        std::swap(parameters[1], parameters[2]);
    if(isValidInteger(buf, parameters[1]) && isValidInteger(buf, parameters[2]) &&
       isValidDouble(buf, parameters[3]) && isValidDouble(buf, parameters[4]))
    {
        std::string edgeName = mapGraph.getEdgeName(std::stoi(parameters[1]), std::stoi(parameters[2]));
        if(edgeName == "")
        {
            strcpy(buf, "Street doesn't exist");
            return;
        }
        try 
        {
            rapidxml::file<> xmlFile("gas.xml");
            rapidxml::xml_document<> doc;
            doc.parse<0>(xmlFile.data());

            rapidxml::xml_node<>* newStationNode = doc.allocate_node(rapidxml::node_element, "station");
            rapidxml::xml_node<>* nameNode = doc.allocate_node(rapidxml::node_element, "name", doc.allocate_string(parameters[0].c_str()));
            rapidxml::xml_node<>* fromNode = doc.allocate_node(rapidxml::node_element, "from", doc.allocate_string(parameters[1].c_str()));
            rapidxml::xml_node<>* toNode = doc.allocate_node(rapidxml::node_element, "to", doc.allocate_string(parameters[2].c_str()));
            rapidxml::xml_node<>* gasNode = doc.allocate_node(rapidxml::node_element, "gas", doc.allocate_string(parameters[3].c_str()));
            rapidxml::xml_node<>* dieselNode = doc.allocate_node(rapidxml::node_element, "diesel", doc.allocate_string(parameters[4].c_str()));

            newStationNode->append_node(nameNode);
            newStationNode->append_node(fromNode);
            newStationNode->append_node(toNode);
            newStationNode->append_node(gasNode);
            newStationNode->append_node(dieselNode);
            doc.first_node()->append_node(newStationNode);

            std::ofstream outputFile("gas.xml");
            outputFile << doc;
            outputFile.close();

            std::string messageText("New gas station: ");
            messageText += parameters[0];
            messageText += " on ";
            messageText += edgeName;
            messageText += ".";
            broadcastMessage(messageText, std::string("priority"), tdL.idThread);
            strcpy(buf, "Gas station added");
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
}

void changeAccountSettings(char* buf, thData& tdL)
{
    strcpy(buf, buf+4); // Get rid of "set "
    char* p = strtok(buf, " ");
    if(p == nullptr)
    {
        strcpy(buf, "Syntax: set <setting> <0/1>");
        return;
    }
    std::string setting(p);
    p = strtok(nullptr, " ");
    if(p == nullptr)
    {
        strcpy(buf, "Syntax: set <setting> <0/1>");
        return;
    }
    if(setting!="hazards" && setting!="weather" && setting!="speedlimit" && setting!="admin")
    {
        strcpy(buf, "Available settings: hazards, weather, speedlimit, admin");
        return;
    }
    std::string value(p);
    if(isValidInteger(buf, value))
    {
        int value = stoi(p);
        if(value!=0 && value!=1)
        {
            strcpy(buf, "Use 0 for false, 1 for true. Try again.");
            return;
        }
        for (thData& td : activeThreads)
            if (td.idThread == tdL.idThread)
            {
                if(setting=="hazards")
                tdL.userInfo.showHazards = value, td.userInfo.showHazards = value;
                else if(setting=="weather")
                tdL.userInfo.showWeather = value, td.userInfo.showWeather = value;
                else if(setting=="speedlimit")
                tdL.userInfo.showSpeedLimit = value, td.userInfo.showSpeedLimit = value;
                else if(setting=="admin")
                tdL.userInfo.admin = value, td.userInfo.admin = value;
                
                strcpy(buf, "Setting modified. Be aware that you aren't logged in.");
                // Save the settings to the account
                if(tdL.userInfo.connectedUsername != nullptr)
                    updateUserSettings(buf, tdL.userInfo.connectedUsername, setting, value);
            }
    }
}

void changeWeather(char* buf, thData& tdL)
{
    if(!tdL.userInfo.admin)
    {
        strcpy(buf, "You do not have admin permissions!");
        return;
    }
    std::lock_guard<std::mutex> lock(weatherMutex);

    strcpy(buf, buf+8); // Get rid of "weather "
    globalWeather = buf;
    std::string messageText("Weather changed to: ");
    messageText += globalWeather;
    broadcastMessage(messageText, std::string("weather"), tdL.idThread);
    strcpy(buf, "Weather changed!");
}

void showWeather(char* buf, thData& tdL)
{
    std::string messageText("Current weather: ");
    messageText += globalWeather;
    strcpy(buf, messageText.c_str());
}

void showActiveThreads(char* buf, thData &tdL) 
{
    std::string result("Active threads: ");
    for(int i=0; i<activeThreads.size(); i++) 
        result += std::to_string(activeThreads[i].idThread) + " ";
    strcpy(buf, result.c_str());
}

void showHazards(char* buf, thData& tdL)
{
    rapidxml::file<> xmlFile("hazards.xml");
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    std::string hazardsInfo("Active hazards: ");

    rapidxml::xml_node<>* hazardsNode = doc.first_node("hazards");
    if (hazardsNode) 
    {
        for (rapidxml::xml_node<>* hazardNode = hazardsNode->first_node("hazard"); hazardNode; hazardNode = hazardNode->next_sibling("hazard")) 
        {
            rapidxml::xml_node<>* fromNode = hazardNode->first_node("from");
            rapidxml::xml_node<>* toNode = hazardNode->first_node("to");
            rapidxml::xml_node<>* typeNode = hazardNode->first_node("type");

            // Concatenate hazard information into the buffer
            std::string streetName = mapGraph.getEdgeName(std::stoi(fromNode->value()), std::stoi(toNode->value()));
            hazardsInfo += std::string("\n") + typeNode->value() + " on " + streetName;
        }
        strcpy(buf, hazardsInfo.c_str());
    }
    else
    {
        strcpy(buf, "XML file is wrongly formatted.");
    }
}

void showGasStations(char* buf, thData &tdL)
{
    rapidxml::file<> xmlFile("gas.xml");
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    std::string stationsInfo("Gas stations: ");

    rapidxml::xml_node<>* stationsNode = doc.first_node("stations");
    if (stationsNode) 
    {
        for (rapidxml::xml_node<>* stationNode = stationsNode->first_node("station"); stationNode; stationNode = stationNode->next_sibling("station")) 
        {
            rapidxml::xml_node<>* nameNode = stationNode->first_node("name");
            rapidxml::xml_node<>* fromNode = stationNode->first_node("from");
            rapidxml::xml_node<>* toNode = stationNode->first_node("to");
            rapidxml::xml_node<>* gasNode = stationNode->first_node("gas");
            rapidxml::xml_node<>* dieselNode = stationNode->first_node("diesel");

            // Concatenate station information into the buffer
            std::string streetName = mapGraph.getEdgeName(std::stoi(fromNode->value()), std::stoi(toNode->value()));
            stationsInfo += std::string("\n") + nameNode->value() + " on " + streetName + " -- gas price: " + gasNode->value() + ", diesel price: " + dieselNode->value();

        }
        std::cout<<stationsInfo;
        strcpy(buf, stationsInfo.c_str());
        //strcpy(buf, "Info printed in server terminal.");
    }
    else
    {
        strcpy(buf, "XML file is wrongly formatted.");
    }
}

void userStatus(char* buf, thData &tdL)
{
    std::string result;
    result += "\nID Thread: " + std::to_string(tdL.idThread) + std::string("\n") + 
    "shouldStop: " + std::to_string(tdL.userInfo.shouldStop) + std::string("\n") + 
    "showHazards: " + std::to_string(tdL.userInfo.showHazards) + std::string("\n") + 
    "showWeather: " + std::to_string(tdL.userInfo.showWeather) + std::string("\n") + 
    "showSpeedLimit: " + std::to_string(tdL.userInfo.showSpeedLimit) + std::string("\n") + 
    "admin: " + std::to_string(tdL.userInfo.admin) + std::string("\n") + "connectedUsername: ";
    if(tdL.userInfo.connectedUsername)
        result += tdL.userInfo.connectedUsername;
    else
        result += "none";
    strcpy(buf, result.c_str());
}

void closeClient(char* buf, thData &tdL) 
{
    tdL.userInfo.shouldStop = true;

    // Lambda function because I need to compare 2 elements with custom ==
    activeThreads.erase(
    std::remove_if(activeThreads.begin(), activeThreads.end(),
                   [tdL](const thData& element) { return element.idThread == tdL.idThread; }),
    activeThreads.end());
    strcpy(buf, "Closing");
}

void defaultOption(char* buf, thData &tdL) 
{
    strcpy(buf, "Invalid command");
}


