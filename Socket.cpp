#include "inc/Socket.hpp"

Socket::Socket() 
{
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Socket creation failed");
        exit(1);
    }
}

Socket::~Socket() 
{
    close(socket_fd);
}

void Socket::setupServerSocket(int port)
{
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons(port);
    memset(socket_address.sin_zero, '\0', sizeof(socket_address.sin_zero));
    
    int     on;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("In socket");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
        perror("In setsockopt");
        exit(EXIT_FAILURE);
    }
    if (bind(socket_fd, (struct sockaddr *)&socket_address, sizeof(socket_address))<0) {
        perror("In bind");
        already_bind = 1;
        exit(EXIT_FAILURE);
    } if (listen(socket_fd, 255) < 0) {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
}

int Socket::get_fd() const
{
    return socket_fd;
}

struct sockaddr_in Socket::get_address() const
{
    return socket_address;
}

bool Socket::get_already_bind() const
{
    return already_bind;
}