#include <iostream>
#include <utility>
#include <unordered_map>
#include <memory>
#include <random>
#include <chrono>
#include <thread>
#include <boost/beast/http.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>
#include <boost/bind.hpp>
#include <boost/asio/io_service.hpp>
#include "server.h"

using namespace std::chrono_literals;
using std::cout, std::cerr, std::endl;
namespace http = boost::beast::http;

constexpr int RETRY_COUNT = 2;
static int req_resp_counter = 0;
static std::unordered_map<int, std::string> socket_name_map;

Session::Session(tcp::socket client_socket,
                 std::shared_ptr<tcp::socket> logging_service_socket,
                 std::shared_ptr<tcp::socket> message_service_socket)
        : client_socket(std::move(client_socket)),
          logging_service_socket(std::move(logging_service_socket)),
          message_service_socket(std::move(message_service_socket)) {
}

void Session::start() {
    read_client_request();
}

void Session::read_client_request() {
    auto self(shared_from_this());
    http::async_read(client_socket, buffer, request,
                     boost::bind(&Session::read_client_request_cb, self,
                                 std::placeholders::_1,
                                 std::placeholders::_2));
}

void Session::read_client_request_cb(boost::system::error_code error,
                                     std::size_t bytes_transferred) {
    if (error && error.value() != boost::asio::error::eof &&
        error.value() != boost::system::errc::success && error != http::error::end_of_stream) {
        cout << "[!] Error occurred while reading request from the client: " << error.what() << endl;
    } else {
        req_resp_counter++;
        std::stringstream request_ss;
        request_ss << request;
        request_str = request_ss.str();
        if (request.method() == boost::beast::http::verb::get) {
            cout << "[" << req_resp_counter << "] Receive GET request, forwarding request to LS and MS" << endl;
            write_micro_request(*logging_service_socket, boost::beast::http::verb::get);
            write_micro_request(*message_service_socket, boost::beast::http::verb::get);
        } else if (request.method() == boost::beast::http::verb::post) {
            cout << "[" << req_resp_counter << "] Receive POST request, forwarding request to LS" << endl;
            std::stringstream modified_request_body_ss;
            modified_request_body_ss << generate_uuid(request_str) << "=" << request.body();
            request.body() = modified_request_body_ss.str();
            request.prepare_payload();
            write_micro_request(*logging_service_socket, boost::beast::http::verb::post);
        } else {
            cout << "[?] Unknown method" << endl;
        }
    }
}

std::string Session::generate_uuid(const std::string &request_str) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10e3, 10e8);
    size_t request_hash{std::hash<uint64_t>{}(dis(gen))};

    std::stringstream ss;
    ss << std::hex << request_hash;

    const size_t hash_len = 5;
    return ss.str().substr(0, hash_len);
}

void Session::write_micro_request(tcp::socket &socket, boost::beast::http::verb request_type) {
    auto self(shared_from_this());
    http::async_write(socket, request,
                      boost::bind(&Session::write_micro_request_cb, self,
                                  std::placeholders::_1, std::ref(socket), request_type));
}

void Session::write_micro_request_cb(const boost::system::error_code &error,
                                     tcp::socket &socket,
                                     boost::beast::http::verb request_type) {
    if (error && error != boost::asio::error::broken_pipe) {
        cout << "[!] Error occurred while sending data to microservices: " << error.what() << endl;
    } else {
        if (request_type == boost::beast::http::verb::get) {
            read_micro_response(socket);
        }
    }
}

void Session::read_micro_response(tcp::socket &socket) {
    auto self(shared_from_this());
    http::async_read(socket, buffer, response,
                      boost::bind(&Session::read_micro_response_cb, self,
                                  boost::asio::placeholders::error,
                                  boost::asio::placeholders::bytes_transferred,
                                  std::ref(socket)));
}

void Session::read_micro_response_cb(const boost::system::error_code &error,
                                     std::size_t bytes_transferred,
                                     tcp::socket &socket) {
    if (error && error.value() != boost::asio::error::eof &&
        error.value() != boost::system::errc::success && error != http::error::end_of_stream) {
            std::cout << "[!] Error occurred while receiving response from microservices: " << error.what() << std::endl;
    } else if (bytes_transferred != 0) {
        req_resp_counter++;
        cout << "[" << req_resp_counter <<
            "] Receive response from " << socket_name_map[socket.native_handle()] << " request:" << endl;
        cout << "\"" << response.body() << "\"" << endl;
        write_client_response(socket);
    }
}

void Session::write_client_response(tcp::socket &socket) {
    auto self(shared_from_this());
    http::async_write(client_socket, response,
                      boost::bind(&Session::write_client_response_cb, self, boost::asio::placeholders::error));
}

void Session::write_client_response_cb(boost::system::error_code error) {
    if (error) {
        cout << "[!] Error occurred while writing response to the client: " << error.what() << endl;
    } else {
        req_resp_counter++;
        cout << "[" << req_resp_counter << "] Sent response to the user" << endl;
    }
}

Server::Server(io_service &io_service, const config_options_t &opt)
        : acceptor(io_service, tcp::endpoint(tcp::v4(), opt.facade_service_port)),
          client_socket(io_service) {
    connect_to_other_microservices(io_service, opt);
    accept_client_connection();
}

void Server::async_connect_cb(const boost::system::error_code& error,
                              tcp::socket& socket,
                              const std::string& service_name) {
    if (!error) {
        socket_name_map[socket.native_handle()] = service_name;
        std::cout << "[^_^] Connected to " << service_name << " successfully!" << std::endl;
    } else {
        std::cout << "[!] Error connecting to " << service_name << ": " << error.message() << std::endl;
    }
}

void Server::async_connect_with_retry(tcp::socket &socket,
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

void Server::connect_to_other_microservices(io_service &io_service, const config_options_t &opt) {
    tcp::endpoint logging_service_endpoint(ip::address::from_string("127.0.0.1"), opt.logging_service_port);
    tcp::endpoint message_service_endpoint(ip::address::from_string("127.0.0.1"), opt.message_service_port);

    logging_service_socket = std::make_shared<tcp::socket>(io_service);
    message_service_socket = std::make_shared<tcp::socket>(io_service);

    async_connect_with_retry(*logging_service_socket, logging_service_endpoint, RETRY_COUNT,
                             "logging service", Server::async_connect_cb);
    async_connect_with_retry(*message_service_socket, message_service_endpoint, RETRY_COUNT,
                             "message service", Server::async_connect_cb);
}

void Server::accept_client_connection() {
    acceptor.async_accept(client_socket,
                          [this](boost::system::error_code error) {
                              if (!error) {
                                  std::make_shared<Session>(std::move(client_socket),
                                                            logging_service_socket,
                                                            message_service_socket)->start();
                              }
                              accept_client_connection();
                          });
}

