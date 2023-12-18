// If you remove const you get a segmentation fault
// Took 3 hours to find the problem
const char* boolToChar(bool input)
{
    return input ? "true" : "false";
}

void connectUser(char* buf, thData &tdL, std::string username, std::string password)
{
    // If a user is already logged in on this thread
    if(tdL.userInfo.connectedUsername!=nullptr)
    //if(!tdL.userInfo.connectedUsername.empty())
    {
        strcpy(buf, "You are already logged in.");
        return;
    }
    
    // Load the XML file into memory
    rapidxml::file<> xmlFile("users.xml");
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());

    // Root node
    rapidxml::xml_node<>* root = doc.first_node("users");

    for (rapidxml::xml_node<>* userNode = root->first_node("user"); userNode; userNode = userNode->next_sibling("user")) 
    {
        rapidxml::xml_node<>* usernameNode = userNode->first_node("username");
        // Check if node exists, otherwise segmentation fault
        if(usernameNode)
        {
            if(usernameNode->value() == username)
            {
                rapidxml::xml_node<>* passwordNode = userNode->first_node("password");

                if(passwordNode->value() != password)
                {
                    strcpy(buf, "Incorrect password");
                    return;
                }
                else
                {
                    // Could be done better but can't be bothered
                    rapidxml::xml_node<>* showHazardsNode = userNode->first_node("hazards");
                    tdL.userInfo.showHazards = (strcmp(showHazardsNode->value(), "true") == 0);
                    rapidxml::xml_node<>* showWeatherNode = userNode->first_node("weather");
                    tdL.userInfo.showWeather = (strcmp(showWeatherNode->value(), "true") == 0);
                    rapidxml::xml_node<>* showSpeedLimitNode = userNode->first_node("speedlimit");
                    tdL.userInfo.showSpeedLimit = (strcmp(showSpeedLimitNode->value(), "true") == 0);
                    rapidxml::xml_node<>* adminNode = userNode->first_node("admin");
                    tdL.userInfo.admin = (strcmp(adminNode->value(), "true") == 0);

                    // Allocate memory to avoid another segmentation fault
                    tdL.userInfo.connectedUsername = new char[username.size() + 1];
                    strcpy(tdL.userInfo.connectedUsername, username.c_str());
                    //tdL.userInfo.connectedUsername = username;
                }

                // There can't be two users with the same username, so it's safe to exit either way
                strcpy(buf, "Log In successful");
                return;
            }
        }
    }
    strcpy(buf, "Invalid username");
}


bool isUsernameTaken(rapidxml::xml_node<> *root, const std::string &username) {
    for (rapidxml::xml_node<> *userNode = root->first_node("user"); userNode; userNode = userNode->next_sibling("user")) {
        rapidxml::xml_node<> *existingUsernameNode = userNode->first_node("username");
        if (existingUsernameNode && existingUsernameNode->value()) {
            if (strcmp(existingUsernameNode->value(), username.c_str()) == 0) {
                return true;
            }
        }
    }
    return false;
}


void createUser(char* buf, thData &tdL, std::string username, std::string password, int &errors)
{
    // If a user is already logged in on this thread
    if(tdL.userInfo.connectedUsername!=nullptr)
    //if(!tdL.userInfo.connectedUsername.empty())
    {
        strcpy(buf, "You are already logged in. Can't create another account.");
        errors = 1;
        return;
    }

    try 
    {
        rapidxml::file<> xmlFile("users.xml");
        rapidxml::xml_document<> doc;
        doc.parse<0>(xmlFile.data());

        // Check if username is already taken
        if (isUsernameTaken(doc.first_node("users"), username)) {
            strcpy(buf, "Username already taken. Choose smth else");
            errors = 1;
            return;
        }
    
        rapidxml::xml_node<>* newUserNode = doc.allocate_node(rapidxml::node_element, "user");
    
        rapidxml::xml_node<>* usernameNode = doc.allocate_node(rapidxml::node_element, "username", doc.allocate_string(username.c_str()));
        rapidxml::xml_node<>* passwordNode = doc.allocate_node(rapidxml::node_element, "password", doc.allocate_string(password.c_str()));
        rapidxml::xml_node<>* hazardsNode = doc.allocate_node(rapidxml::node_element, "hazards", doc.allocate_string(boolToChar(tdL.userInfo.showHazards)));
        rapidxml::xml_node<>* weatherNode = doc.allocate_node(rapidxml::node_element, "weather", doc.allocate_string(boolToChar(tdL.userInfo.showWeather)));
        rapidxml::xml_node<>* speedLimitNode = doc.allocate_node(rapidxml::node_element, "speedlimit", doc.allocate_string(boolToChar(tdL.userInfo.showSpeedLimit)));
        char* adminStatus = doc.allocate_string(username == "admin" ? "true" : "false");
        rapidxml::xml_node<>* adminNode = doc.allocate_node(rapidxml::node_element, "admin", adminStatus);
    
        // Append child nodes
        newUserNode->append_node(usernameNode);
        newUserNode->append_node(passwordNode);
        newUserNode->append_node(hazardsNode);
        newUserNode->append_node(weatherNode);
        newUserNode->append_node(speedLimitNode);
        newUserNode->append_node(adminNode);

        // Append to the root
        doc.first_node()->append_node(newUserNode);
        
        // Save the changes
        std::ofstream outputFile("users.xml");
        outputFile << doc; 
        outputFile.close();
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}