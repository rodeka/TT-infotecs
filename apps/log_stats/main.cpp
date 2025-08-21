#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

struct Stats {
    // счетчики количества сообщений
    uint64_t total{ 0 };
    uint64_t by_level[3]{ 0,0,0 }; // индексы: 0=INFO, 1=WARN, 2=ERROR

    // очередь временных меток приёма последних сообщений
    // (нужна, чтобы быстро сдвигать окно последнего часа)
    std::deque<std::chrono::steady_clock::time_point> times;

    // статистика длин (в символах) полученных строк.
    size_t min_len{ SIZE_MAX };
    size_t max_len{ 0 };
    uint64_t sum_len{ 0 };

    // флаг поменялось ли что-то с момента последнего вывода.
    bool changed{ false };
};

// вывод статистики в консоль
static void print_stats(const Stats& s) {
    using std::cout;
    cout << "=== stats ===\n";
    cout << "total: " << s.total << "\n";
    cout << "by level: info=" << s.by_level[0] << " warn=" << s.by_level[1] << " error=" << s.by_level[2] << "\n";
    cout << "last hour: " << s.times.size() << "\n";

    if (s.total == 0) {
        cout << "lengths: min=0 max=0 avg=0\n";
    }
    else {
        double avg = s.total ? (double)s.sum_len / (double)s.total : 0.0;
        cout << "lengths: min=" << (s.min_len == SIZE_MAX ? 0 : s.min_len)
            << " max=" << s.max_len
            << " avg=" << (int)avg << "\n";
    }
}

// пытаемся извлечь уровень из строки формата ... [LEVEL] ...
static int level_index_from_line(const std::string& line) {
    auto l = line.find('[');
    auto r = line.find(']');
    if (l != std::string::npos && r != std::string::npos && r > l + 1) {
        auto L = line.substr(l + 1, r - l - 1);
        for (auto& c : L) c = (char)tolower(c);
        if (L == "info")  return 0;
        if (L == "warn")  return 1;
        if (L == "error") return 2;
    }
    return 0; // по умолчанию считаем info
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: log_stats <listen_port> <N> <T_seconds>\n";
        return 1;
    }

    uint16_t port = (uint16_t)std::stoi(argv[1]);
    int everyN = std::stoi(argv[2]);
    int timeoutSec = std::stoi(argv[3]);
    if (everyN <= 0) everyN = 1;
    if (timeoutSec <= 0) timeoutSec = 5;

    // создаём udp сокет и биндимся на 0.0.0.0:port
    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { // страж на сокет
        std::cerr << "socket failed\n"; return 2;
    }

    sockaddr_in addr{}; addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (::bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) { std::cerr << "bind failed\n"; return 3; }

    std::cout << "Listening UDP port " << port << " ...\n";

    Stats s;
    auto last_print = std::chrono::steady_clock::now();

    while (true) {
        // используем select с таймаутом в 1 секунду чтобы
        // 1. периодически проверять, не пора ли печатать статистику по T
        // 2. не блокироваться бесконечно на recvfrom
        fd_set rfds; FD_ZERO(&rfds); FD_SET(sock, &rfds);
        timeval tv{}; tv.tv_sec = 1; tv.tv_usec = 0;
        int ret = ::select(sock + 1, &rfds, nullptr, nullptr, &tv);
        auto now = std::chrono::steady_clock::now();

        // проверяем условие печати по таймауту T, но только если статистика менялась
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_print).count();
        if (elapsed >= timeoutSec && s.changed) {
            print_stats(s);
            s.changed = false;
            last_print = now;
        }

        if (ret <= 0) continue; // таймаут select возвращаемся к началу цикла

        // принимаем один udp пакет. 4 КБ буфер
        char buf[4096];
        sockaddr_in src{}; socklen_t sl = sizeof(src);
        ssize_t n = ::recvfrom(sock, buf, sizeof(buf) - 1, 0, (sockaddr*)&src, &sl);
        if (n <= 0) continue;
        buf[n] = '\0';

        std::string line(buf);
        std::cout << line << "\n";

        s.total++;

        // уровень (по формату, который генерирует наш logger/sink)
        int idx = level_index_from_line(line);
        if (idx < 0 || idx > 2) idx = 0;
        s.by_level[idx]++;

        // длины сообщений.
        size_t len = line.size();
        if (len < s.min_len) s.min_len = len;
        if (len > s.max_len) s.max_len = len;
        s.sum_len += len;

        // скользящее окно последний час
        s.times.push_back(now);
        auto hour = std::chrono::hours(1);
        while (!s.times.empty() && now - s.times.front() > hour) s.times.pop_front();

        s.changed = true;

        // печать каждые N сообщений (N>0).
        if (everyN > 0 && (s.total % everyN) == 0) {
            print_stats(s);
            s.changed = false;
            last_print = now;
        }
    }
    return 0;
}
