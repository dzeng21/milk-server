#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

using namespace std;

const int BUFFER_SIZE = 1024;

int main() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cout << "could not connect to server\n";
        exit(1);
    }

    cout << "Milk Client\n";
    cout << "Copyright (C) Milk Corporation. All rights reserved.\n\n";

    string msg;

    while (true) {
        cout << "PS > ";
        getline(cin, msg);
        if (msg == "exit") { break;}
        if (msg.size() <= 0) { msg = "help"; }

        if (send(clientSocket, msg.c_str(), msg.size(), 0) < 0) {
            cout << "could not send\n";
        }

        char buffer[BUFFER_SIZE] = {0};
        int bytes_receieved = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytes_receieved == 0) {
            cout << "could not connect to server\n";
            exit(1);
        }

        cout << "server: " << buffer;
    }

    close(clientSocket);    

    return 0;
}