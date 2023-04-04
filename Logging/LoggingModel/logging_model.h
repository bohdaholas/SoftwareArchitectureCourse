#ifndef _LOGGING_MODEL_H
#define _LOGGING_MODEL_H

#include <string>

class Message {
public:
    explicit Message(const std::string& msg);
    Message(const std::string& uuid, const std::string& msg);

    std::string generate_uuid(const std::string& string);

    [[nodiscard]] std::string str() const;
    [[nodiscard]] const std::string &get_message() const;
    [[nodiscard]] const std::string &get_uuid() const;
private:
    std::string message;
    std::string uuid;
};

#endif //_LOGGING_MODEL_H
