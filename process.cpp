#include <algorithm> // remove_if

std::vector<bool> shouldStop(0, false);
std::vector<int> connectedIDs;


void option1(char*, int);
void option2(char*, int);
void closeClient(char*, int);
void defaultOption(char*, int);

void processCommand(char* buf, const int &id) {
    // Define an array of function pointers
    void (*functionArray[])(char*, int) = {option1, option2, closeClient, defaultOption};
    const char* inputPossibilities[] = {"test", "test2", "exit"};

    size_t noOfFunctions = sizeof(functionArray) / sizeof(functionArray[0]);

    // Find the index corresponding to the input string
    int index;
    bool foundCommand = false;
    for (index = 0; index < noOfFunctions; index++)
        if (strcmp(buf, inputPossibilities[index]) == 0)
        {
            foundCommand = true;
            break;
        }
    
    // No match was found => Invalid command
    if(!foundCommand)
        index--;
    (*functionArray[index])(buf, id);
}

void option1(char* buf, int id) {
    strcpy(buf, "Primul test");
}

void option2(char* buf, int id) {
    for(int i=0; i<connectedIDs.size(); i++)
        printf("%d ", connectedIDs[i]);
    strcpy(buf, "Al doilea test");
}

//std::atomic<bool> shouldStop(false);
void closeClient(char* buf, int id) {
    //shouldStop[0].store(true);
    shouldStop[id]=true;

    // Remove the element from connectedIDs vector
    auto newEnd = std::remove(connectedIDs.begin(), connectedIDs.end(), id);
    connectedIDs.erase(newEnd, connectedIDs.end());
    //connectedIDs.shrink_to_fit();
    strcpy(buf, "Closing");
}

void defaultOption(char* buf, int id) {
    strcpy(buf, "Invalid command");
}
