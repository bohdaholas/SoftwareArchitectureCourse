#include <iostream>
#include <unordered_map>
#include "server.h"

using std::cout, std::cerr, std::endl;

static std::unordered_map<std::string, std::string> messages;

void RequestHandler::handleRequest(HTTPServerRequest &request, HTTPServerResponse &response) {
    static int requests_counter = 0;

    std::stringstream ss;
    requests_counter++;
    ss << "[" << requests_counter << "] ";
    std::string counter_str = ss.str();

    if (request.getMethod() == HTTPServerRequest::HTTP_GET) {
        cout << counter_str << "got GET request -> sending response to client" << endl;
        if (!messages.empty()) {
            std::stringstream response_ss;
            for (const auto &[uuid, message]: messages) {
                response_ss << uuid << "=" << message << endl;
            }
            std::string response_str = response_ss.str();
            response_str.pop_back();
            const char* buffer = response_str.c_str();
            size_t length = response_str.length();

            response.sendBuffer(buffer, length);
        } else {
            response.setStatus(Poco::Net::HTTPResponse::HTTP_NO_CONTENT);
            response.send();
        }
    } else if (request.getMethod() == HTTPServerRequest::HTTP_POST) {
        cout << counter_str << "got POST request -> saving client message" << endl;
        std::istream& request_body_stream = request.stream();
        std::stringstream request_body;
        request_body << request_body_stream.rdbuf();
        std::string request_body_str = request_body.str();

        size_t delimiter_idx = request_body_str.find('=');
        std::string uuid = request_body_str.substr(0, delimiter_idx);
        std::string message = request_body_str.substr(delimiter_idx + 1);
        messages[uuid] = message;
    } else {
        cout << counter_str << "unknown request" << endl;
    }
}

HTTPRequestHandler *RequestHandlerFactory::createRequestHandler(const HTTPServerRequest &request) {
    return new RequestHandler;
}

int RequestHandler::facade_service_port = 0;

int Server::main(const std::vector<std::string> &args) {
    std::string filename;
    if (args.empty()) {
        filename = "config.cfg";
    } else if (args.size() == 1) {
        filename = args.at(0);
    } else {
        cerr << "Wrong number of arguments" << endl;
        exit(EXIT_FAILURE);
    }

    config_options_t opt{filename};
    RequestHandler::facade_service_port = opt.facade_service_port;

    ServerSocket svs(opt.logging_service_port);
    HTTPServer server(new RequestHandlerFactory, svs, new HTTPServerParams);
    server.start();
    cout << "--- LoggingService is on!" << endl;
    waitForTerminationRequest();
    server.stop();
    cout << "--- LoggingService is off!" << endl;
    return 0;
}
