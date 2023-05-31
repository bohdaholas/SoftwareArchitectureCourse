// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include <iostream>
#include <utility>
#include <memory>
#include <random>
#include <boost/beast/http.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/bind.hpp>
#include "facade_service.h"

using std::cout, std::cerr, std::endl;
namespace http = boost::beast::http;

FacadeService::FacadeService(const std::shared_ptr<tcp::socket>& logging_service_socket,
                             const std::shared_ptr<tcp::socket>& message_service_socket,
                             const std::string &mq_name) :
                             logging_service_socket(logging_service_socket),
                             message_service_socket(message_service_socket) {
    if (logging_service_socket)
        socket_promise_map[logging_service_socket->native_handle()] = boost::promise<void>();
    if (message_service_socket)
        socket_promise_map[message_service_socket->native_handle()] = boost::promise<void>();
    hazelcast::client::client_config config;
    config.get_logger_config().level(hazelcast::logger::level::off);
    hz_client = std::make_shared<hazelcast::client::hazelcast_client>(hazelcast::new_client(std::move(config)).get());
    queue = hz_client->get_queue(mq_name).get();
}

void FacadeService::set_request(const http::request<http::string_body> &req) {
    this->request = req;
}

boost::future<FacadeService::micros_response_t> FacadeService::retrieve_messages() {
    std::vector<boost::future<void>> futures;
    for (auto &[fd, promise]: socket_promise_map) {
        futures.emplace_back(promise.get_future());
    }

    if (message_service_socket)
        write_micro_request(*message_service_socket, http::verb::get);
    if (logging_service_socket)
        write_micro_request(*logging_service_socket, http::verb::get);

    return boost::when_all(futures.begin(), futures.end());
}

void FacadeService::post_message() {
    Message message(request.body());
//    cout << "writing to " << ServiceManager::get_instance().get_name(logging_service_socket->native_handle()) << " " << message.str() << endl;
    request.body() = message.str();
    request.prepare_payload();
    if (logging_service_socket) {
        write_micro_request(*logging_service_socket, http::verb::post);
    }
    queue->put(message.str()).get();
}

void FacadeService::write_micro_request(tcp::socket &socket,
                                        http::verb request_type) {
    auto self(shared_from_this());
//    cout << "writing micro" << endl;
    http::async_write(socket, request,
                      boost::bind(&FacadeService::write_micro_request_cb, self,
                                  boost::asio::placeholders::error,
                                  std::ref(socket),
                                  request_type));
}

void FacadeService::write_micro_request_cb(const boost::system::error_code &error,
                                           tcp::socket &socket,
                                           http::verb request_type) {
    if (error) {
        cout << "[!] Error occurred while sending data to microservices: " << error.what() << endl;
    } else {
        if (request_type == http::verb::get) {
            read_micro_response(socket);
        }
    }
}

void FacadeService::read_micro_response(tcp::socket &socket) {
    auto self(shared_from_this());
    http::async_read(socket, buffer, response,
                     boost::bind(&FacadeService::read_micro_response_cb, self,
                                 boost::asio::placeholders::error,
                                 boost::asio::placeholders::bytes_transferred,
                                 std::ref(socket)));
}

void FacadeService::read_micro_response_cb(const boost::system::error_code &error,
                                           std::size_t bytes_transferred,
                                           tcp::socket &socket) {
    if (error) {
        std::cout << "[!] Error occurred while receiving response from microservices: " << error.what() << std::endl;
    } else if (bytes_transferred != 0) {
        if (!response.body().empty()) {
            Counter::get_instance().increment();
            std::string name = ServiceManager::get_instance().get_name(socket.native_handle());
            cout << "[" << Counter::get_instance().get_count() <<
                 "] Receive response from " << name << " request:" << endl;
            cout << "\"" << response.body() << "\"" << endl;
            messages_str += response.body() + "\n";
        }
        socket_promise_map[socket.native_handle()].set_value();
    }
}
