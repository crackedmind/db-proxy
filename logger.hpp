#pragma once

#include <iostream>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <fstream>

class Logger{
    std::ostream &stream_;
    std::mutex m_;
public:
    Logger() = delete;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    Logger(std::ostream& stream) : stream_(stream) {
    }
    Logger(std::ostream&& stream) : stream_(stream) {
    }

    virtual ~Logger(){
    }
    std::ostream& raw_stream() const { return stream_; }

    Logger& log() { return *this; }

    template<typename T>
    Logger& operator<<(const T &arg) {
        std::lock_guard<std::mutex> l(m_);
        stream_ << arg;
        return *this;
    }

    // support for endl, etc...
    Logger& operator<<(std::ostream& (*os)(std::ostream&)) {
        stream_ << os;
        return *this;
    }
};
class NullStream : public std::ostream {
public:
    NullStream() : std::ostream( nullptr) {}
};

class NullLogger : public Logger {
    NullStream null;
public:
    NullLogger(): Logger(null) {}
};

class FileLogger : public Logger {
    std::ofstream stream_;
    std::string filename_;
public:
    FileLogger(const std::string& filename) : Logger (stream_), filename_(filename) {
        stream_.open(filename);
    }
    ~FileLogger() override {
    }
};

using LoggerPtr = std::shared_ptr<Logger>;

class LoggerRegistry {
    std::unordered_map<std::string, LoggerPtr> loggers_;
    std::mutex mutex_;
public:

    LoggerPtr get(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto logger = loggers_.find(name);
        return logger == loggers_.end() ? nullptr : logger->second;
    }

    void register_logger(const std::string& logger_name, LoggerPtr logger) {
        std::lock_guard<std::mutex> lock(mutex_);

        loggers_[logger_name] = logger;
    }

    LoggerPtr create_stdout(const std::string& logger_name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto logger = std::make_shared<Logger>(std::cout);
        loggers_[logger_name] = logger;
        return logger;
    }

    LoggerPtr create_null(const std::string& logger_name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto logger = std::make_shared<NullLogger>();
        loggers_[logger_name] = logger;
        return logger;
    }

    LoggerPtr create_file(const std::string& logger_name, const std::string &filename) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto logger = std::make_shared<FileLogger>(filename);
        loggers_[logger_name] = logger;
        return logger;
    }

    static LoggerRegistry& instance() {
        static LoggerRegistry instance;
        return instance;
    }
};
