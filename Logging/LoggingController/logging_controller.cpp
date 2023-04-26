#include <iostream>
#include <unordered_map>
#include <boost/asio.hpp>
#include "logging_controller.h"
#include "logging_service.h"
#include "options_parser.h"

using std::cout, std::cerr, std::endl;
using boost::asio::ip::tcp;

LoggingController::LoggingController(int listening_port, const LoggingService& logging_service)
                                    : svs(listening_port),
                                      server(new RequestHandlerFactory(logging_service), svs, new HTTPServerParams) {}

void LoggingController::accept_connections(int listening_port) {
    cout << "--- Logging microservice #" << listening_port << " is on!" << endl;
    server.start();
}

LoggingController::~LoggingController() {
    server.stop();
    cout << "--- Logging microservice is off!" << endl;
}

LoggingController::RequestHandler::RequestHandler(const LoggingService &logging_service) : logging_service(logging_service) {}

void LoggingController::RequestHandler::handleRequest(HTTPServerRequest &request, HTTPServerResponse &response) {
    static int requests_counter = 0;

    std::stringstream ss;
    requests_counter++;
    ss << "[" << requests_counter << "] ";
    std::string counter_str = ss.str();

    if (request.getMethod() == HTTPServerRequest::HTTP_GET) {
        cout << counter_str << "got GET request -> sending response to client" << endl;
        std::vector<Message> messages_list = logging_service.retrieve_messages();
        if (!messages_list.empty()) {
            std::string messages_str = messages_list_to_str(messages_list);
            const char *buffer = messages_str.c_str();
            response.sendBuffer(buffer, messages_str.length());
        } else {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
            response.send();
        }
    } else if (request.getMethod() == HTTPServerRequest::HTTP_POST) {
        cout << counter_str << "got POST request -> saving client message" << endl;
        std::istream &request_body_stream = request.stream();
        std::stringstream request_body;
        request_body << request_body_stream.rdbuf();
        std::string request_body_str = request_body.str();

        size_t delimiter_idx = request_body_str.find('=');
        std::string uuid = request_body_str.substr(0, delimiter_idx);
        std::string message_str = request_body_str.substr(delimiter_idx + 1);
        Message message(uuid, message_str);

        logging_service.add_message(message);
    } else {
        cout << counter_str << "unknown request" << endl;
    }
}

std::string LoggingController::RequestHandler::messages_list_to_str(const std::vector<Message> &messages) {
    std::stringstream messages_ss;
    for (const auto &message: messages) {
        messages_ss << message.str() << endl;
    }
    std::string messages_str = messages_ss.str();
    messages_str.pop_back();
    return messages_str;
}

LoggingController::RequestHandlerFactory::RequestHandlerFactory(const LoggingService &logging_service) : logging_service(logging_service) {}

HTTPRequestHandler *LoggingController::RequestHandlerFactory::createRequestHandler(const HTTPServerRequest &request) {
    return new RequestHandler(logging_service);
}

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

int Server::main(const std::vector<std::string> &args) {
    std::string filename;
    if (args.empty()) {
        filename = "../config.cfg";
    } else if (args.size() == 1) {
        filename = args.at(0);
    } else {
        cerr << "Wrong number of arguments" << endl;
        exit(EXIT_FAILURE);
    }

    config_options_t opt{filename};

    HazelcastPersistence persistence("user_messages");
    LoggingService logging_service(persistence);
    int port = 0;
    for (int test_port = opt.logging_microservice_port; test_port < opt.logging_microservice_port + opt.logging_microservices_count; ++test_port) {
        if (!is_port_busy(test_port)) {
            port = test_port;
            break;
        }
    }
    if (!port) {
        cout << "[!] Failed to launch logging microservice" << endl;
        exit(EXIT_FAILURE);
    }
    LoggingController logging_controller(port, logging_service);
    logging_controller.accept_connections(port);

    waitForTerminationRequest();

    return 0;
}

