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
                    
                    for (thData& td : activeThreads)
                        if (td.idThread == tdL.idThread)
                    {
                    // Could be done better but can't be bothered
                    rapidxml::xml_node<>* showHazardsNode = userNode->first_node("hazards");
                    tdL.userInfo.showHazards = (strcmp(showHazardsNode->value(), "true") == 0);
                    td.userInfo.showHazards = (strcmp(showHazardsNode->value(), "true") == 0);
                    rapidxml::xml_node<>* showWeatherNode = userNode->first_node("weather");
                    tdL.userInfo.showWeather = (strcmp(showWeatherNode->value(), "true") == 0);
                    td.userInfo.showWeather = (strcmp(showWeatherNode->value(), "true") == 0);
                    rapidxml::xml_node<>* showSpeedLimitNode = userNode->first_node("speedlimit");
                    tdL.userInfo.showSpeedLimit = (strcmp(showSpeedLimitNode->value(), "true") == 0);
                    td.userInfo.showSpeedLimit = (strcmp(showSpeedLimitNode->value(), "true") == 0);
                    rapidxml::xml_node<>* adminNode = userNode->first_node("admin");
                    tdL.userInfo.admin = (strcmp(adminNode->value(), "true") == 0);
                    td.userInfo.admin = (strcmp(adminNode->value(), "true") == 0);

                    // Allocate memory to avoid another segmentation fault
                    tdL.userInfo.connectedUsername = new char[username.size() + 1];
                    strcpy(tdL.userInfo.connectedUsername, username.c_str());

                    td.userInfo.connectedUsername = new char[username.size() + 1];
                    strcpy(td.userInfo.connectedUsername, username.c_str());
                    break;
                    }
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


void updateUserSettings(char* buf, const char* username, const std::string& setting, bool value)
{
    try {
        rapidxml::file<> xmlFile("users.xml");
        rapidxml::xml_document<> doc;
        doc.parse<0>(xmlFile.data());

        for (rapidxml::xml_node<>* userNode = doc.first_node("users")->first_node("user"); userNode; userNode = userNode->next_sibling("user")) 
        {
            rapidxml::xml_node<>* usernameNode = userNode->first_node("username");
            if (usernameNode && strcmp(usernameNode->value(), username) == 0) 
            {
                // Update the specified setting
                rapidxml::xml_node<>* settingNode = userNode->first_node(setting.c_str());
                if (settingNode)
                {
                    // Even though you are on the desired node, you need to access first_node()
                    // to change its content - Took a whole day to discover
                    settingNode->first_node()->value(doc.allocate_string(boolToChar(value)));
                    std::ofstream outFile("users.xml");
                    outFile << doc;
                    outFile.close();

                    strcpy(buf, "Setting modified in the account.");
                    return; // Stop the for from searching
                }
                    
                // There shouldn't be a natural way to reach this else
                else
                    std::cerr << "How am I here? - Setting node not found";
            }
        }
        // Same as before, this shouldn't happen if the fution is called correctly
        std::cerr << "User '" << username << "' not found in the XML file." << std::endl;
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}