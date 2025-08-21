#include "logging/logger.hpp"
#include <cassert>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using logging::Logger;
using logging::Severity;

struct UdpCapture {
    int sock{ -1 };
    uint16_t port{ 0 };
    std::string captured;
    std::mutex m;
    std::condition_variable cv;
    bool got{ false };

    bool start() {
        sock = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return false;
        sockaddr_in addr{}; addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
        addr.sin_port = htons(0); // эпемерный порт
        if (::bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) return false;
        sockaddr_in real{}; socklen_t sl = sizeof(real);
        if (::getsockname(sock, (sockaddr*)&real, &sl) < 0) return false;
        port = ntohs(real.sin_port);
        return true;
    }
    void stop() { if (sock >= 0) { ::close(sock); sock = -1; } }

    void run_once() {
        char buf[2048];
        sockaddr_in src{}; socklen_t sl = sizeof(src);
        fd_set rfds; FD_ZERO(&rfds); FD_SET(sock, &rfds);
        timeval tv{}; tv.tv_sec = 2; tv.tv_usec = 0; // таймаут 2с
        int ret = ::select(sock + 1, &rfds, nullptr, nullptr, &tv);
        if (ret > 0) {
            ssize_t n = ::recvfrom(sock, buf, sizeof(buf) - 1, 0, (sockaddr*)&src, &sl);
            if (n > 0) {
                buf[n] = '\0';
                std::unique_lock<std::mutex> lk(m);
                captured = buf;
                got = true;
                cv.notify_all();
            }
        }
    }
};

int main() {
    UdpCapture cap;
    assert(cap.start());

    // приёмник в отдельном потоке (одна датаграмма)
    std::thread rx([&] { cap.run_once(); });

    // логгер шлёт в этот порт
    Logger L;
    auto st = L.init_udp("127.0.0.1", cap.port, Severity::Info);
    assert(st.ok);

    const std::string msg = "Hello UDP test";
    st = L.log(Severity::Info, msg);
    assert(st.ok);

    // ждём приёма
    {
        std::unique_lock<std::mutex> lk(cap.m);
        cap.cv.wait_for(lk, std::chrono::seconds(3), [&] { return cap.got; });
    }

    rx.join();
    cap.stop();

    // проверяем что строка пришла и содержит уровень + текст
    assert(cap.got);
    assert(cap.captured.find("[INFO]") != std::string::npos);
    assert(cap.captured.find(msg) != std::string::npos);

    return 0;
}
