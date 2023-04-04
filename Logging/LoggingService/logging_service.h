#ifndef _LOGGING_SERVICE_H_
#define _LOGGING_SERVICE_H_

#include <vector>
#include "logging_model.h"
#include "logging_persistence.h"

class LoggingService {
public:
    explicit LoggingService(const LoggingPersistence& logging_persistence);

    void add_message(const Message& msg);
    std::vector<Message> retrieve_messages();

private:
    LoggingPersistence &logging_persistence;
};

#endif