#include "logging_service.h"

LoggingService::LoggingService(const LoggingPersistence &logging_persistence) : logging_persistence(const_cast<LoggingPersistence &>(logging_persistence)) {}

void LoggingService::add_message(const Message &msg) {
    logging_persistence.save_message(msg);
}

std::vector<Message> LoggingService::retrieve_messages() {
    return logging_persistence.get_messages();
}
