#include <boost/asio.hpp>
#include <iostream>
#include <memory>

namespace bip = boost::asio::ip;
namespace basio = boost::asio;
namespace bsystem = boost::system;

class Session: public std::enable_shared_from_this<Session> {
public:
    
    Session(bip::tcp::socket&& mv_socket) : session_socket(std::move(mv_socket)) {
        std::cout << "Session initialized..." << std::endl;
    }

    ~Session() {
        std::cout << "Session destroy" << std::endl;
    }

    void createSession() {
        basio::async_read_until(
            session_socket,
            buffer,
            "\n",
            [this_session = shared_from_this()](bsystem::error_code errorcode, std::size_t bytes_transf)
            {
                if (!errorcode) {
                    std::cout << &this_session->buffer;
                }
            }
        );
    }

private:
    bip::tcp::socket session_socket;
    basio::streambuf buffer;
};

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
                std::shared_ptr<Session> nsession = std::make_shared<Session>(std::move(socket));
                nsession->createSession();
        
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