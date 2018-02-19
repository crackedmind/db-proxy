#include <iostream>
#include <iomanip>
#include "parser.hpp"
#include "debug.hpp"
#include "logger.hpp"

namespace My {
Parser::~Parser(){
    prepared_stmts.clear();
}

bool Parser::parse(const uint8_t *data, size_t size) {
    auto logger = LoggerRegistry::instance().get("logger");

    // some magick to skip auth phase
    if(packets_ < 3)
    {
        packets_++;
        return true;
    }

    // command phase
    size_t offset = 0;

    PacketHeader header = read_header(data, size);
    offset += 4;

    if(header.sequence_id == 0) {
        current_state_ = State::PARSE_QUERY;
    }

    switch(current_state_) {
    case State::PARSE_QUERY:
    {
        auto type = data[offset++];
        std::string s;

        switch(type) {
        case COM_QUERY:
        {
            s.assign(reinterpret_cast<char*>(const_cast<uint8_t*>(&data[offset])), header.payload_length - 1);
            logger->log() << "Execute query: " << s << '\n';
            current_state_ = State::PARSE_QUERY_RESPONSE;
        }
            break;
        case COM_STMT_PREPARE:
            last_stmt_.assign(reinterpret_cast<char*>(const_cast<uint8_t*>(&data[offset])), header.payload_length - 1);
            logger->log() << "Prepare statement: " << last_stmt_ << '\n';
            current_state_ = State::PARSE_STMT_RESPONSE;
            break;
        case COM_STMT_SEND_LONG_DATA:
            logger->log() << "COM_STMT_SEND_LONG_DATA\n";
            current_state_ = State::PARSE_QUERY_RESPONSE;
            break;
        case COM_STMT_EXECUTE:
        {
            uint32_t stmt_id = read_u4(data+offset);
            auto stmt = prepared_stmts[stmt_id];
            offset += 5;

            logger->log() << "Execute prepared statement: " << stmt.first;

            if(stmt.second > 0) {
                uint32_t bitmap = (stmt.second + 7) / 8;
                offset += bitmap;
                uint32_t bound_flag = data[offset];
                if (bound_flag == 1) {

                }
            }

            current_state_ = State::PARSE_STMT_EXECUTE_RESPONSE;
            logger->log() << '\n';
        }
            break;
        case COM_STMT_CLOSE:
        {
            uint32_t stmt_id = read_u4(data+offset);
            auto stmt = prepared_stmts[stmt_id];
            logger->log() << "Deallocate prepared statement: " << stmt.first << '\n';
            prepared_stmts.erase(stmt_id);
        }
            break;
        }
    }
        break;
    case State::PARSE_QUERY_RESPONSE:
        break;
    case State::PARSE_STMT_RESPONSE:
    {

        int status = data[offset++];
        if (status == 0) // OK
        {
            uint32_t stmt_id = read_u4(data+offset);
            offset += 4;

            uint32_t num_params = read_u2(data+offset+2);

            prepared_stmts.emplace(stmt_id, std::make_pair(last_stmt_, num_params));
        }
    }
        break;
    case State::PARSE_STMT_EXECUTE_RESPONSE:
        break;
    }

    return true;
}

PacketHeader Parser::read_header(const uint8_t *data, size_t size)
{
    PacketHeader header;
    header.payload_length = read_u3(data);
    header.sequence_id = data[3];
    return header;
}

}
