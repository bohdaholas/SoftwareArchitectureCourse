#include <iostream>
#include <unordered_map>
#include <boost/asio.hpp>
#include <ppconsul/agent.h>
#include "logging_controller.h"
#include "logging_service.h"
#include "options_parser.h"

using std::cout, std::cerr, std::endl;
using boost::asio::ip::tcp;
using ppconsul::Consul;
using namespace ppconsul::agent;

static Consul consul;
static Agent agent(consul);
static const std::string MICROSERVICE_NAME = "logging_microservice";
static int logging_microservice_port;

enum class RequestType {
    HealthCheck,
    MicroserviceCommunication
};

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

static RequestType parse_request_type(HTTPServerRequest &request) {
    if (request.getURI() == "/health") {
        return RequestType::HealthCheck;
    }
    return RequestType::MicroserviceCommunication;
}

static std::pair<std::string, std::string> parse_request_body(HTTPServerRequest &request) {
    std::istream &request_body_stream = request.stream();
    std::stringstream request_body;
    request_body << request_body_stream.rdbuf();
    std::string request_body_str = request_body.str();

    size_t delimiter_idx = request_body_str.find('=');
    std::string uuid = request_body_str.substr(0, delimiter_idx);
    std::string message_str = request_body_str.substr(delimiter_idx + 1);
    return {uuid, message_str};
}

LoggingController::LoggingController(int listening_port, const LoggingService& logging_service)
                                    : svs(listening_port),
                                      server(new RequestHandlerFactory(logging_service), svs, new HTTPServerParams) {}

void LoggingController::accept_connections(int listening_port) {
    cout << "--- Logging microservice #" << listening_port << " is on!" << endl;
    server.start();
}

LoggingController::~LoggingController() {
    server.stop();
    agent.deregisterService(MICROSERVICE_NAME + "_" + std::to_string(logging_microservice_port));
    cout << "--- Logging microservice is off!" << endl;
}

LoggingController::RequestHandler::RequestHandler(const LoggingService &logging_service) : logging_service(logging_service) {}

void LoggingController::RequestHandler::handleRequest(HTTPServerRequest &request, HTTPServerResponse &response) {
    switch (parse_request_type(request)) {
        case RequestType::HealthCheck:
            handle_healthcheck_request(response);
            break;
        case RequestType::MicroserviceCommunication:
            handle_microservice_request(request, response);
            break;
    }
}

void LoggingController::RequestHandler::handle_microservice_request(HTTPServerRequest &request,
                                                                    HTTPServerResponse &response) {
    static int requests_counter = 0;

    std::stringstream ss;
    requests_counter++;
    ss << "[" << requests_counter << "] ";
    std::string counter_str = ss.str();

    if (request.getMethod() == HTTPServerRequest::HTTP_GET) {
        cout << counter_str << "got GET request -> sending response to client" << endl;
        std::vector<Message> messages_list = logging_service.retrieve_messages();
        if (!messages_list.empty()) {
            response.setStatus(HTTPResponse::HTTP_OK);
            std::string messages_str = messages_list_to_str(messages_list);
            auto &response_body_ostr = response.send();
            response_body_ostr << messages_str;
        } else {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
            response.send();
        }
    } else if (request.getMethod() == HTTPServerRequest::HTTP_POST) {
        cout << counter_str << "got POST request -> saving client message" << endl;
        auto [uuid, message_str] = parse_request_body(request);
        Message message(uuid, message_str);
        logging_service.add_message(message);
    } else {
        cout << counter_str << "unknown request" << endl;
    }
}

void LoggingController::RequestHandler::handle_healthcheck_request(HTTPServerResponse &response) {
    response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
    auto &response_body_ostr = response.send();
    response_body_ostr << MICROSERVICE_NAME << " #" << logging_microservice_port;
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

    logging_microservice_port = get_first_usable_port(opt.start_port);
    agent.registerService(
            kw::name = MICROSERVICE_NAME,
            kw::port = logging_microservice_port,
            kw::check = HttpCheck{"http://localhost:" + std::to_string(logging_microservice_port) + "/health", std::chrono::seconds(5)},
            kw::id = MICROSERVICE_NAME + "_" + std::to_string(logging_microservice_port)
    );

    HazelcastPersistence persistence("user_messages");
    LoggingService logging_service(persistence);
    LoggingController logging_controller(logging_microservice_port, logging_service);
    logging_controller.accept_connections(logging_microservice_port);

    waitForTerminationRequest();

    return 0;
}

