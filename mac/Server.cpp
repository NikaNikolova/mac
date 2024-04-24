#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

using namespace std;

const int MaxQueueSize = 100;
const int BufferSize = 4096;

SOCKET serviceSocket;
vector<string> orderLog;
struct MenuItem {
    int prepTime;
    string replyMessage;
    bool isValidOrder;
};

MenuItem AnalyzeOrder(const string& customerInput);
void SetupServer();
void HandleConnections();

int main() {
    cout << "Launching server...\n";
    SetupServer();
    cout << "Server is active and ready to accept clients.\n";
    HandleConnections();
    WSACleanup();
}

void SetupServer() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Winsock setup failed. Error: " << WSAGetLastError() << endl;
        exit(EXIT_FAILURE);
    }

    if ((serviceSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        cerr << "Could not create socket. Error: " << WSAGetLastError() << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverInfo;
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(8888);

    if (bind(serviceSocket, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) == SOCKET_ERROR) {
        cerr << "Socket binding failed. Error: " << WSAGetLastError() << endl;
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    listen(serviceSocket, MaxQueueSize);
}

void HandleConnections() {
    fd_set sockets;
    SOCKET clientSockets[MaxQueueSize] = {0};

    while (true) {
        FD_ZERO(&sockets);
        FD_SET(serviceSocket, &sockets);

        for (int i = 0; i < MaxQueueSize; i++) {
            SOCKET s = clientSockets[i];
            if (s > 0) {
                FD_SET(s, &sockets);
            }
        }

        if (select(0, &sockets, NULL, NULL, NULL) == SOCKET_ERROR) {
            cerr << "Select call failed with error: " << WSAGetLastError() << endl;
            continue;
        }

        sockaddr_in clientInfo;
        int infoSize = sizeof(clientInfo);
        SOCKET newSocket;
        if (FD_ISSET(serviceSocket, &sockets)) {
            if ((newSocket = accept(serviceSocket, (struct sockaddr *)&clientInfo, &infoSize)) < 0) {
                cerr << "Failed to accept new connection." << endl;
                continue;
            }

            cout << "New client connected: " << inet_ntoa(clientInfo.sin_addr) << ":" << ntohs(clientInfo.sin_port) << endl;
            for (int i = 0; i < MaxQueueSize; i++) {
                if (clientSockets[i] == 0) {
                    clientSockets[i] = newSocket;
                    cout << "Registered at index " << i << endl;
                    break;
                }
            }
        }

        for (int i = 0; i < MaxQueueSize; i++) {
            SOCKET s = clientSockets[i];
            if (FD_ISSET(s, &sockets)) {
                char receivedData[BufferSize];
                int msgSize = recv(s, receivedData, BufferSize, 0);
                if (msgSize > 0) {
                    receivedData[msgSize] = '\0';
                    MenuItem orderDetails = AnalyzeOrder(string(receivedData));
                    send(s, orderDetails.replyMessage.c_str(), orderDetails.replyMessage.length(), 0);
                    if (orderDetails.isValidOrder) {
                        Sleep(orderDetails.prepTime * 1000);
                        string completionMsg = "Order completed. Please enjoy your meal!";
                        send(s, completionMsg.c_str(), completionMsg.length(), 0);
                    }
                } else {
                    cout << "Client disconnected." << endl;
                    closesocket(s);
                    clientSockets[i] = 0;
                }
            }
        }
    }
}

MenuItem AnalyzeOrder(const string& input) {
    MenuItem result;
    string output = "Processing your order:\n";
    transform(input.begin(), input.end(), input.begin(), ::toupper);
    int totalPrepTime = 0, totalCost = 0;
    bool orderFound = false;

    if (input.find("HAMBURGER") != string::npos) {
        orderFound = true;
        totalPrepTime += 5;
        totalCost += 4;
    }
    // Continue checking for other menu items...
    if (orderFound) {
        output += "Please wait " + to_string(totalPrepTime) + " seconds.\n";
        output += "Total cost is $" + to_string(totalCost) + ".\n";
    } else {
        output = "Please make a valid selection from the menu.\n";
    }

    result.prepTime = totalPrepTime;
    result.replyMessage = output;
    result.isValidOrder = orderFound;
    return result;
}
