bool isValidInteger(char* buf, const std::string& value) 
{
    try 
    {
        std::stoi(value);
        return true;
    } 
    catch (const std::invalid_argument&) 
    {
        strcpy(buf, "Invalid argument, use a natural number.");
        return false;
    } 
    catch (const std::out_of_range&) 
    {
        strcpy(buf, "Number is too big");
        return false;
    }
}

bool isValidDouble(char* buf, const std::string& value) 
{
    try 
    {
        std::stod(value);
        return true;
    } 
    catch (const std::invalid_argument&) 
    {
        strcpy(buf, "Invalid argument, use a number.");
        return false;
    } 
    catch (const std::out_of_range&) 
    {
        strcpy(buf, "Number is too big");
        return false;
    }
}

// If you remove const you get a segmentation fault
// Took 3 hours to find the problem
const char* boolToChar(bool input)
{
    return input ? "true" : "false";
}

std::string insertNewlines(const std::string &input, std::size_t lineLength)
{
    std::string result;
    for (int i = 0; i < input.length(); i++)
    {
        result += input[i];
        if ((i + 1) % lineLength == 0)
            result += '\n';
    }
    return result;
}