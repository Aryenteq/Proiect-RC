#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <mutex>
#include <future>
#include <sstream>

std::mutex outputTextMutex;
void sendRequest(const int sd, const std::string &req)
{
    char buf[1000];
    strcpy(buf, req.c_str());
    if (write(sd, &buf, sizeof(buf)) <= 0)
    {
        std::cerr << "[client]Eroare la write() spre server: " << strerror(errno) << std::endl;
        return;
    }
}

// Define constants
const float NODE_RADIUS = 20;
const float EDGE_THICKNESS = 12.5;
const float MIN_NODE_DISTANCE = 0.2;

const int popUpHeight = 100;
const int popUpWidth = 200;
sf::Font font;
sf::Text outputText("", font, 12);

std::mutex promiseMutex;
std::atomic<bool> received(false);
std::vector<std::promise<std::string>> promises;

void PassBufferToThread(const char *buf)
{
    std::string message(buf);
    message = insertNewlines(message, 35);
    std::promise<std::string> promise;
    {
        std::lock_guard<std::mutex> lock(promiseMutex);
        promises.push_back(std::move(promise));
        promises.back().set_value(message);
        received = true;
    }
}

bool isTooClose(const sf::Vector2f &newPos, const std::vector<sf::Vector2f> &existingPositions, const sf::RenderWindow &window, float minDistance)
{
    for (const auto &existingPos : existingPositions)
    {
        float distance = std::sqrt((newPos.x - existingPos.x) * (newPos.x - existingPos.x) +
                                   (newPos.y - existingPos.y) * (newPos.y - existingPos.y));
        if (distance < minDistance * window.getSize().y)
            return true;
    }
    return false;
}

void updatePath(const sf::Text &outputText, std::vector<int> &path)
{
    std::string outputString = outputText.getString();
    outputString.erase(std::remove(outputString.begin(), outputString.end(), '\n'), outputString.end());

    std::string prefix = "Route you need to follow: ";
    size_t found = outputString.find(prefix);

    if (found != std::string::npos)
    {
        std::string pathSubstring = outputString.substr(found + prefix.length());

        // If there's only one node, bye
        if (pathSubstring.find(' ') == std::string::npos)
            return;
        path.clear();
        // push_back every node from the path
        std::istringstream iss(pathSubstring);
        int number;
        while (iss >> number)
            path.push_back(number);
    }
}

void updateCarPosition(const sf::Text &outputText, sf::Vector2f &globalCarPosition, const std::vector<sf::Vector2f> &nodePositions)
{
    char buf[1000];
    std::string outputString = outputText.getString();
    std::string prefix = "Location changed to ";
    size_t found = outputString.find(prefix);
    if (found != std::string::npos)
    {
        size_t numberStart = found + prefix.length();
        size_t numberEnd = outputString.find_first_not_of("0123456789", numberStart);

        std::string numberString = outputString.substr(numberStart, numberEnd - numberStart);

        if (isValidInteger(buf, numberString))
        {
            int nodeNumber = std::stoi(numberString);
            globalCarPosition = nodePositions[nodeNumber];
        }
        else
            std::cout << buf;
    }
}

void drawCar(const sf::Vector2f &position, sf::RenderWindow &window, const sf::Texture &carTexture)
{
    sf::Sprite carSprite(carTexture);
    carSprite.setScale(0.2, 0.2);
    carSprite.setPosition(position);
    window.draw(carSprite);
}

