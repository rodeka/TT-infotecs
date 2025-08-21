#include "logging/timefmt.hpp"
#include <cassert>
#include <cctype>
#include <string>

using logging::now_iso_local;

static bool is_digit_str(const std::string& s) {
    for (char c : s) if (!std::isdigit((unsigned char)c)) return false;
    return true;
}

int main() {
    std::string t = now_iso_local();
    // ожидаем формат вроде: YYYY-MM-DDTHH:MM:SS.mmm (минимум 23 символа)
    assert(t.size() >= 23);

    // базовая проверка разметки
    assert(t[4] == '-');
    assert(t[7] == '-');
    assert(t[10] == 'T');
    assert(t[13] == ':');
    assert(t[16] == ':');

    // проверяем, что миллисекунды отделены точкой и цифры
    auto dot = t.find('.');
    assert(dot != std::string::npos);
    std::string ms = t.substr(dot + 1);
    assert(ms.size() >= 3);
    for (char c : ms) assert(std::isdigit((unsigned char)c));

    // Проверяем, что год, месяц, день — цифры
    assert(is_digit_str(t.substr(0, 4)));
    assert(is_digit_str(t.substr(5, 2)));
    assert(is_digit_str(t.substr(8, 2)));

    return 0;
}
