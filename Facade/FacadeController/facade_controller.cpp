// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <iostream>
#include <utility>
#include <unordered_map>
#include <memory>
#include <random>
#include <chrono>
#include <boost/beast/http.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>
#include <boost/asio/io_service.hpp>
#include <ppconsul/catalog.h>
#include "facade_controller.h"
#include "facade_service.h"

using namespace std::chrono_literals;
using std::cout, std::cerr, std::endl;
namespace http = boost::beast::http;

using ppconsul::Consul;
using namespace ppconsul::agent;
using namespace ppconsul::catalog;

constexpr int CONSUL_PORT = 8500;
constexpr int RETRY_COUNT = 2;
const std::string MICROSERVICE_NAME = "facade_microservice";

static int facade_microservice_port;

static bool is_port_busy(int port) {
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::acceptor acceptor(io_service);
    boost::system::error_code ec;

    acceptor.open(boost::asio::ip::tcp::v4(), ec);
    if (ec) {
        return true; // error opening the acceptor means port is busy
    }

    acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port), ec);
    if (ec) {
        return true; // error binding to the port means port is busy
    }

    acceptor.close();
    return false; // port is not busy
}

static int get_first_usable_port(int start_port) {
    int test_port = start_port;
    while (is_port_busy(test_port))
        test_port++;
    return test_port;
}

FacadeController::FacadeController(const config_options_t &opt) : opt(opt), agent(consul) {}

void FacadeController::register_microservice() {
    agent.registerService(
            ppconsul::agent::kw::name = MICROSERVICE_NAME,
            ppconsul::agent::kw::port = facade_microservice_port,
            ppconsul::agent::kw::check = HttpCheck{"http://localhost:" + std::to_string(facade_microservice_port) + "/health", std::chrono::seconds(5)}
    );
}