void drawEdges(const sf::Text &outputText, const std::vector<sf::Vector2f> &nodePositions, const Graph &graph, sf::RenderWindow &window)
{
    std::vector<int> path;
    updatePath(outputText, path);
    for (int i = 0; i <= graph.lastNode(); ++i)
    {
        for (const Edge &edge : graph.getAdjacencyList(i))
        {
            // Center points
            sf::Vector2f start = nodePositions[i];
            sf::Vector2f end = nodePositions[edge.destination];

            // Normalized direction vector - some math
            sf::Vector2f dir = end - start;
            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            dir /= length;
            sf::Vector2f offset(-dir.y, dir.x);

            // Edge rectangle
            sf::ConvexShape edgeShape;
            edgeShape.setPointCount(4);
            edgeShape.setPoint(0, start + offset * EDGE_THICKNESS / 2.0f);
            edgeShape.setPoint(1, end + offset * EDGE_THICKNESS / 2.0f);
            edgeShape.setPoint(2, end - offset * EDGE_THICKNESS / 2.0f);
            edgeShape.setPoint(3, start - offset * EDGE_THICKNESS / 2.0f);

            // Based on path vector, color the nodes with blue (dijkstra)
            bool currentEdgeInPath = std::find(path.begin(), path.end(), i) != path.end() &&
                                     std::find(path.begin(), path.end(), edge.destination) != path.end();

            if (currentEdgeInPath)
                edgeShape.setFillColor(sf::Color(173, 216, 230)); // Light blue color
            else
                edgeShape.setFillColor(sf::Color(200, 200, 200)); // Light gray color

            window.draw(edgeShape);

            // Borders
            sf::VertexArray lineBlack(sf::LinesStrip, 5);
            lineBlack[0].position = start + offset * EDGE_THICKNESS / 2.0f;
            lineBlack[1].position = start - offset * EDGE_THICKNESS / 2.0f;
            lineBlack[2].position = end - offset * EDGE_THICKNESS / 2.0f;
            lineBlack[3].position = end + offset * EDGE_THICKNESS / 2.0f;
            lineBlack[4].position = start + offset * EDGE_THICKNESS / 2.0f;

            for (int i = 0; i < 5; i++)
                lineBlack[i].color = sf::Color(0, 0, 0); // Black

            window.draw(lineBlack);

            // Rotation for text - add 180 degrees if upside down
            float angle = std::atan2(dir.y, dir.x) * 180.0f / static_cast<float>(M_PI);
            if (angle > 90 || angle < -90)
                angle += 180.0f;

            sf::Text text(edge.streetName + " - " + std::to_string(edge.length) + " km", font, 12);
            text.setFillColor(sf::Color::Black);

            // Position
            text.setPosition(start + dir * length / 2.0f);
            text.setOrigin(text.getLocalBounds().width / 2.0f, text.getLocalBounds().height / 2.0f);
            text.setRotation(angle);

            window.draw(text);
        }
    }
}

