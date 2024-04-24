#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

constexpr int MaxBufferSize = 4096;
const char* const ServerIP = "127.0.0.1";
const char* const Port = "8888";

SOCKET communicationSocket;

void DisplayFoodMenu() {
    cout << "Menu Selection:\n";
    cout << "1. Beef Burger - $5\n";
    cout << "2. Soda - $2\n";
    cout << "3. Onion Fries - $3\n";
    cout << "4. Gelato - $2\n";
    cout << "5. Veggie Burger - $6\n";
    cout << "6. Double Cheeseburger - $4\n";
    cout << "7. Veg Wrap - $5\n";
    cout << "8. Club Sandwich - $7\n";
    cout << "9. Seafood Roll - $5\n";
    cout << "10. Spicy Dip - $1\n";
    cout << "Input your choices (e.g., Club Sandwich, Soda, Onion Fries, Spicy Dip):\n";
}

DWORD WINAPI ProcessOrders(LPVOID) {
    while (true) {
        char inputBuffer[MaxBufferSize];
        cin.getline(inputBuffer, MaxBufferSize);
        int sendResult = send(communicationSocket, inputBuffer, strlen(inputBuffer), 0);
        if (sendResult == SOCKET_ERROR) {
            cerr << "Failed to send the order: " << WSAGetLastError() << "\n";
            continue;
        }
    }
    return 0;
}

DWORD WINAPI FetchResponses(LPVOID) {
    while (true) {
        char serverReply[MaxBufferSize];
        int bytesReceived = recv(communicationSocket, serverReply, MaxBufferSize, 0);
        if (bytesReceived > 0) {
            serverReply[bytesReceived] = '\0';
            cout << serverReply << "\n";
        } else if (bytesReceived == 0) {
            cout << "Server connection closed.\n";
            break;
        } else {
            cerr << "Receiving failed: " << WSAGetLastError() << "\n";
            break;
        }
    }
    return 0;
}

int SetupConnection() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup error.\n";
        return -1;
    }

    struct addrinfo hints = {}, *serverInfo;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int getStatus = getaddrinfo(ServerIP, Port, &hints, &serverInfo);
    if (getStatus != 0) {
        WSACleanup();
        cerr << "Getaddrinfo error: " << getStatus << "\n";
        return -2;
    }

    for (auto ptr = serverInfo; ptr != nullptr; ptr = ptr->ai_next) {
        communicationSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (communicationSocket == INVALID_SOCKET) {
            WSACleanup();
            cerr << "Socket creation error: " << WSAGetLastError() << "\n";
            return -3;
        }

        if (connect(communicationSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
            closesocket(communicationSocket);
            communicationSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(serverInfo);

    if (communicationSocket == INVALID_SOCKET) {
        WSACleanup();
        cerr << "Connection failed.\n";
        return -4;
    }

    return 0;
}

int main() {
    if (SetupConnection() != 0) {
        return 1;
    }

    DisplayFoodMenu();

    CreateThread(NULL, 0, ProcessOrders, NULL, 0, NULL);
    CreateThread(NULL, 0, FetchResponses, NULL, 0, NULL);

    WaitForSingleObject(GetCurrentThread(), INFINITE);

    closesocket(communicationSocket);
    WSACleanup();
    return 0;
}
