#include "logging/sinks.hpp"
#include "logging/timefmt.hpp"
#include <string>
#include <mutex>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

namespace logging
{

    class UdpSink final : public ISink
    {
    public:
        UdpSink(std::string host, uint16_t port) : host_(std::move(host)), port_(port) {}

        Status open() override
        {
            std::lock_guard<std::mutex> lk(m_);

            // создаем сокет UDP ipv4
            sock_ = ::socket(AF_INET, SOCK_DGRAM, 0);
            if (sock_ < 0)
            {
                last_error_ = "socket() failed";
                return {false, 20, last_error_};
            }

            // host:port -> sockaddr через getaddrinfo
            struct addrinfo hints{};
            hints.ai_family = AF_INET;      // ipv4
            hints.ai_socktype = SOCK_DGRAM; // udp
            struct addrinfo *res = nullptr;

            char portstr[16];
            std::snprintf(portstr, sizeof(portstr), "%u", (unsigned)port_);

            if (::getaddrinfo(host_.c_str(), portstr, &hints, &res) != 0 || !res)
            {
                last_error_ = "getaddrinfo() failed";
                ::close(sock_);
                sock_ = -1;
                return {false, 21, last_error_};
            }

            // копируем результат
            std::memcpy(&addr_, res->ai_addr, res->ai_addrlen);
            addrlen_ = res->ai_addrlen;
            ::freeaddrinfo(res);
            return {true, 0, {}};
        }

        void close() override
        {
            std::lock_guard<std::mutex> lk(m_);
            if (sock_ >= 0)
            {
                ::close(sock_);
                sock_ = -1;
            }
        }

        Status write(const LogRecord &rec) override
        {
            std::lock_guard<std::mutex> lk(m_);

            // страж на открытие сокета
            if (sock_ < 0)
            {
                last_error_ = "socket not open";
                return {false, 22, last_error_};
            }

            // строка совместимая с sink
            std::string line = rec.iso_time;
            line += " [";
            line += to_str_severity(static_cast<int>(rec.level));
            line += "] ";
            line += rec.text;

            // отправка
            ssize_t n = ::sendto(sock_, line.data(), line.size(), 0, (struct sockaddr *)&addr_, addrlen_);
            if (n < 0)
            {
                last_error_ = "sendto() failed";
                return {false, 23, last_error_};
            }

            return {true, 0, {}};
        }
        std::string last_error() const override { return last_error_; }

    private:
        std::string host_;
        uint16_t port_{0};
        int sock_{-1}; // файловый дескриптор udp сокета
        struct sockaddr_storage addr_{};
        socklen_t addrlen_{0};
        mutable std::mutex m_;
        std::string last_error_; // последняя ошибка
    };

    std::unique_ptr<ISink> make_udp_sink(const std::string &host, uint16_t port)
    {
        return std::unique_ptr<ISink>(new UdpSink(host, port));
    }

} // namespace logging