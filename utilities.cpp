bool isValidInteger(char* buf, const std::string& value) 
{
    try 
    {
        std::stoi(value);
        return true; // Conversion succeeded
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