void FacadeController::run() {
    try {
        boost::asio::io_service io_service;
        facade_microservice_port = get_first_usable_port(opt.start_port);
        cout << "--- Facade microservice #" << facade_microservice_port << " is on!" << endl;
        register_microservice();
        Server server(io_service, consul, facade_microservice_port);
        io_service.run();
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

FacadeController::~FacadeController() {
    cout << "destructor called" << endl;
    agent.deregisterService(MICROSERVICE_NAME);
}

FacadeController::Server::Server(io_service &io_service, ppconsul::Consul& consul, int facade_microservice_port)
        : acceptor(io_service, tcp::endpoint(tcp::v4(), facade_microservice_port)),
          consul(consul),
          client_socket(io_service) {
    connect_to_other_microservices(io_service);
    accept_client_connection();
}

void FacadeController::Server::connect_to_microservice(io_service &io_service,
                                                       const std::string &microservice_name,
                                                       std::vector<std::shared_ptr<tcp::socket>>& microservice_sockets) {
    Catalog catalog(consul);
    auto service_entries = catalog.service(microservice_name);
    microservice_sockets.reserve(service_entries.size());
    cout << service_entries.size() << endl;
    for (const auto& service_entry : service_entries) {
        int microservice_port = service_entry.second.port;
        tcp::endpoint service_endpoint(ip::address::from_string("127.0.0.1"), microservice_port);
        microservice_sockets.emplace_back(std::make_shared<tcp::socket>(io_service));
        std::string microservice_id = service_entry.second.id;
        async_connect_with_retry(*microservice_sockets.back(), service_endpoint, RETRY_COUNT,
                                 microservice_id, FacadeController::Server::async_connect_cb);
    }
}

void FacadeController::Server::connect_to_other_microservices(io_service &io_service) {
    std::vector<std::pair<std::string, std::vector<std::shared_ptr<tcp::socket>>&>> microservice_data = {
            {"logging_microservice", logging_service_sockets},
            {"message_microservice", message_service_sockets}
    };
    for (auto &[microservice_name, microservice_sockets]: microservice_data) {
        connect_to_microservice(io_service, microservice_name, microservice_sockets);
    }
}

void FacadeController::Server::async_connect_with_retry(tcp::socket &socket,
                                                        const tcp::endpoint &endpoint,
                                                        int retry_count,
                                                        const std::string& service_name,
                                                        const std::function<void(const boost::system::error_code &error,
                                                                                 tcp::socket& socket,
                                                                                 const std::string& service_name)>& callback) {
    socket.async_connect(endpoint, [=, &socket](const boost::system::error_code &error) {
        if (error) {
            // connection failed
            if (retry_count > 0) {
                // retry again
                std::cout << "[!] Failed connecting to " << service_name << ", retrying..." << std::endl;
                // TODO: fix timer
                boost::asio::steady_timer timer(socket.get_executor());
                timer.expires_after(std::chrono::seconds(1)); // wait for 1 second before retrying
                timer.async_wait([=, &socket](const boost::system::error_code& error) {
                    async_connect_with_retry(socket, endpoint, retry_count - 1, service_name, callback);
                });
            } else {
                // out of retries
                callback(error, socket, service_name);
            }
        } else {
            // connection succeeded
            callback(error, socket, service_name);
        }
    });
}

void FacadeController::Server::async_connect_cb(const boost::system::error_code& error,
                                                tcp::socket& socket,
                                                const std::string& service_name) {
    if (!error) {
        ServiceManager::get_instance().set_name(socket.native_handle(), service_name);
        std::cout << "[^_^] Connected to " << service_name << " successfully!" << std::endl;
    } else {
        std::cout << "[!] Error connecting to " << service_name << ": " << error.message() << std::endl;
    }
}

void FacadeController::Server::accept_client_connection() {
    acceptor.async_accept(client_socket,
                          [this](boost::system::error_code error) {
                              static size_t logging_service_idx = 0;
                              static size_t message_service_idx = 0;
                              size_t logging_service_count = logging_service_sockets.size();
                              size_t message_service_count = message_service_sockets.size();
                              auto logging_service_socket = logging_service_count == 0 ? nullptr : logging_service_sockets[logging_service_idx];
                              auto message_service_socket = message_service_count == 0 ? nullptr : message_service_sockets[message_service_idx];
                              if (!error) {
                                  std::string s{"messages_queue"};
                                  auto session = std::make_shared<Session>(*this, std::move(client_socket),
                                                                           logging_service_socket,
                                                                           message_service_socket,
                                                                           s);
                                  session->start();
                                  active_sessions.push_back(session);
                              }
                              if (logging_service_count)
                                  logging_service_idx = (logging_service_idx + 1) % logging_service_count;
                              if (message_service_count)
                                  message_service_idx = (message_service_idx + 1) % message_service_count;
                              accept_client_connection();
                          });
}

void FacadeController::Server::remove_session(Session& inactive_session) {
    auto delete_inactive = [&inactive_session] (const std::shared_ptr<Session>& session_ptr) { return session_ptr.get() == &inactive_session;};
    active_sessions.erase(
            std::remove_if(active_sessions.begin(), active_sessions.end(), delete_inactive),
            active_sessions.end()
    );
}

FacadeController::Session::Session(FacadeController::Server& server,
                 tcp::socket client_socket,
                 const std::shared_ptr<tcp::socket>& logging_service_socket,
                 const std::shared_ptr<tcp::socket>& message_service_socket,
                 const std::string &mq_name)
        : server{server},
          client_socket(std::move(client_socket)),
          facade_service(std::make_shared<FacadeService>(logging_service_socket,
                                                         message_service_socket,
                                                         mq_name)) {}

void FacadeController::Session::start() {
    read_client_request();
}

void FacadeController::Session::read_client_request() {
    auto self(shared_from_this());
    http::async_read(client_socket, buffer, request,
                     boost::bind(&FacadeController::Session::read_client_request_cb, self,
                                 boost::asio::placeholders::error));
}

request_type_t FacadeController::Session::parse_request_type() {
    if (request.target() == "/health")
        return request_type_t::HealthCheck;
    return request_type_t::MicroserviceCommunication;
}

void FacadeController::Session::handle_user_request() {
    Counter::get_instance().increment();
    facade_service->set_request(request);
    if (request.method() == http::verb::get) {
        cout << "[" << Counter::get_instance().get_count() << "] Receive GET request, forwarding request to LS and MS" << endl;
        auto future = facade_service->retrieve_messages();
        future.then([&](boost::future<FacadeService::micros_response_t>&& f) {
            try {
                if (facade_service->messages_str.ends_with("\n")) {
                    facade_service->messages_str.pop_back();
                }
                response.body() = facade_service->messages_str;
                write_client_response(client_socket, true);
            } catch (const std::exception& ex) {
                std::cerr << "Exception: " << ex.what() << std::endl;
            }
        });
    } else if (request.method() == http::verb::post) {
        cout << "[" << Counter::get_instance().get_count() << "] Receive POST request, forwarding request to LS and MS" << endl;
        facade_service->post_message();
        write_client_response(client_socket, true);
    } else {
        cout << "[?] Unknown method" << endl;
    }
}

void FacadeController::Session::handle_healthcheck_request() {
    std::stringstream ss;
    ss << MICROSERVICE_NAME << " #" << facade_microservice_port;
    response.body() = ss.str();
    write_client_response(client_socket, false);
}

void FacadeController::Session::read_client_request_cb(boost::system::error_code error) {
    if (error) {
        cout << "[!] Error occurred while reading request from the client: " << error.what() << endl;
    } else {
        switch (parse_request_type()) {
            case request_type_t::HealthCheck:
                handle_healthcheck_request();
                break;
            case request_type_t::MicroserviceCommunication:
                handle_user_request();
                break;
        }
    }
}

void FacadeController::Session::write_client_response(tcp::socket &socket, bool verbose) {
    auto self(shared_from_this());
    http::async_write(socket, response,
                      boost::bind(&FacadeController::Session::write_client_response_cb, self,
                                  boost::asio::placeholders::error,
                                  verbose));
}

void FacadeController::Session::write_client_response_cb(boost::system::error_code error, bool verbose) {
    if (error) {
        cout << "[!] Error occurred while writing response to the client: " << error.what() << endl;
    } else {
        if (verbose) {
            Counter::get_instance().increment();
            cout << "[" << Counter::get_instance().get_count() << "] Sent response to the user" << endl;
        }
        server.remove_session(*this);
    }
}
