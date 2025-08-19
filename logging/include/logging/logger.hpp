#pragma once
#include "sinks.hpp"
#include "timefmt.hpp"
#include <mutex>
#include <memory>
#include <string>
#include <string_view>

namespace logging
{

    class Logger
    {
    public:
        Logger() = default;
        ~Logger(); // стандартный деконструктор

        // Инициализацции файла-журнала
        // - filename - путо до файла
        // - default_level - уровень записи
        Status init_file(const std::string &filename, Severity default_level);

        // Инициализация UDP
        // - host, port - адрес отправки
        // - default_level - уровень записи
        Status init_udp(const std::string &host, uint16_t port, Severity);

        // Установка и чтение уровня записи
        void set_level(Severity lvl);
        Severity level() const;

        // Метод записи лога с указанным уровнем
        Status log(Severity lvl, std::string_view message);

        // Метод записи с уровнем по умолчанию
        Status log(std::string_view message);

        // Последняя ошибка Logger
        std::string last_error() const;

    private:
        // Метод иниализации Sink
        Status init_with_sink(std::unique_ptr<ISink> s, Severity lvl);

        Severity default_level_{Severity::Info}; // Текцщий уровень по умолчанию
        std::unique_ptr<ISink> sink_;
        mutable std::mutex mtx_;
        std::string last_error_; // Буфер последний ошибки Logger
    };

} // namespace logging
