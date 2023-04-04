#ifndef _LOGGING_CONTROLLER_H_
#define _LOGGING_CONTROLLER_H_

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Util/ServerApplication.h>
#include "logging_service.h"

using namespace Poco::Net;
using namespace Poco::Util;

class Controller {
public:
    virtual void accept_connections() = 0;
};

class LoggingController : public Controller {
public:
    LoggingController(int listening_port, const LoggingService& logging_service);

    void accept_connections() override;

    ~LoggingController();

private:
    class RequestHandler : public HTTPRequestHandler {
    public:
        explicit RequestHandler(const LoggingService& logging_service);
        void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override;
        static std::string messages_list_to_str(const std::vector<Message>& messages);
    private:
        LoggingService logging_service;
    };

    class RequestHandlerFactory : public HTTPRequestHandlerFactory {
    public:
        explicit RequestHandlerFactory(const LoggingService& logging_service);
        HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request) override;
    private:
        LoggingService logging_service;
    };

    ServerSocket svs;
    HTTPServer server;
};

class Server : public ServerApplication {
protected:
    int main(const std::vector<std::string> &args) override;
};

#endif