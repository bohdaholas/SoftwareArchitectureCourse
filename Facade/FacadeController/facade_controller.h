// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef _FACADE_CONTROLLER_H_
#define _FACADE_CONTROLLER_H_

#include <chrono>
#include <string>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <boost/bind.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/placeholders.hpp>
#include "options_parser.h"
#include "facade_service.h"

using namespace boost::asio;
using namespace boost::asio::ip;

class Controller {
public:
    virtual void run() = 0;
};

class FacadeController : public Controller {
public:
    FacadeController(const config_options_t &opt);

    void run() override;

private:
    config_options_t opt;

    class Server;

    class Session : public std::enable_shared_from_this<Session> {
    public:
        explicit Session(Server& server,
                         tcp::socket socket,
                         const std::shared_ptr<tcp::socket>& logging_service_socket,
                         const std::shared_ptr<tcp::socket>& message_service_socket);

        void start();

    private:
        std::shared_ptr<FacadeService> facade_service;

        void read_client_request();
        void read_client_request_cb(boost::system::error_code ec,
                                    std::size_t bytes_transferred);
        void write_client_response(tcp::socket &socket);
        void write_client_response_cb(boost::system::error_code error);

        tcp::socket client_socket;
        boost::beast::flat_buffer buffer;
        boost::beast::http::request<boost::beast::http::string_body> request;
        boost::beast::http::response<boost::beast::http::string_body> response;
        Server &server;
    };

    class Server {
    public:
        Server(boost::asio::io_service &io_service, const config_options_t& opt);

    private:
        void connect_to_other_microservices(io_service &io_service, const config_options_t& opt);
        void async_connect_with_retry(tcp::socket &socket,
                                      const tcp::endpoint &endpoint,
                                      int retry_count,
                                      const std::string& service_name,
                                      const std::function<void(const boost::system::error_code &,
                                                               tcp::socket& socket,
                                                               const std::string& service_name)>& callback);
        static void async_connect_cb(const boost::system::error_code& ec,
                                     tcp::socket& socket,
                                     const std::string& service_name);
        void accept_client_connection();
        void remove_session(Session& inactive_session);

        friend class FacadeController::Session;

        const config_options_t& opt;
        tcp::acceptor acceptor;
        tcp::socket client_socket;
        std::shared_ptr<tcp::socket> message_service_socket;
        std::vector<std::shared_ptr<tcp::socket>> logging_service_sockets;
        std::vector<std::shared_ptr<Session>> active_sessions;
    };
};

#endif