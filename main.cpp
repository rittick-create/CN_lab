#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "./Sender/Frame.cpp"

using namespace std;

int main()
{
    //Take the input from the text file
    ifstream file("textfile.txt");

    if (!file.is_open())
    {
        cout << "Could not open the file\n";
        return 1;
    }

    //storing the textfile data into a vector of strings
    // vector<string> fileData;
    string line;

    //Reading each line and storing into fileData line be line
    while (getline(file, line))
    {
        //Read line by line and store it into a file inside Sender
    }

    file.close();// Close the file after reading

    

    return 0;
}