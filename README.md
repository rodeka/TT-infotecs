# TT-infotecs

## Структура репозитория

```bash
TT-infotecs/
├─ CMakeLists.txt
├─ README.md
├─ logging/     # часть 1
│  ├─ include/
│  │  ├─ logging/logger.hpp
│  │  ├─ logging/sinks.hpp
│  │  └─ logging/timefmt.hpp
│  └─ src/
│     ├─ logger.cpp
│     ├─ file_sink.cpp
│     └─ udp_sink.cpp
├─ apps/
│  ├─ log_app/    # часть 2
│  │  └─ main.cpp
│  └─ log_stats/  # часть 3
│     └─ main.cpp
└─ tests/
   ├─ test_logger_basic.cpp
   ├─ test_filters_and_format.cpp
   ├─ test_udp_sink.cpp
   ├─ test_init_errors.cpp
   └─ test_timefmt.cpp
```

## Сборка
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

## Использование приложений

### Часть 2: лог в файл

```bash
./log_app app.log warn
# введите строки:
# info это не запишется (фильтр warn)
# warn Это запишется
# error Ошибка!
# quit
```
Логи будут в формате `YYYY-MM-DDTHH:MM:SS.mmm [LEVEL] message`

### Часть 3
Приемник статистики
```bash
./log_stats 9000 5 10
# слушает UDP порт 9000
# печатает статистику каждые 5 сообщений и по таймауту 10с (если она изменилась)
```

Отправитель логов
```bash
./log_app dummy.log info --udp 127.0.0.1:9000
# введите:
# info Привет
# warn Осторожно
# error Упс
# quit
```
