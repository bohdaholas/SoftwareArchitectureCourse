#ifndef _SERVER_H_
#define _SERVER_H_

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Util/ServerApplication.h>
#include "options_parser.h"

using namespace Poco::Net;
using namespace Poco::Util;

class RequestHandler : public HTTPRequestHandler {
public:
    void handleRequest(HTTPServerRequest &request, HTTPServerResponse &response);
    static int facade_service_port;
};

class RequestHandlerFactory : public HTTPRequestHandlerFactory {
public:
    HTTPRequestHandler *createRequestHandler(const HTTPServerRequest &request);
};

class Server : public ServerApplication {
protected:
    int main(const std::vector<std::string> &args);
};

#endif