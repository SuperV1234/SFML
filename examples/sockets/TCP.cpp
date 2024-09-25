////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "TCP.hpp"

#include "SFML/Network/IpAddress.hpp"
#include "SFML/Network/Socket.hpp"
#include "SFML/Network/TcpListener.hpp"
#include "SFML/Network/TcpSocket.hpp"

#include "SFML/Base/Optional.hpp"

#include <iostream>

#include <cstddef>


////////////////////////////////////////////////////////////
/// Launch a server, wait for an incoming connection,
/// send a message and wait for the answer.
///
////////////////////////////////////////////////////////////
void runTcpServer(unsigned short port)
{
    // Create a server socket to accept new connections
    sf::TcpListener listener(/* isBlocking */ true);

    // Listen to the given port for incoming connections
    if (listener.listen(port) != sf::Socket::Status::Done)
        return;
    std::cout << "Server is listening to port " << port << ", waiting for connections... " << std::endl;

    // Wait for a connection
    sf::TcpSocket socket(/* isBlocking */ true);
    if (listener.accept(socket) != sf::Socket::Status::Done)
        return;
    std::cout << "Client connected: " << socket.getRemoteAddress().value() << std::endl;

    // Send a message to the connected client
    const char out[] = "Hi, I'm the server";
    if (socket.send(out, sizeof(out)) != sf::Socket::Status::Done)
        return;
    std::cout << "Message sent to the client: \"" << out << '"' << std::endl;

    // Receive a message back from the client
    char        in[128];
    std::size_t received = 0;
    if (socket.receive(in, sizeof(in), received) != sf::Socket::Status::Done)
        return;
    std::cout << "Answer received from the client: \"" << in << '"' << std::endl;
}


////////////////////////////////////////////////////////////
/// Create a client, connect it to a server, display the
/// welcome message and send an answer.
///
////////////////////////////////////////////////////////////
void runTcpClient(unsigned short port)
{
    // Ask for the server address
    sf::base::Optional<sf::IpAddress> server;
    do
    {
        std::cout << "Type the address or name of the server to connect to: ";
        std::cin >> server;
    } while (!server.hasValue());

    // Create a socket for communicating with the server
    sf::TcpSocket socket(/* isBlocking */ true);

    // Connect to the server
    if (socket.connect(server.value(), port) != sf::Socket::Status::Done)
        return;
    std::cout << "Connected to server " << server.value() << std::endl;

    // Receive a message from the server
    char        in[128];
    std::size_t received = 0;
    if (socket.receive(in, sizeof(in), received) != sf::Socket::Status::Done)
        return;
    std::cout << "Message received from the server: \"" << in << '"' << std::endl;

    // Send an answer to the server
    const char out[] = "Hi, I'm a client";
    if (socket.send(out, sizeof(out)) != sf::Socket::Status::Done)
        return;
    std::cout << "Message sent to the server: \"" << out << '"' << std::endl;
}
