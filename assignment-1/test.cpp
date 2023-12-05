#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <vector>
#include <fstream>
#include <string>
#include <filesystem>
using namespace std;
using filesystem::path;

pair<string,string> parseRequest(string request)
{
    int firstSpacePos = request.find(" ");
    int secondSpacePos = request.find(" ",firstSpacePos+1);

    string URL = request.substr(firstSpacePos+2,secondSpacePos-firstSpacePos-1);

    int bodyPos = request.find_last_of("\n") + 1;

    string body = request.substr(bodyPos);

    return {URL,body};
}

int main(int argc, char** argv)
{
    // string getRequest = "GET /hello.txt HTTP/1.1\r\n\r\n";
    // auto [URL,body] = parseRequest(getRequest);
    // cout << URL<<endl;
    // cout << body.size();

    string extension = path("ali.txt").extension();
    cout << extension << endl;

    // if(getRequest.find("GET") == 0)
    // {
    //     cout << "get" << endl;
        

    // }
    // else 
    // {
    //     cout << "post" << endl;
    // }

}