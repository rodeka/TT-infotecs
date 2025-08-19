#include "logging/logger.hpp"
#include <utility>

namespace logging
{

    Logger::~Logger()
    {
        if (sink_)
            sink_->close();
    }

    Status Logger::init_with_sink(std::unique_ptr<ISink> s, Severity lvl)
    {
        if (!s)
            return {false, 1, "sink is null"}; // страж?

        // попытка открыть файл
        auto st = s->open();
        if (!st.ok)
        {
            last_error_ = st.message;
            return st;
        }

        // allgood - устанавливаем состояние логгера
        std::lock_guard<std::mutex> lk(mtx_);
        sink_ = std::move(s);
        default_level_ = lvl;
        return {true, 0, {}};
    }

    Status Logger::init_file(const std::string &filename, Severity default_level)
    {
        return init_with_sink(make_file_sink(filename), default_level);
    }

    Status Logger::init_udp(const std::string &host, uint16_t port, Severity default_level)
    {
        return init_with_sink(make_udp_sink(host, port), default_level);
    }

    void Logger::set_level(Severity lvl)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        default_level_ = lvl;
    }

    Severity Logger::level() const
    {
        std::lock_guard<std::mutex> lk(mtx_);
        return default_level_;
    }

    Status Logger::log(Severity lvl, std::string_view message)
    {
        std::lock_guard<std::mutex> lk(mtx_); // блокируем поток (мне не нравится, что на весь метод, но делать нечего)

        if (!sink_)
            return {false, 2, "logger not init"}; // страж на инициализацию sink

        // фильтр по уровню
        if (static_cast<int>(lvl) < static_cast<int>(default_level_))
        {
            return {true, 0, {}};
        }

        // формирование записи
        LogRecord rec;
        rec.level = lvl;
        rec.iso_time = now_iso_local();
        rec.text = std::string(message);

        // trying write
        auto st = sink_->write(rec);
        if (!st.ok)
            last_error_ = sink_->last_error();
        return st;
    }

    Status Logger::log(std::string_view message)
    {
        return log(default_level_, message);
    }

    std::string Logger::last_error() const
    {
        std::lock_guard<std::mutex> lk(mtx_);
        return last_error_;
    }

}