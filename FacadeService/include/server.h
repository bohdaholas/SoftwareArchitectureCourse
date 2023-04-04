#ifndef _SERVER_H_
#define _SERVER_H_

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

using namespace boost::asio;
using namespace boost::asio::ip;

constexpr size_t ONE_KB = 1024;
constexpr size_t ONE_MB = 1000 * ONE_KB;
constexpr size_t BUFFER_SIZE = ONE_MB;

class Session : public std::enable_shared_from_this<Session> {
public:
    explicit Session(tcp::socket socket,
                     std::shared_ptr<tcp::socket> logging_service_socket,
                     std::shared_ptr<tcp::socket> message_service_socket);
    void start();

private:
    void read_client_request();
    void read_client_request_cb(boost::system::error_code ec,
                                std::size_t bytes_transferred);
    std::string generate_uuid(const std::string& request_str);
    void write_micro_request(tcp::socket &socket, boost::beast::http::verb request_type);
    void write_micro_request_cb(const boost::system::error_code &error,
                                tcp::socket &socket,
                                boost::beast::http::verb request_type);
    void read_micro_response(tcp::socket &socket);
    void read_micro_response_cb(const boost::system::error_code &error,
                                std::size_t bytes_transferred,
                                tcp::socket &socket);
    void write_client_response(tcp::socket &socket);
    void write_client_response_cb(boost::system::error_code error);


    tcp::socket client_socket;
    std::shared_ptr<tcp::socket> logging_service_socket;
    std::shared_ptr<tcp::socket> message_service_socket;

    boost::beast::flat_buffer buffer;
    boost::beast::http::request<boost::beast::http::string_body> request;
    boost::beast::http::response<boost::beast::http::string_body> response;

    char data_buffer[BUFFER_SIZE];
    std::string request_str;
};

class Server {
public:
    Server(boost::asio::io_service &io_service, const config_options_t& opt);

private:
    void connect_to_other_microservices(io_service &io_service, const config_options_t& opt);
    static void async_connect_cb(const boost::system::error_code& ec,
                          tcp::socket& socket,
                          const std::string& service_name);
    void async_connect_with_retry(tcp::socket &socket,
                                  const tcp::endpoint &endpoint,
                                  int retry_count,
                                  const std::string& service_name,
                                  const std::function<void(const boost::system::error_code &,
                                                           tcp::socket& socket,
                                                           const std::string& service_name)>& callback);
    void accept_client_connection();

    tcp::acceptor acceptor;
    tcp::socket client_socket;
    std::shared_ptr<tcp::socket> logging_service_socket;
    std::shared_ptr<tcp::socket> message_service_socket;
};

#endif