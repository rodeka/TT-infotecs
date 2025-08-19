#pragma once
#include <string>
#include <memory>
#include <cstdint>

namespace logging
{

    // Унифицированный код возврата
    struct Status
    {
        bool ok{true};
        int code{0};
        std::string message{};
    };

    // Уровень важности логов
    enum class Severity : int
    {
        Info = 0,
        Warn = 1,
        Error = 2
    };

    // Нормализованная запись лога
    struct LogRecord
    {
        std::string text;
        Severity level;
        std::string iso_time;
    };

    // Абстрактный интерфейк записи лога
    class ISink
    {
    public:
        virtual ~ISink() = default;

        // метод подготовки лога
        virtual Status open() = 0;

        // освобождение
        virtual void close() = 0;

        // запись лога
        virtual Status write(const LogRecord &rec) = 0;

        // последняя ошибка в sink (optional)
        virtual std::string last_error() const = 0;
    };

    // Фабричные функии создания ISink
    std::unique_ptr<ISink> make_file_sink(const std::string &filename);
    std::unique_ptr<ISink> make_udp_sink(const std::string &host, uint16_t port);

} // namespace logging