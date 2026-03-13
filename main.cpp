#include <boost/asio.hpp>
#include <iostream>

namespace bip = boost::asio::ip;
namespace basio = boost::asio;
namespace bsystem = boost::system;

class Server
{
public:
    Server(basio::io_context& context, int port) : socket{context}, acceptor{context, bip::tcp::endpoint(bip::tcp::v4(), port)} 
    {
        
    }

    void asc_acceptor()
    {
        acceptor.async_accept(socket, [&](boost::system::error_code errorcode)
        {   
            if (!errorcode) {
                std::cout << "Connect success...\n";
                asc_acceptor();
            } else {
                std::cout << errorcode.message();
            }
        });
    }

private:
    int port;
    bip::tcp::socket socket;
    bip::tcp::acceptor acceptor;
};

int main()  
{
    basio::io_context context;
    int port = 4051;
    Server server(context, port);
    server.asc_acceptor();

    context.run();
}