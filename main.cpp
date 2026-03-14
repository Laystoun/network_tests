#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <functional>

namespace bip = boost::asio::ip;
namespace basio = boost::asio;
namespace bsystem = boost::system;

class Session: public std::enable_shared_from_this<Session> {
public:
    std::function<void(std::shared_ptr<Session>)> on_leave;
    std::function<void(std::shared_ptr<Session>, std::string&)> on_chat_message;
    bip::tcp::socket session_socket;

    Session(bip::tcp::socket&& mv_socket) : session_socket(std::move(mv_socket)) {
        std::cout << "Session initialized..." << std::endl;
    }

    ~Session() {
        std::cout << "Session destroy: " << session_socket.remote_endpoint() << std::endl;
    }

    void createSession() {
        basio::async_read_until(
            session_socket,
            buffer,
            "\n",
            [this_session = shared_from_this()](bsystem::error_code errorcode, std::size_t bytes_transf)
            {
                if (!errorcode) {
                    std::string msg;
                    std::istream b_str(&this_session->buffer);
                    
                    std::getline(b_str, msg);
                    msg += '\n';
                    this_session->on_chat_message(this_session, msg);
                    this_session->createSession();
                } else if (errorcode == basio::error::eof) {
                    std::cout << "Connection has broken" << std::endl;
                    this_session->on_leave(this_session);
                }
            }
        );
    }

    void sendMessage(const std::string& msg) {
        basio::async_write(session_socket, basio::buffer(msg), 
        [this_session = shared_from_this(), c_msg = msg](bsystem::error_code ec, std::size_t bytes) {
            
            if(!ec) {
                std::cout << "Succes send message..." << std::endl;
            } else {
                std::cout << ec.message();
            }
        });
    }

private:
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
                nsession->on_leave = [&](std::shared_ptr<Session> s) {
                    this->remove_session(s);
                };

                nsession->on_chat_message = [&](std::shared_ptr<Session> sender, std::string& message) {
                    send_chat_msg(sender, message);
                };

                sessions.push_back(nsession);
                std::cout << "Online: " << sessions.size() << std::endl;
                nsession->createSession();

                nsession->sendMessage("Hello user, online: " + std::to_string(sessions.size()) + "\n");

                for (auto& user : sessions) {
                    if (user != nsession) {
                        user->sendMessage("New user in chat!\n");
                    }
                }

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
    std::vector<std::shared_ptr<Session>> sessions;

    void remove_session(std::shared_ptr<Session> s) {
        auto rv = std::find(sessions.begin(), sessions.end(), s);
        if (rv != sessions.end()) {
            sessions.erase(rv);
            std::cout << "Connection removed... (Online: " << sessions.size() << ")" << std::endl;
        }
    }

    void send_chat_msg(std::shared_ptr<Session> sender, std::string& message) {
        for (auto& user : sessions) {
            if (user != sender) {
                std::stringstream r_message;
                r_message << sender->session_socket.remote_endpoint() << ": " << message;

                user->sendMessage(r_message.str());
            }
        }
    }
};

int main()  
{
    basio::io_context context;
    int port = 4051;
    Server server(context, port);
    server.asc_acceptor();
    std::cout << "Start..." << std::endl;
    context.run();
}