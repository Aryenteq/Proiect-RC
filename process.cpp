#include <algorithm> // remove_if
#include <mutex>

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

void broadcastMessage(const std::string& messageText, const std::string& messageType, int senderID)
{
    std::lock_guard<std::mutex> lock(mtx);
    for (thData td : activeThreads)
    {
        // Skip the sender thread
        if (td.idThread == senderID)
            continue;
        if(td.userInfo.showHazards)
            if (write(td.cl, messageText.c_str(), messageText.size() + 1) <= 0)
                std::cerr << "Error writing to client: " << strerror(errno) << std::endl;
    }
}

// Functions with parameters
void logIn(char*, thData&);
void signUp(char*, thData&);
void createHazard(char*, thData&);
void changeSettings(char*, thData&);

// Function without parameters
void showActiveThreads(char*, thData&);
void userStatus(char*, thData&);
void closeClient(char*, thData&);
void defaultOption(char*, thData&);

void processCommand(char* buf, thData &tdL) 
{
    // Define an array of function pointers
    void (*functionArray[])(char*, thData&) = {logIn, 
                                                signUp,
                                                createHazard,
                                                changeSettings,
                                                showActiveThreads,
                                                userStatus,
                                                closeClient,
                                                defaultOption};
    const char* inputPossibilities[] = {"log in", 
                                        "sign up", 
                                        "hazard",
                                        "set",
                                        "threads",
                                        "status",
                                        "exit"};

    size_t noOfFunctions = sizeof(functionArray) / sizeof(functionArray[0]);
    size_t index;
    bool foundCommand = false;
    
    for (index = 0; index < noOfFunctions; ++index) 
    {
        if(index == noOfFunctions - 1)
            break;
        if (index <= 3) // To change when adding a new function with parameters
        {
            if (strncmp(buf, inputPossibilities[index], strlen(inputPossibilities[index])) == 0 && 
            strlen(buf) > strlen(inputPossibilities[index]) && 
            buf[strlen(inputPossibilities[index])] == ' ')
            {
                foundCommand = true;
                break;
            }
        } 
        else 
        {
            // For other commands (w/o parameters)
            if (strcmp(buf, inputPossibilities[index]) == 0) 
            {
                foundCommand = true;
                break;
            }
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
    char* p = strtok(buf, " ");
    if(p == nullptr)
    {
        strcpy(buf, "Syntax: hazard <from> <to> <type>");
        return;
    }
    std::string fromString(p);
    p = strtok(nullptr, " ");
    if(p == nullptr)
    {
        strcpy(buf, "Syntax: hazard <from> <to> <type>");
        return;
    }
    std::string toString(p);
    p = strtok(nullptr, " ");
    if(p == nullptr)
    {
        strcpy(buf, "Syntax: hazard <from> <to> <type>");
        return;
    }
    std::string type(p);
    if(type!="pathole" && type!="accident" && type!="construction" && type!="other")
    {
        strcpy(buf, "Available hazards: pathole, accident, construction, other");
        return;
    }
    if(fromString>toString)
        std::swap(toString, fromString);
    if(isValidInteger(buf, fromString) && isValidInteger(buf, toString))
    {
        int from = std::stoi(fromString);
        int to = std::stoi(toString);
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
            rapidxml::xml_node<>* fromNode = doc.allocate_node(rapidxml::node_element, "from", doc.allocate_string(fromString.c_str()));
            rapidxml::xml_node<>* toNode = doc.allocate_node(rapidxml::node_element, "to", doc.allocate_string(toString.c_str()));
            rapidxml::xml_node<>* typeNode = doc.allocate_node(rapidxml::node_element, "type", doc.allocate_string(type.c_str()));
                        
            newHazardNode->append_node(fromNode);
            newHazardNode->append_node(toNode);
            newHazardNode->append_node(typeNode);
            doc.first_node()->append_node(newHazardNode);

            std::ofstream outputFile("hazards.xml");
            outputFile << doc;
            outputFile.close();

            std::string messageText("New hazard: ");
            messageText += type;
            messageText += " on ";
            messageText += edgeName;
            messageText += ".";
            std::string messageType("Hazard");
            broadcastMessage(messageText, messageType, tdL.idThread);
            strcpy(buf, "Hazard added");
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
}

void changeSettings(char* buf, thData& tdL)
{
    strcpy(buf, buf+4); // Get rid of "set "
    char* p = strtok(buf, " ");
    if(p == nullptr)
    {
        strcpy(buf, "Sintax: set <setting> <0/1>");
        return;
    }
    std::string setting(p);
    p = strtok(nullptr, " ");
    if(p == nullptr)
    {
        strcpy(buf, "Sintax: set <setting> <0/1>");
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

void showActiveThreads(char* buf, thData &tdL) {
    char result[100];
    strcpy(result, "Active threads: ");
    for(int i=0; i<activeThreads.size(); i++) {
        printf("%d ", activeThreads[i].idThread);
        std::cout<<"hazard "<<activeThreads[i].userInfo.showHazards<<endl;
        char threadStr[5];
        sprintf(threadStr, "%d", activeThreads[i].idThread);
        strcat(result, threadStr);
        strcat(result, " ");
    }
    strcpy(buf, result);
}

void userStatus(char* buf, thData &tdL)
{
    std::cout << "ID Thread: " << tdL.idThread << std::endl <<
    "shouldStop: " << tdL.userInfo.shouldStop << std::endl <<
    "showHazards: " << tdL.userInfo.showHazards << std::endl <<
    "showWeather: " << tdL.userInfo.showWeather << std::endl <<
    "showSpeedLimit: " << tdL.userInfo.showSpeedLimit << std::endl <<
    "admin: " << tdL.userInfo.admin << std::endl;
    if(tdL.userInfo.connectedUsername)
    //if(!tdL.userInfo.connectedUsername.empty())
        std::cout<<"connectedUsername: " << tdL.userInfo.connectedUsername << std::endl;
    else
        std::cout<<"connectedUsername: none"<<std::endl;
    strcpy(buf, "Status printed in server terminal");
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