void popUp(sf::RenderWindow &window, bool showPopUp, sf::Vector2f &popUpPosition, int globalFrom, int globalTo, int sd)
{
    if (showPopUp)
    {
        sf::RectangleShape popUpBox(sf::Vector2f(popUpWidth, popUpHeight));
        popUpBox.setFillColor(sf::Color(144, 238, 144)); // Light green color
        popUpBox.setOutlineColor(sf::Color::Black);
        popUpBox.setOutlineThickness(2.0f);
        popUpBox.setPosition(popUpPosition);

        window.draw(popUpBox);

        sf::Text patholeButton("Pathole on road", font, 12);
        sf::Text accidentButton("Accident", font, 12);
        sf::Text constructionButton("Construction", font, 12);

        // Dimension based on popUpBox
        float buttonWidth = 0.9f * popUpBox.getSize().x;
        float buttonHeight = 0.25f * popUpBox.getSize().y;

        sf::FloatRect popUpBoxRect = popUpBox.getGlobalBounds();

        sf::RectangleShape patholeButtonShape(sf::Vector2f(buttonWidth, buttonHeight));
        patholeButtonShape.setFillColor(sf::Color(144, 238, 144));
        patholeButtonShape.setOutlineColor(sf::Color::Black);
        patholeButtonShape.setOutlineThickness(1.5f);
        patholeButtonShape.setPosition(popUpPosition.x + 0.05f * popUpBox.getSize().x, popUpPosition.y + 0.05f * popUpBox.getSize().y);

        sf::RectangleShape accidentButtonShape(sf::Vector2f(buttonWidth, buttonHeight));
        accidentButtonShape.setFillColor(sf::Color(144, 238, 144));
        accidentButtonShape.setOutlineColor(sf::Color::Black);
        accidentButtonShape.setOutlineThickness(1.5f);
        accidentButtonShape.setPosition(popUpPosition.x + 0.05f * popUpBox.getSize().x, popUpPosition.y + 0.35f * popUpBox.getSize().y);

        sf::RectangleShape constructionButtonShape(sf::Vector2f(buttonWidth, buttonHeight));
        constructionButtonShape.setFillColor(sf::Color(144, 238, 144));
        constructionButtonShape.setOutlineColor(sf::Color::Black);
        constructionButtonShape.setOutlineThickness(1.5f);
        constructionButtonShape.setPosition(popUpPosition.x + 0.05f * popUpBox.getSize().x, popUpPosition.y + 0.65f * popUpBox.getSize().y);

        // Text size and positions
        patholeButton.setCharacterSize(static_cast<unsigned int>(buttonHeight * 0.4f));
        accidentButton.setCharacterSize(static_cast<unsigned int>(buttonHeight * 0.4f));
        constructionButton.setCharacterSize(static_cast<unsigned int>(buttonHeight * 0.4f));

        patholeButton.setPosition(patholeButtonShape.getPosition().x + 0.5f * (buttonWidth - patholeButton.getLocalBounds().width),
                                  patholeButtonShape.getPosition().y + 0.5f * (buttonHeight - patholeButton.getLocalBounds().height));

        accidentButton.setPosition(accidentButtonShape.getPosition().x + 0.5f * (buttonWidth - accidentButton.getLocalBounds().width),
                                   accidentButtonShape.getPosition().y + 0.5f * (buttonHeight - accidentButton.getLocalBounds().height));
        constructionButton.setPosition(constructionButtonShape.getPosition().x + 0.5f * (buttonWidth - constructionButton.getLocalBounds().width),
                                       constructionButtonShape.getPosition().y + 0.5f * (buttonHeight - constructionButton.getLocalBounds().height));

        window.draw(patholeButtonShape);
        window.draw(accidentButtonShape);
        window.draw(constructionButtonShape);

        // Text color
        patholeButton.setFillColor(sf::Color::Black);
        accidentButton.setFillColor(sf::Color::Black);
        constructionButton.setFillColor(sf::Color::Black);

        window.draw(patholeButton);
        window.draw(accidentButton);
        window.draw(constructionButton);

        // Button click logic
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
        {
            sf::Vector2f mousePosition = window.mapPixelToCoords(sf::Mouse::getPosition(window));

            sf::FloatRect patholeButtonRect = patholeButtonShape.getGlobalBounds();
            sf::FloatRect accidentButtonRect = accidentButtonShape.getGlobalBounds();
            sf::FloatRect constructionButtonRect = constructionButtonShape.getGlobalBounds();

            if (patholeButtonRect.contains(mousePosition))
                sendRequest(sd, "hazard " + std::to_string(globalFrom) + " " + std::to_string(globalTo) + " pathole");
            else if (accidentButtonRect.contains(mousePosition))
                sendRequest(sd, "hazard " + std::to_string(globalFrom) + " " + std::to_string(globalTo) + " accident");
            else if (constructionButtonRect.contains(mousePosition))
                sendRequest(sd, "hazard " + std::to_string(globalFrom) + " " + std::to_string(globalTo) + " construction");
        }
    }
}

