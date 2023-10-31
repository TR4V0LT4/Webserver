#include "inc/Multiplexing.hpp"

Multiplexing::Multiplexing()
{
    FD_ZERO(&io.readSockets);
    FD_ZERO(&io.writeSockets);
    FD_ZERO(&io.tmpReadSockets);
    FD_ZERO(&io.tmpWriteSockets);
}

Multiplexing::~Multiplexing()
{
}

ioSets Multiplexing::getIoSets() const
{
    return io;
}


void Multiplexing::setupServer(std::vector <std::pair <Socket , Server_storage > > _server)
{
    for ( std::vector<std::pair <Socket , Server_storage > >::iterator it = _server.begin(); it != _server.end(); ++it)
    {
        FD_SET(it->second.getFd(), &io.readSockets);
        maxFd = std::max( maxFd, it->second.getFd());
    }

    //==> I COMMENT THIS FOR COMPILATION SAKE

    //const char* responseHeader = "HTTP/1.1 200 OK\r\nContent-Type: video/mp4\r\nContent-Length: ";
   // const char* responseEnd = "\r\n\r\n";

    while (true)
    {
        io.tmpReadSockets = io.readSockets;
        io.tmpWriteSockets = io.writeSockets;
        if (select(maxFd + 1, &io.tmpReadSockets, &io.tmpWriteSockets, NULL, NULL) == -1)
        {
            perror("Waiting for incoming data failed");
            break;
        }
        // check for new connection
        for (std::vector <std::pair <Socket , Server_storage > >::iterator it = _server.begin(); it != _server.end(); ++it)
        {
            if (FD_ISSET(it->second.getFd(), &io.tmpReadSockets))
            {
                handleNewConnection(it->first, it->second);
            }
        }
        // loop through clients and check for events
        for (size_t i = 0; i < clients.size(); i++)
        {
            // check for read event
            if (FD_ISSET(clients[i].first.get_fd(), &io.tmpReadSockets))
            {
                
                unsigned char buffer[2048];
                bzero(buffer, 2048);
                // std::cout << "buffer size " << sizeof(buffer) << std::endl;
                ssize_t bytesRead = recv(clients[i].first.get_fd(), buffer, sizeof(buffer), 0);
                if (bytesRead == -1) {
                    perror("Receiving data failed");
                    break;
                }
                else if (bytesRead == 0) {
                    std::cout << "Client closed connection" << std::endl;
                    close(clients[i].first.get_fd());
                    FD_CLR(clients[i].first.get_fd(), &io.readSockets);
                    FD_CLR(clients[i].first.get_fd(), &io.writeSockets);
                    clients.erase(clients.begin() + i);
                    break;
                }
                else {
                    buffer[bytesRead] = '\0';
                    // buffer is ready for parse here
                    try{
                        clients[i].first.req.reader(buffer, bytesRead);
                    }
                    catch (std::exception &exp){
                        std::string exceptionMessage = exp.what();
                        clients[i].first.res.set_status_code(std::stoi(exceptionMessage));
						// error page call !!
                    }
                    // move from read to write sockets if request is done 
                    if (clients[i].first.req.isReadDone())
                    {
                        
                        FD_CLR(clients[i].first.get_fd(), &io.readSockets);
                        FD_SET(clients[i].first.get_fd(), &io.writeSockets);
                    }
                }
            }
            // check for write event
            if (FD_ISSET(clients[i].first.get_fd(), &io.tmpWriteSockets))
            {
                std::cout << "socket "<<clients[i].first.get_fd() << std::endl;
                clients[i].first.res.init_response(clients[i].first.req , clients[i].second);
                // ft_response(clients[i].first, clients[i].second);
                // std::cout << "write" << std::endl;
                // const char* responseHeader = "HTTP/1.1 204 No Content\r\n\r\n";
                // ssize_t bytesSent = send(clients[i].first.get_fd(), responseHeader, strlen(responseHeader), 0);
                // if (bytesSent == -1) {
                //     perror("Sending response header failed");
                //     FD_CLR(clients[i].first.get_fd(), &io.writeSockets);
                //     close(clients[i].first.get_fd());
                //     continue;
                // }
                // std::cout << "sent" << std::endl;
                FD_CLR(clients[i].first.get_fd(), &io.writeSockets);
                close(clients[i].first.get_fd());
                clients.erase(clients.begin() + i);
                i--;
            }
        }
    }
    for (std::vector <std::pair <Socket , Server_storage > >::iterator it = _server.begin(); it != _server.end(); ++it)
        close(it->second.getFd());
}

void Multiplexing::handleNewConnection(Socket& serverSocket, Server_storage server)
{
    Client client;
    // std::cout << client.req.getChunkSize() << std::endl;
    struct sockaddr_in address = serverSocket.get_address();
    socklen_t clientAddrLen = sizeof(address);
    int clientSocket = accept(serverSocket.get_fd(), (struct sockaddr *)&address, &clientAddrLen);
    client.set_fd(clientSocket);
    if (clientSocket == -1)
    {
        perror("Accepting connection failed");
    }
    else
    {
        FD_SET(clientSocket, &io.readSockets);
        this->clients.push_back(std::make_pair(client, server));
        maxFd = std::max(maxFd, clientSocket);
        std::cout << "Accepted client connection from " << inet_ntoa(client.get_address().sin_addr) << std::endl;
        std::cout << clients.size() << std::endl;
    }
}

