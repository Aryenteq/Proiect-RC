char* boolToChar(bool input)
{
    char* result;
    if(input==true)
        strcpy(result, "true");
    else
        strcpy(result, "false");
    return result;
}

void connectUser(char* buf, thData &tdL, std::string username, std::string password)
{
    // If a user is already logged in on this thread
    if(tdL.userInfo.connectedUsername!=nullptr)
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
                    tdL.userInfo.showHazards = (showHazardsNode->value() == "true");
                    rapidxml::xml_node<>* showWeatherNode = userNode->first_node("weather");
                    tdL.userInfo.showWeather = (showWeatherNode->value() == "true");
                    rapidxml::xml_node<>* showSpeedLimitNode = userNode->first_node("speedlimit");
                    tdL.userInfo.showSpeedLimit = (showSpeedLimitNode->value() == "true");
                    rapidxml::xml_node<>* adminNode = userNode->first_node("admin");
                    tdL.userInfo.admin = (adminNode->value() == "true");

                    // Allocate memory to avoid another segmentation fault
                    tdL.userInfo.connectedUsername = new char[username.size() + 1];
                    strcpy(tdL.userInfo.connectedUsername, username.c_str());
                }

                // There can't be two users with the same username, so it's safe to exit either way
                strcpy(buf, "Log In successful");
                return;
            }
        }
    }
    strcpy(buf, "Invalid username");
}


void createUser(char* buf, thData &tdL, std::string username, std::string password)
{
    rapidxml::file<> xmlFile("users.xml");
    rapidxml::xml_document<> doc;
    doc.parse<0>(xmlFile.data());
    // Check if usernames already taken
    // Create a new user node
    rapidxml::xml_node<>* newUserNode = doc.allocate_node(rapidxml::node_element, "user");

    rapidxml::xml_node<>* usernameNode = doc.allocate_node(rapidxml::node_element, "username", doc.allocate_string(username.c_str()));
    rapidxml::xml_node<>* passwordNode = doc.allocate_node(rapidxml::node_element, "password", doc.allocate_string(password.c_str()));
    rapidxml::xml_node<>* hazardsNode = doc.allocate_node(rapidxml::node_element, "hazards", doc.allocate_string(boolToChar(tdL.userInfo.showHazards)));
    rapidxml::xml_node<>* weatherNode = doc.allocate_node(rapidxml::node_element, "weather", doc.allocate_string(boolToChar(tdL.userInfo.showWeather)));
    rapidxml::xml_node<>* speedLimitNode = doc.allocate_node(rapidxml::node_element, "speedlimit", doc.allocate_string(boolToChar(tdL.userInfo.showSpeedLimit)));

    char* adminStatus;
    if(username == "admin")
        strcpy(adminStatus, "true");
    else
        strcpy(adminStatus, "false");
    rapidxml::xml_node<>* adminNode = doc.allocate_node(rapidxml::node_element, "admin", doc.allocate_string(adminStatus));

    // Append child nodes to the new user node
    newUserNode->append_node(usernameNode);
    newUserNode->append_node(passwordNode);
    newUserNode->append_node(hazardsNode);
    newUserNode->append_node(weatherNode);
    newUserNode->append_node(speedLimitNode);
    newUserNode->append_node(adminNode);

    // Append to the root
    doc.first_node()->append_node(newUserNode);
}