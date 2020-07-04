#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif


#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <thread>
#include <chrono>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_CLIENT_PORT 27015
#define DEFAULT_SERVER_PORT "27015"
#define DEFAULT_BUFLEN 512
#define DEFAULT_PING_AMMOUNT 4
#define DEFAULT_SERVER_IP "127.0.0.1"


using namespace std;

int client(PCSTR ipAddr, int n) {


    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);

    struct addrinfo* result = NULL, hints;
    sockaddr_in RecvAddr;
    RecvAddr.sin_family = AF_INET;
    RecvAddr.sin_port = htons(DEFAULT_CLIENT_PORT);
    RecvAddr.sin_addr.s_addr = inet_addr(ipAddr);

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    // Start up Winsock library
    int iResult = WSAStartup(wVersionRequested, &wsaData);
    if (iResult != 0) {
        cout << "Client: Winsock library failed to start, error " << iResult << endl;
        return 1;
    }

    // Resolve the server address and port
    iResult = getaddrinfo(ipAddr, "27015", &hints, &result);
    if (iResult != 0) {
        cout << "Client: Failed to resolve server IP, error " << iResult << endl;
        WSACleanup();
        return 1;
    }
    cout << "Client: Server IP resolved" << endl;

    // Create a SOCKET for connecting to server
    SOCKET ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        cout << "Client: Failed to create a socket, error " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    cout << "Client: Socket successfully created" << endl;

    freeaddrinfo(result);
    int recvbuflen = DEFAULT_BUFLEN;

    const char* sendbuf = "ping";
    char recvbuf[DEFAULT_BUFLEN];

    double avgTime = .0;

    int addrlen = sizeof(RecvAddr);

    for (int i = 0; i < n; ++i)
    {

        cout << "Client: Sending packet to server..." << endl;
        // Start Time
        chrono::high_resolution_clock::time_point startTime = chrono::high_resolution_clock::now();

        iResult = sendto(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
        if (iResult == SOCKET_ERROR) {
            cout << "Client: Send failed: " << WSAGetLastError() << endl;
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }

        iResult = recvfrom(ConnectSocket, recvbuf, recvbuflen, 0, (SOCKADDR*)&RecvAddr, &addrlen);

        // End Time
        chrono::high_resolution_clock::time_point endTime = chrono::high_resolution_clock::now();
        chrono::duration<double, std::milli> timeSpan = endTime - startTime;
        cout << "Client: Recieved reply from " << ipAddr << " in " << timeSpan.count()  << "ms" << endl;
        avgTime += timeSpan.count();
    }
    // Average Time
    avgTime /= n;
    cout << "Client: Average ping time: " << avgTime << "ms" << endl;

    // Shutdown the connection
    iResult = shutdown(ConnectSocket, SD_BOTH);
    if (iResult == SOCKET_ERROR) {
        cout << "Client: Connection shutdown failed: " << WSAGetLastError() << endl;
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    // Cleanup
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}

int server(int n) 
{
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);

    // Start up Winsock library
    int iResult = WSAStartup(wVersionRequested, &wsaData);
    if (iResult != 0) {
        cout << "Server: Winsock library failed to start, error " << iResult << endl;
        return 1;
    }

    struct addrinfo* result = NULL, * ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, DEFAULT_SERVER_PORT, &hints, &result);
    if (iResult != 0) {
        cout << "Server: Failed to resolve local IP and Port, error " << iResult << endl;
        WSACleanup();
        return 1;
    }
    cout << "Server: Local address and port ready for server use." << endl;

    // Create a UDP SOCKET for the server to listen for client connections
    SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        cout << "Server: Failed to create a socket, error " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    cout << "Server: Socket successfully created" << endl;

    // Bind the socket to local address
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        cout << "Server: Failed to bind the socket, error " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    cout << "Server: Socket bound to local IP and ready to use" << endl;
    freeaddrinfo(result);

    char recvbuf[DEFAULT_BUFLEN];
    int iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;
    sockaddr source[16];
    int addrlen = 16;
    char ip[16];
    // Receive until the peer shuts down the connection
    for (int i = 0; i < n; ++i) {

        iResult = recvfrom(ListenSocket, recvbuf, recvbuflen, 0, source, &addrlen);
        if (iResult > 0) {
            // Echo the buffer back to the sender
            cout << "Server: Recieved a Packet from " << inet_ntop(AF_INET, source->sa_data + 2, ip, 16) << ", sending the packet back to source" << endl;
            iSendResult = sendto(ListenSocket, recvbuf, iResult, 0, source, addrlen);
            if (iSendResult == SOCKET_ERROR) {
                cout << "Server: Send failed: " << WSAGetLastError() << endl;
                closesocket(ListenSocket);
                WSACleanup();
                return 1;
            }
        }
        else if (iResult == 0)
            cout << "Connection closing..." << endl;
        else {
            cout << "Failed to receive a packet, error " << WSAGetLastError() << endl;
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
    }

    // shutdown the send half of the connection since no more data will be sent
    iResult = shutdown(ListenSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}

int main(int argc, char* argv[], char* envp[]) {
    // argv[1] - mode of operation ["client" | "server" | "both"], default "both" 
    // argv[2] - number of pings, default 4
    // argv[3] - for client: server IP address, default localhost ("127.0.0.1")


    int n = argc >= 3 ? atoi(argv[2]) : DEFAULT_PING_AMMOUNT;
    PCSTR ipAddr = argc > 4 ? argv[3] : DEFAULT_SERVER_IP;
    //cout << "printing arugments" << endl;
    //for (int i = 0; i < argc; ++i)
    //    cout << argv[i] << endl;
    //cout << "done printing arugments" << endl;
    if (argc >= 2) 
    {
        if (strcmp(argv[1], "server") == 0) {
            server(n);
            system("PAUSE");
            return 0;
        }
        if (strcmp(argv[1], "client") == 0) {
            client(ipAddr, n);
            system("PAUSE");
            return 0;
        }
    }
    std::thread newThread(server, n);
    Sleep(500);
    client(ipAddr, n);
    newThread.join();
    system("PAUSE");
    return 0;
}