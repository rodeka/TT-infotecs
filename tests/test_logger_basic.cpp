#include "logging/logger.hpp"
#include <cassert>
#include <fstream>
#include <string>

using logging::Logger;
using logging::Severity;

static int count_lines(const std::string &path)
{
    std::ifstream in(path);
    int c = 0;
    std::string s;
    while (std::getline(in, s))
        c++;
    return c;
}

int main()
{
    const std::string f = "test_log.txt";
    // 1) test: init + фильтрация
    Logger L;
    auto st = L.init_file(f, Severity::Warn);
    assert(st.ok);

    L.log(Severity::Info, "info hidden");
    L.log(Severity::Warn, "warn shown");
    L.log(Severity::Error, "error shown");

    int n = count_lines(f);
    assert(n >= 2); // минимум WARN + ERROR

    // 2) test: смена уровня
    L.set_level(Severity::Info);
    L.log("default info visible");
    int n2 = count_lines(f);
    assert(n2 >= n + 1);

    return 0;
}