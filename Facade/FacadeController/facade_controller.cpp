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
#include "facade_controller.h"
#include "facade_service.h"

using namespace std::chrono_literals;
using std::cout, std::cerr, std::endl;
namespace http = boost::beast::http;

constexpr int RETRY_COUNT = 2;

FacadeController::FacadeController(const config_options_t &opt) : opt(opt) {}

void FacadeController::run() {
    try {
        boost::asio::io_service io_service;
        Server server(io_service, opt);
        io_service.run();
    }
    catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

FacadeController::Server::Server(io_service &io_service, const config_options_t &opt)
        : acceptor(io_service, tcp::endpoint(tcp::v4(), opt.facade_microservice_port)),
          client_socket(io_service),
          opt(opt) {
    connect_to_other_microservices(io_service, opt);
    accept_client_connection();
}

void FacadeController::Server::connect_to_other_microservices(io_service &io_service, const config_options_t &opt) {
    using namespace std::string_literals;
    logging_service_sockets.reserve(opt.logging_microservices_count);
    for (size_t i = 0; i < opt.logging_microservices_count; ++i) {
        tcp::endpoint logging_service_endpoint(ip::address::from_string("127.0.0.1"), opt.logging_microservice_port + i);
        logging_service_sockets.emplace_back(std::make_shared<tcp::socket>(io_service));
        std::string str_id = "logging microservice #"s + std::to_string(opt.logging_microservice_port + i);
        async_connect_with_retry(*(logging_service_sockets[i]), logging_service_endpoint, RETRY_COUNT,
                                 str_id, FacadeController::Server::async_connect_cb);
    }
    tcp::endpoint message_service_endpoint(ip::address::from_string("127.0.0.1"), opt.message_microservice_port);
    message_service_socket = std::make_shared<tcp::socket>(io_service);
    async_connect_with_retry(*message_service_socket, message_service_endpoint, RETRY_COUNT,
                             "message service", FacadeController::Server::async_connect_cb);
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
                              if (!error) {
                                  auto session = std::make_shared<Session>(*this, std::move(client_socket),
                                                                           logging_service_sockets[logging_service_idx],
                                                                           message_service_socket);
                                  session->start();
                                  active_sessions.push_back(session);
                              }
                              logging_service_idx = (logging_service_idx + 1) % opt.logging_microservices_count;
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
                 const std::shared_ptr<tcp::socket>& message_service_socket)
        : server{server},
          client_socket(std::move(client_socket)),
          facade_service(std::make_shared<FacadeService>(logging_service_socket,
                                                         message_service_socket)) {}

void FacadeController::Session::start() {
    read_client_request();
}

void FacadeController::Session::read_client_request() {
    auto self(shared_from_this());
    http::async_read(client_socket, buffer, request,
                     boost::bind(&FacadeController::Session::read_client_request_cb, self,
                                 boost::asio::placeholders::error,
                                 boost::asio::placeholders::bytes_transferred));
}

void FacadeController::Session::read_client_request_cb(boost::system::error_code error,
                                     std::size_t bytes_transferred) {
    if (error && error.value() != boost::asio::error::eof &&
        error.value() != boost::system::errc::success && error != http::error::end_of_stream) {
        cout << "[!] Error occurred while reading request from the client: " << error.what() << endl;
    } else {
        Counter::get_instance().increment();
        facade_service->set_request(request);
        if (request.method() == http::verb::get) {
            cout << "[" << Counter::get_instance().get_count() << "] Receive GET request, forwarding request to LS and MS" << endl;
            auto future = facade_service->retrieve_messages();
            future.then([&](boost::unique_future<FacadeService::micros_response_t>&& f) {
                try {
                    if (facade_service->messages_str.ends_with("\n")) {
                       facade_service->messages_str.pop_back();
                    }
                    response.body() = facade_service->messages_str;
                    write_client_response(client_socket);
                } catch (const std::exception& ex) {
                    std::cerr << "Exception: " << ex.what() << std::endl;
                }
            });
        } else if (request.method() == http::verb::post) {
            cout << "[" << Counter::get_instance().get_count() << "] Receive POST request, forwarding request to LS" << endl;
            facade_service->post_message();
            write_client_response(client_socket);
        } else {
            cout << "[?] Unknown method" << endl;
        }
    }
}

void FacadeController::Session::write_client_response(tcp::socket &socket) {
    auto self(shared_from_this());
    http::async_write(socket, response,
                      boost::bind(&FacadeController::Session::write_client_response_cb, self,
                                  boost::asio::placeholders::error));
}

void FacadeController::Session::write_client_response_cb(boost::system::error_code error) {
    if (error) {
        cout << "[!] Error occurred while writing response to the client: " << error.what() << endl;
    } else {
        Counter::get_instance().increment();
        cout << "[" << Counter::get_instance().get_count() << "] Sent response to the user" << endl;
        server.remove_session(*this);
    }
}
