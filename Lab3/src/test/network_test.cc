#include "core.hpp"
#include "network.hpp"

void network_client_test()
{
    Network client("127.0.0.1", 8001);
    while(client.reconnect()) {  }
    std::string rmsg;
    std::cout << client.sendAndRecv("Hello, I am client!", rmsg) << std::endl;//协调者
    std::cout << rmsg << std::endl;

}

void network_server_test()
{
    Network server(8001);
    while(server.reaccept()) {  } 
    std::string rmsg;

    std::cout << server.recv(rmsg) << std::endl;//参与者
    std::cout << rmsg << std::endl;

    std::cout << server.send("Hi, I am server!") << std::endl;

}