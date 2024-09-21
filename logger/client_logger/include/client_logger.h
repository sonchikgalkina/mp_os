#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H

#include "../../logger/include/logger.h"
#include "client_logger_builder.h"

class client_logger final:
    public logger
{
    
    friend class client_logger_builder;

    std::string _format;

    std::map<std::string, std::set<logger::severity>> _streams;
    
    static std::map<std::string, std::pair<std::ofstream, int>> _streams_users;

    client_logger(std::map<std::string, std::set<logger::severity>> streams, std::string format);

    void close_streams();

public:

    client_logger(client_logger const &other);

    client_logger &operator=(client_logger const &other);

    client_logger(client_logger &&other) noexcept;

    client_logger &operator=(client_logger &&other) noexcept;

    ~client_logger() noexcept final;

public:

    [[nodiscard]] logger const *log(const std::string &message, logger::severity severity) const noexcept override;

};

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_CLIENT_LOGGER_H
