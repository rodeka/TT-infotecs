#include "logging/logger.hpp"
#include <cassert>
#include <string>

using logging::Logger;
using logging::Severity;

int main() {
    // вызов log без инициализации
    {
        Logger L;
        auto st = L.log(Severity::Info, "no init");
        assert(!st.ok);
    }

    // неверный host для UDP - getaddrinfo должен упасть
    {
        Logger L;
        auto st = L.init_udp("this_host_should_not_exist_xyz", 65000, Severity::Info);
        assert(!st.ok);
    }

    // невалидный файл - попытка открыть директорию как файл должна провалиться
    {
        Logger L;
        auto st = L.init_file("/", Severity::Info);
        assert(!st.ok);
    }

    return 0;
}
