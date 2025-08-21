#include "logging/logger.hpp"
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <sstream>

using logging::Logger;
using logging::Severity;
using logging::Status;

struct Msg {
    Severity sev;
    std::string text;
};

// парсинг уровня из текста
static Severity parse_sev(std::string s, bool* has) {
    for (auto& c : s)
        c = static_cast<char>(::tolower(c));
    if (s == "info") {
        if (has)
            *has = true;
        return Severity::Info;
    }
    if (s == "warn" || s == "warning") {
        if (has)
            *has = true;
        return Severity::Warn;
    }
    if (s == "error" || s == "err") {
        if (has)
            *has = true;
        return Severity::Error;
    }
    if (has)
        *has = false;
    return Severity::Info;
}

int main(int argc, char** argv) {
    // 2 аргумента - файл и уровень по дефолту
    if (argc < 3) {
        std::cerr << "Usage: log_app <logfile> <default-level: info|warn|error> [--udp host:port]\n";
        return 1;
    }

    std::string logfile = argv[1];

    // берем уровень по умолчанию
    bool has = false;
    Severity def = parse_sev(argv[2], &has);
    if (!has) {
        std::cerr << "Bad default level\n";
        return 2;
    }

    Logger logger;
    Status st;

    // опционально режим udp
    if (argc >= 5 && std::string(argv[3]) == "--udp") {
        std::string arg = argv[4];
        size_t pos = arg.find(':');
        if (pos == std::string::npos) {
            std::cerr << "Bad host:port\n";
            return 3;
        }
        std::string host = arg.substr(0, pos);
        uint16_t port = static_cast<uint16_t>(std::stoi(arg.substr(pos + 1)));
        st = logger.init_udp(host, port, def);
    }
    else {
        st = logger.init_file(logfile, def);
    }

    if (!st.ok) {
        std::cerr << "Init failed: " << st.message << "\n";
        return 4;
    }

    // очередь логов + синхрон
    std::queue<Msg> q;
    std::mutex m;
    std::condition_variable cv;
    std::atomic<bool> done{ false };

    // рабочий поток
    std::thread worker([&]() {
        while (true) {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [&] {return !q.empty() || done.load(); });
            if (q.empty() && done.load()) break;
            Msg msg = std::move(q.front());
            q.pop();
            lk.unlock();
            logger.log(msg.sev, msg.text);
        }
    });

    // основной поток
    std::string line;
    std::cout << "Enter messages (prefix with level: info|warn|error). Type 'quit' to exit.\n";
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;

        // проверяем первый токен (может быть уровень)
        std::istringstream iss(line);
        std::string maybe_level;
        if (!(iss >> maybe_level)) continue;

        bool had = false;
        Severity sv = parse_sev(maybe_level, &had);
        std::string text;

        if (had) {
            // уровень распознан, берем отсальную строку как текст
            std::getline(iss, text);
        }
        else {
            sv = logger.level();
            text = line;
        }

        // убираем возможный ведущий пробел после getline
        if (!text.empty() && text.front() == ' ') text.erase(0, 1);

        // помещаем в очередь
        {
            std::lock_guard<std::mutex> lk(m);
            q.push(Msg{ sv, text });
        }

        cv.notify_one();
    }

    // сигнал воркеру
    done.store(true);
    cv.notify_all();
    worker.join();
    return 0;
}
