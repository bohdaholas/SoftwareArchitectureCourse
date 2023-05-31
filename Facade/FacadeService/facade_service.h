// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef _FACADE_SERVICE_H_
#define _FACADE_SERVICE_H_

#include <chrono>
#include <string>
#include <future>
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
#define BOOST_THREAD_PROVIDES_VARIADIC_THREAD
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/thread.hpp>
#include <hazelcast/client/hazelcast_client.h>
#include "options_parser.h"
#include "facade_model.h"

using namespace boost::asio;
using namespace boost::asio::ip;

class FacadeService : public std::enable_shared_from_this<FacadeService> {
public:
    using micros_response_t = std::vector<boost::future<void>>;
    FacadeService(const std::shared_ptr<tcp::socket>& logging_service_socket,
                  const std::shared_ptr<tcp::socket>& message_service_socket,
                  const std::string &mq_name);

    void set_request(const boost::beast::http::request<boost::beast::http::string_body>& request);

    void post_message();
    boost::future<micros_response_t> retrieve_messages();

    void write_micro_request(tcp::socket& socket,
                             boost::beast::http::verb request_type);
    void write_micro_request_cb(const boost::system::error_code& error,
                                tcp::socket& socket,
                                boost::beast::http::verb request_type);
    void read_micro_response(tcp::socket &socket);
    void read_micro_response_cb(const boost::system::error_code &error,
                                std::size_t bytes_transferred,
                                tcp::socket &socket);


    boost::beast::flat_buffer buffer;
    boost::beast::http::request<boost::beast::http::string_body> request;
    boost::beast::http::response<boost::beast::http::string_body> response;
    std::string messages_str;

    std::shared_ptr<tcp::socket> logging_service_socket;
    std::shared_ptr<tcp::socket> message_service_socket;
    std::unordered_map<int, boost::promise<void>> socket_promise_map;

    hazelcast::client::client_config clint_config;
    std::shared_ptr<hazelcast::client::hazelcast_client> hz_client;
    std::shared_ptr<hazelcast::client::iqueue> queue;
};

#endif