#include <algorithm> // remove_if

std::vector<int> activeThreads;
struct userInformation
{
  bool shouldStop;
  bool showHazards;
  bool showWeather;
  bool showSpeedLimit;
  bool admin;
  char* connectedUsername; // Deallocation not taken care of! Could create problems?
  //std::string connectedUsername;
};

typedef struct thData{
	int idThread; //id-ul thread-ului tinut in evidenta de acest program
	int cl; //descriptorul intors de accept
  userInformation userInfo;
}thData;

#include "users.cpp"


void initialiseThread(thData *td, int &i, int &client)
{
    td->idThread=i++;
	td->cl=client;
    td->userInfo.shouldStop = false;
    td->userInfo.showHazards = false;
    td->userInfo.showWeather = false;
    td->userInfo.showSpeedLimit = false;
    td->userInfo.admin = false;
    td->userInfo.connectedUsername = nullptr;
    //td->userInfo.connectedUsername = "";

    activeThreads.push_back(td->idThread);
}


void logIn(char*, thData&);
void signUp(char*, thData&);
void showActiveThreads(char*, thData&);
void closeClient(char*, thData&);
void defaultOption(char*, thData&);

void processCommand(char* buf, thData tdL) {
    // Define an array of function pointers
    void (*functionArray[])(char*, thData&) = {logIn, signUp, showActiveThreads, closeClient, defaultOption};
    const char* inputPossibilities[] = {"log in", "sing up", "threads", "exit"};

    size_t noOfFunctions = sizeof(functionArray) / sizeof(functionArray[0]);

    // Find the index corresponding to the input string
    int index;
    bool foundCommand = false;
    
    for (index = 0; index < noOfFunctions; index++)
        if ((index <= 1 && strncmp(buf, inputPossibilities[index], strlen(inputPossibilities[index])) == 0 &&
        strlen(buf) > strlen(inputPossibilities[index]) && buf[strlen(inputPossibilities[index])] == ' ') ||
    (index > 1 && strcmp(buf, inputPossibilities[index]) == 0))
        {
            foundCommand = true;
            break;
        }
    
    // No match was found => Invalid command
    if(!foundCommand)
        index--;
    
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
    strcpy(buf, buf+9); // Get rid of "sign up "
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
            createUser(buf, tdL, username, password);
            connectUser(buf, tdL, username, password);
        } 
    }
}

void showActiveThreads(char* buf, thData &tdL) {
    for(int i=0; i<activeThreads.size(); i++)
        printf("%d ", activeThreads[i]);
    strcpy(buf, "Al doilea test");
}

void closeClient(char* buf, thData &tdL) {
    tdL.userInfo.shouldStop=true;

    // Remove the element from activeThreads vector
    auto newEnd = std::remove(activeThreads.begin(), activeThreads.end(), tdL.idThread);
    activeThreads.erase(newEnd, activeThreads.end());
    //activeThreads.shrink_to_fit();
    strcpy(buf, "Closing");
}

void defaultOption(char* buf, thData &tdL) {
    strcpy(buf, "Invalid command");
}
