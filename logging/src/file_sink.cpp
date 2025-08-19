#include "logging/sinks.hpp"
#include "logging/timefmt.hpp"
#include <fstream>
#include <mutex>

namespace logging
{

    // Реализация Sink по файлу
    class FileSink final : public ISink
    {
    public:
        explicit FileSink(std::string filename) : filename_(std::move(filename)) {}

        Status open() override
        {
            std::lock_guard<std::mutex> lk(m_);

            // запись в файл (если он есть)
            out_.open(filename_, std::ios::app | std::ios::out);
            if (!out_.is_open() || !out_.good())
            {
                last_error_ = "failed to open file: " + filename_;
                return {false, 10, last_error_};
            }
            return {true, 0, {}};
        }

        void close() override
        {
            std::lock_guard<std::mutex> lk(m_);
            if (out_.is_open())
                out_.close();
        }

        Status write(const LogRecord &rec) override
        {
            std::lock_guard<std::mutex> lm(m_);

            // страж на открытие файла
            if (!out_.is_open())
            {
                last_error_ = "file not open";
                return {false, 11, last_error_};
            }

            // формат строки <ISO-time> [LEVEL] <message>
            out_ << rec.iso_time << " [" << to_str_severity(static_cast<int>(rec.level)) << "] " << rec.text << '\n';

            // проверка состояния потока после записи
            if (!out_.good())
            {
                last_error_ = "write failed";
                return {false, 12, last_error_};
            }

            out_.flush();
            return {true, 0, {}};
        }
        std::string last_error() const override { return last_error_; }

    private:
        std::string filename_; // путь к файлу
        std::ofstream out_;    // файловый поток
        mutable std::mutex m_;
        std::string last_error_; // последняя ошибка
    };

    std::unique_ptr<ISink> make_file_sink(const std::string &filename)
    {
        return std::unique_ptr<ISink>(new FileSink(filename));
    }

} // namespace logging