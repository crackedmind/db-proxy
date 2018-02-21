#include <boost/asio.hpp>

#include "debug.hpp"
#include "parser.hpp"
#include "logger.hpp"

namespace net = boost::asio;

namespace db_proxy
{
    enum { max_data_length = 8192 }; // 8KB

    class session : public std::enable_shared_from_this<session>
    {
    public:

        using ptr_type = std::shared_ptr<session>;

        session(net::io_context& ios)
            : client_socket_(ios), server_socket_(ios)
        {
        }

        net::ip::tcp::socket& client_socket()
        {
            return client_socket_;
        }

        net::ip::tcp::socket& server_socket()
        {
            return server_socket_;
        }

        void start(const std::string& server_host, unsigned short server_port)
        {
            server_socket_.async_connect(
                        net::ip::tcp::endpoint(
                            net::ip::make_address(server_host),
                            server_port),
                        std::bind(&session::handle_server_connect,
                                  shared_from_this(),
                                  std::placeholders::_1));
        }

        void handle_server_connect(const boost::system::error_code& error)
        {
            if (!error)
            {
                std::cout << "Client connected from " << client_socket_.local_endpoint().address() << '\n';
                server_socket_.async_read_some(
                            net::buffer(server_data_, max_data_length),
                            std::bind(&session::handle_server_read,
                                      shared_from_this(),
                                      std::placeholders::_1,
                                      std::placeholders::_2));

                client_socket_.async_read_some(
                            net::buffer(client_data_, max_data_length),
                            std::bind(&session::handle_client_read,
                                      shared_from_this(),
                                      std::placeholders::_1,
                                      std::placeholders::_2));
                }
            else
                close();
        }

    private:
        void handle_server_read(const boost::system::error_code& error,
                            const size_t& bytes_transferred)
        {
            if (!error)
            {
                parser_.parse(server_data_, bytes_transferred);

                async_write(client_socket_,
                            net::buffer(server_data_, bytes_transferred),
                            std::bind(&session::handle_client_write,
                                      shared_from_this(),
                                      std::placeholders::_1));
            }
            else
                close();
        }

        void handle_client_write(const boost::system::error_code& error)
        {
            if (!error)
            {
                server_socket_.async_read_some(
                            net::buffer(server_data_, max_data_length),
                            std::bind(&session::handle_server_read,
                                      shared_from_this(),
                                      std::placeholders::_1,
                                      std::placeholders::_2));
            }
            else
                close();
        }

        void handle_client_read(const boost::system::error_code& error,
                  const size_t& bytes_transferred)
        {
            if (!error)
            {
                parser_.parse(client_data_, bytes_transferred);

                async_write(server_socket_,
                            net::buffer(client_data_,bytes_transferred),
                            std::bind(&session::handle_server_write,
                                      shared_from_this(),
                                      std::placeholders::_1));
            }
            else
                close();
        }

        void handle_server_write(const boost::system::error_code& error)
        {
            if (!error)
            {
                client_socket_.async_read_some(
                            net::buffer(client_data_,max_data_length),
                            std::bind(&session::handle_client_read,
                                      shared_from_this(),
                                      std::placeholders::_1,
                                      std::placeholders::_2));
            }
            else
                close();
        }

        void close()
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (client_socket_.is_open())
                client_socket_.close();

            if (server_socket_.is_open())
                server_socket_.close();
        }

        net::ip::tcp::socket client_socket_;
        net::ip::tcp::socket server_socket_;

        uint8_t client_data_[max_data_length] = {0};
        uint8_t server_data_[max_data_length] = {0};

        std::mutex mutex_;
        My::Parser parser_;
    };

    class server
    {
    public:

        server(net::io_context& io_service,
              const std::string& local_host, unsigned short local_port,
              const std::string& server_host, unsigned short server_port)
        : io_service_(io_service),
          localhost_address(net::ip::make_address_v4(local_host)),
          server_(io_service_,net::ip::tcp::endpoint(localhost_address,local_port)),
          server_port_(server_port),
          server_host_(server_host)
        {}

        bool accept_connections()
        {
            try
            {
                session_ = std::make_shared<session>(io_service_);

                server_.async_accept(session_->client_socket(),
                                       std::bind(&server::handle_accept,
                                                 this,
                                                 std::placeholders::_1));
            }
            catch(std::exception& e)
            {
                std::cerr << "server exception: " << e.what() << std::endl;
                return false;
            }

            return true;
        }

    private:

        void handle_accept(const boost::system::error_code& error)
        {
            if (!error)
            {
                session_->start(server_host_,server_port_);

                if (!accept_connections())
                {
                    std::cerr << "Failure during call to accept." << std::endl;
                }
            }
            else
            {
               std::cerr << "Error: " << error.message() << std::endl;
            }
        }

        net::io_context& io_service_;
        net::ip::address_v4 localhost_address;
        net::ip::tcp::acceptor server_;
        session::ptr_type session_;
        unsigned short server_port_;
        std::string server_host_;
    };
}

class CmdOptions {
private:
    int argc_;
    char **argv_;
public:
    unsigned short  bind_port = 8080;
    unsigned short  remote_port = 0;
    std::string     bind_host = "127.0.0.1";
    std::string     remote_host = "";

    CmdOptions(int argc, char **argv) : argc_(argc), argv_(argv) {
    }

    bool parse() {
        for(int i = 1; i < argc_; i++) {
            std::string arg = argv_[i];

            if(arg == "--remote-port")
                remote_port = static_cast<unsigned short>(std::stoi(argv_[++i]));
            if(arg == "--bind-port")
                remote_port = static_cast<unsigned short>(std::stoi(argv_[++i]));
            if(arg == "--bind-host")
                bind_host = argv_[++i];
            if(arg == "--remote-host")
                remote_host = argv_[++i];
            if(arg == "--help") {
                help();
                return true;
            }
        }

        return required_missing();
    }

    bool required_missing() {
        return remote_host.empty() || remote_port == 0;
    }

    void help() {
        std::cout << "Usage: " << argv_[0] << " [options]" << '\n';
        std::cout << "  Options:\n";
        std::cout << "    --remote-port arg" << "\t Remote DB port\n";
        std::cout << "    --remote-host arg" << "\t Remote DB host\n";
        std::cout << "    --bind-port [arg]" << "\t Local port to listen. Default: " << bind_port << '\n';
        std::cout << "    --bind-host [arg]" << "\t Local adress to listen.  Default: " << bind_host << '\n';
    }
};

static net::io_context ios;

BOOL WINAPI CtrlHandler(DWORD dwCtrlType) {
    switch (dwCtrlType)
    {
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_C_EVENT:
        if(!ios.stopped())
            ios.stop();
        return TRUE;
    default:
        return FALSE;
    }

}
int main(int argc, char** argv)
{
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    auto logger = LoggerRegistry::instance().create_file("logger", "db-proxy.log");

    CmdOptions options(argc, argv);

    bool res = options.parse();

    if (res) {
        std::cout << "Missed required options\n\n";
        options.help();
        return EXIT_FAILURE;
    }

    try
    {
        db_proxy::server server(ios,
                                options.bind_host, options.bind_port,
                                options.remote_host, options.remote_port);

        server.accept_connections();

        ios.run();
    }
    catch(std::exception& e)
    {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
    }

    return EXIT_SUCCESS;
}