void drawGraph(int sd, Graph &graph, std::atomic<bool> &stopThreads, std::condition_variable &cv)
{
    sf::RenderWindow window(sf::VideoMode(1200, 800), "Monitorizarea traficului");
    window.setFramerateLimit(60);

    // Random positions for each node - mt19937 would be better but hei
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    std::vector<sf::Vector2f> nodePositions(graph.lastNode() + 1);

    // Set node pos
    for (int i = 0; i <= graph.lastNode(); ++i)
    {
        sf::Vector2f newPos;
        do
        {
            float x = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * 4 / 5 * window.getSize().x;
            float y = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * window.getSize().y;
            newPos = sf::Vector2f(x, y);
        } while (isTooClose(newPos, nodePositions, window, MIN_NODE_DISTANCE));

        nodePositions[i] = newPos;
    }

    // Car texture
    sf::Texture carTexture;
    if (!carTexture.loadFromFile("/home/ciprian/Desktop/Proiect RC/car.png"))
    {
        std::cerr << "Failed to load car image" << std::endl;
        return;
    }
    if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
        std::cerr << "Failed to load font" << std::endl;

    // Random variables
    sf::Vector2f globalCarPosition = nodePositions[0];
    bool showPopUp = false;
    bool clickedOnEdge = false;
    int globalFrom = -1;
    int globalTo = -1;
    sf::Vector2f popUpPosition;
    std::string popUpText;
    sf::String inputString; // This will store the user's input
    bool inputActive = true;

    // Right vertical linne
    float xPosition = 4.0f / 5.0f * window.getSize().x;

    // I/O
    sf::RectangleShape inputBox(sf::Vector2f(window.getSize().x - xPosition, 40.0f));
    inputBox.setFillColor(sf::Color::White);
    inputBox.setOutlineColor(sf::Color::Black);
    inputBox.setOutlineThickness(2.0f);
    inputBox.setPosition(xPosition, 0.0f);

    sf::Text inputText("", font, 16);
    inputText.setFillColor(sf::Color::Black);
    inputText.setPosition(xPosition + 5.0f, 5.0f); // Adjust the position as needed

    sf::RectangleShape outputLocation(sf::Vector2f(window.getSize().x - xPosition, window.getSize().y - inputBox.getSize().y));
    outputLocation.setFillColor(sf::Color::White);
    outputLocation.setOutlineColor(sf::Color::Black);
    outputLocation.setOutlineThickness(2.0f);
    outputLocation.setPosition(xPosition, inputBox.getSize().y);

    sf::Text outputText("", font, 16);
    outputText.setFillColor(sf::Color::Black);
    outputText.setPosition(xPosition + 5.0f, inputBox.getSize().y + 5.0f); // Adjust the position as needed

    // Main while
    while (window.isOpen())
    {
        // Stop thread on close
        if (stopThreads.load())
            break;
        cv.notify_all();

        updateCarPosition(outputText, globalCarPosition, nodePositions);

        sf::Event event;
        while (window.pollEvent(event))
        {
            std::lock_guard<std::mutex> lock(promiseMutex);

            // Update outputText
            if (received)
            {
                std::shared_future<std::string> future = promises.back().get_future().share();
                std::string receivedMessage = future.get();
                outputText.setString(receivedMessage);
                received = false;
            }
            if (event.type == sf::Event::Closed)
            {
                window.close();
                std::string pa("exit");
                sendRequest(sd, pa);
            }
            else if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    // Mouse position
                    sf::Vector2f mousePosition = window.mapPixelToCoords(sf::Mouse::getPosition(window));

                    // Check if the click is on an edge
                    bool clickedOnEdge = false;
                    float minDistance = std::numeric_limits<float>::max();
                    int closestEdgeFrom = -1;
                    int closestEdgeTo = -1;

                    for (int i = 0; i <= graph.lastNode(); ++i)
                    {
                        for (const Edge &edge : graph.getAdjacencyList(i))
                        {
                            sf::Vector2f start = nodePositions[i];
                            sf::Vector2f end = nodePositions[edge.destination];

                            sf::Vector2f dir = end - start;
                            float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                            dir /= length;
                            sf::Vector2f offset(-dir.y, dir.x);
                            sf::Vector2f mouseToStart = mousePosition - start;
                            float projection = mouseToStart.x * dir.x + mouseToStart.y * dir.y;

                            sf::Vector2f closestPoint;
                            if (projection <= 0)
                                closestPoint = start;
                            else if (projection >= length)
                                closestPoint = end;
                            else
                                closestPoint = start + dir * projection;

                            // Calculate distance from the mouse click to the edge
                            float distance = std::sqrt((mousePosition.x - closestPoint.x) * (mousePosition.x - closestPoint.x) +
                                                       (mousePosition.y - closestPoint.y) * (mousePosition.y - closestPoint.y));

                            // Set a threshold for click proximity to the edge
                            float clickThreshold = 5.0f;

                            if (distance < clickThreshold && distance < minDistance)
                            {
                                // Found a closer edge
                                minDistance = distance;
                                clickedOnEdge = true;
                                popUpPosition = mousePosition;
                                popUpText = "Edge: " + std::to_string(i) + " to " + std::to_string(edge.destination);
                                closestEdgeFrom = i;
                                closestEdgeTo = edge.destination;
                            }
                        }
                    }

                    // Update global variables with the closest edge
                    if (clickedOnEdge)
                    {
                        showPopUp = true;
                        globalFrom = closestEdgeFrom;
                        globalTo = closestEdgeTo;
                    }
                    else
                        showPopUp = false;
                }
            }
            else if (inputActive && event.type == sf::Event::TextEntered)
            {
                if (event.text.unicode < 128) // ASCII
                {
                    if (event.text.unicode == 13) // Enter
                    {
                        sendRequest(sd, inputString.toAnsiString());
                        inputString.clear();
                    }
                    else if (event.text.unicode == 8 && !inputString.isEmpty()) // Backspace
                        inputString.erase(inputString.getSize() - 1);
                    else if (event.text.unicode != 8) // Other characters
                        inputString += event.text.unicode;

                    inputText.setString(inputString);
                }
            }
        }

        window.clear(sf::Color::White);

        drawEdges(outputText, nodePositions, graph, window);
        // Draw nodes
        for (int i = 0; i <= graph.lastNode(); ++i)
        {
            sf::CircleShape node(NODE_RADIUS);
            node.setFillColor(sf::Color::Black);
            node.setPosition(nodePositions[i] - sf::Vector2f(NODE_RADIUS, NODE_RADIUS));
            window.draw(node);

            sf::Text text(std::to_string(i), font, 12);
            text.setFillColor(sf::Color::White);
            text.setPosition(nodePositions[i] - sf::Vector2f(NODE_RADIUS / 2.0f, NODE_RADIUS / 2.0f));
            window.draw(text);
        }

        // Right lane to break the graph from input/output
        sf::VertexArray verticalLine(sf::Lines, 2);
        float xPosition = 4.0f / 5.0f * window.getSize().x;
        verticalLine[0].position = sf::Vector2f(xPosition, 0.0f);
        verticalLine[1].position = sf::Vector2f(xPosition, window.getSize().y);

        // Set the color of the line to black
        for (int i = 0; i < 2; ++i)
            verticalLine[i].color = sf::Color::Black;

        window.draw(verticalLine);
        drawCar(globalCarPosition, window, carTexture);

        // Draw pop-up if needed
        popUp(window, showPopUp, popUpPosition, globalFrom, globalTo, sd);

        // I/O
        sf::RectangleShape inputBox(sf::Vector2f(window.getSize().x - xPosition, 40.0f));
        inputBox.setFillColor(sf::Color::White);
        inputBox.setOutlineColor(sf::Color::Black);
        inputBox.setOutlineThickness(2.0f);
        inputBox.setPosition(xPosition, 0.0f);

        sf::RectangleShape outputLocation(sf::Vector2f(window.getSize().x - xPosition, window.getSize().y - inputBox.getSize().y));
        outputLocation.setFillColor(sf::Color::White);
        outputLocation.setOutlineColor(sf::Color::Black);
        outputLocation.setOutlineThickness(2.0f);
        outputLocation.setPosition(xPosition, inputBox.getSize().y);

        window.draw(inputBox);
        window.draw(outputLocation);
        std::lock_guard<std::mutex> lock(outputTextMutex);
        window.draw(inputText);
        window.draw(outputText);
        window.display();
    }
}
