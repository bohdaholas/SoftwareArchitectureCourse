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
#include <ppconsul/agent.h>
#include "options_parser.h"
#include "facade_service.h"

using namespace boost::asio;
using namespace boost::asio::ip;

enum class request_type_t {
    HealthCheck,
    MicroserviceCommunication
};

class Controller {
public:
    virtual void run() = 0;
};

class FacadeController : public Controller {
public:
    FacadeController(const config_options_t &opt);

    void register_microservice();
    void run() override;

    virtual ~FacadeController();

private:
    config_options_t opt;
    ppconsul::Consul consul;
    ppconsul::agent::Agent agent;

    class Server;

    class Session : public std::enable_shared_from_this<Session> {
    public:
        explicit Session(FacadeController::Server& server,
                         tcp::socket client_socket,
                         const std::shared_ptr<tcp::socket>& logging_service_socket,
                         const std::shared_ptr<tcp::socket>& message_service_socket,
                         const std::string &mq_name);

        void start();

    private:
        std::shared_ptr<FacadeService> facade_service;
        tcp::socket client_socket;
        boost::beast::flat_buffer buffer;
        boost::beast::http::request<boost::beast::http::string_body> request;
        boost::beast::http::response<boost::beast::http::string_body> response;
        Server &server;

        void read_client_request();
        void handle_user_request();
        void handle_healthcheck_request();
        request_type_t parse_request_type();
        void read_client_request_cb(boost::system::error_code ec);
        void write_client_response(tcp::socket &socket, bool verbose);
        void write_client_response_cb(boost::system::error_code error,
                                      bool verbose);
    };

    class Server {
    public:
        Server(io_service &io_service, ppconsul::Consul& consul,  int facade_microservice_port);

    private:
        void connect_to_microservice(io_service& io_service,
                                     const std::string &microservice_name,
                                     std::vector<std::shared_ptr<tcp::socket>>& microservice_sockets);
        void connect_to_other_microservices(io_service &io_service);
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

        ppconsul::Consul& consul;
        tcp::acceptor acceptor;
        tcp::socket client_socket;
        std::vector<std::shared_ptr<tcp::socket>> message_service_sockets;
        std::vector<std::shared_ptr<tcp::socket>> logging_service_sockets;
        std::vector<std::shared_ptr<Session>> active_sessions;
    };
};

#endif