#pragma once

#include <fstream>
#include <sstream>
#include <filesystem>

using namespace std;

string readBuffer(const string& filePath)
{
    if (!filesystem::is_regular_file(filePath))
        return "";

    ifstream file(filePath);
    stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

void writeBuffer(const string& filePath, const string& buffer)
{
    ofstream file(filePath);
    file << buffer;
}