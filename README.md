# Ханойская башня — C++20

Классическая задача на трёх стержнях: A → C. Оптимальная последовательность
содержит ровно **2^N−1 ходов**. Четыре потоковых алгоритма, точный подсчёт,
текстовый вывод и ASCII/ANSI-анимация. Внешних библиотек нет.

## Сборка и тесты на Linux

Нужны GCC/Clang с C++20, CMake ≥3.20, make и Python 3 (только для тестов/замеров).
На Debian/Ubuntu: `sudo apt-get install g++ cmake make python3`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

## CLI

```sh
build/hanoi --help
build/hanoi 3                          # 7 строк вида: 1 A -> C
build/hanoi 64 --count                 # 18446744073709551615, без генерации
build/hanoi 26 --mode quiet --all      # без вывода ходов; итоговая контрольная сумма
build/hanoi 64 --mode quiet --limit 1000000
build/hanoi 4 --mode ascii --all        # последовательные кадры, без escape-кодов
build/hanoi 5 --mode ansi --delay-ms 100 --limit 20
build/hanoi 10 --algorithm recursive --mode quiet --all
```

`N` — целое от 0 до 64 включительно. 0 — пустая башня, 0 ходов.
Выше 64 точный результат не помещается в `uint64_t`; программа отклоняет ввод,
не переполняет число и не использует приближения. Номера дисков — от 1
(маленький); стержни A, B, C. Алгоритмы: `formula` (по умолчанию), `bitwise`,
`iterative`, `recursive`. Выбор алгоритма не меняет последовательность.

`--limit M` ограничивает **генерацию**, а вместе с ней и вывод. По умолчанию
лимит 1000, для `quiet` — 1000000. `--limit 0` не генерирует ходов;
визуализация показывает только начальное состояние. `--all` разрешает полную
последовательность, поэтому при больших N выполнение практически не завершится.
В конце выводятся `generated`, `total` и `complete=yes|no`; ограниченный префикс
не выдаётся за полное решение. `--count` печатает только число и не сочетается
с параметрами генерации. Ошибки ввода/вывода дают ненулевой код (обычно 2).

`quiet` не создаёт визуализатор, не хранит стержни и не печатает каждый ход.
Контрольная сумма потребляет все три поля хода и выводится один раз в конце.
ASCII/ANSI используют O(N) памяти состояния; каждый кадр имеет O(N²) символов.
ANSI явно включается пользователем и требует совместимого терминала; ASCII
подходит для файлов. Задержка 0…60000 мс задаётся только для визуализации,
действует перед каждым ходом; по умолчанию 0. Ctrl+C прерывает длинный запуск.
Большие N дают широкие кадры; практически для просмотра удобны N≤10.

## Docker

```sh
docker build -t hanoi .
docker run --rm hanoi 3 --all
docker run --rm hanoi 64 --count
docker run --rm -it hanoi 4 --mode ansi --delay-ms 150 --all
```

Multi-stage сборка запускает CTest; финальный образ содержит исполняемый файл
и runtime C++, работает от непривилегированного пользователя. Для тестов,
бенчмарков и разработки доступна стадия `build`:

```sh
docker build --target build -t hanoi-build .
docker run --rm hanoi-build ctest --test-dir build --output-on-failure
# Сохранить CSV на хост, CPU выбрать из разрешённого affinity-набора:
docker run --rm --user "$(id -u):$(id -g)" \
  -v "$PWD/benchmarks:/results" hanoi-build \
  python3 benchmarks/run.py --cpu 0 --output /results/results.csv
```

Если CPU 0 недоступен, выберите разрешённый CPU или опустите `--cpu`.
Для побитово воспроизводимого окружения сохраните ID собранного образа:
пакеты из apt со временем обновляются даже при фиксированном базовом образе.

## Измерения и устройство

```sh
python3 benchmarks/run.py --binary build/hanoi_bench --cpu 0
# Отдельный замер: algorithm,N,moves,seconds,moves_per_second,peak_rss_kib,checksum
build/hanoi_bench formula 24
```

Скрипт использует пять повторов, перемешивает порядок с фиксированным seed,
проверяет совпадение контрольных сумм и сохраняет исходные CSV/окружение.
Диапазон отдельного бенчмарка ограничен N≤30, чтобы уменьшить риск случайного
многолетнего запуска. Основное приложение поддерживает N≤64.

- [Исследование, доказательство, источники](docs/research.md).
- [Методика, реальные результаты, решения об оптимизации](docs/benchmarks.md).
- `include/hanoi/hanoi.hpp`: ядро, получатель `Move{disk,from,to}`, O(1)-формулы,
  рекурсивный и итеративный эталоны, подсчёт и доступ к отдельному ходу.
- `src/main.cpp`: CLI; `src/visualization.*`: только терминальное представление.
- `tests/`: независимая проверка правил, BFS малых задач, сравнение алгоритмов,
  старшие биты и ошибки CLI. Проверки работают и с `NDEBUG`.
- `benchmarks/`: генерация без I/O в измеряемом участке и сырые результаты.

Ядро можно подключать как CMake-target `hanoi_core`. `generate(n, limit,
algorithm, callback)` выдаёт префикс оптимального решения, возвращает число
ходов; исключения callback распространяются вызывающему коду. `move_at(n,m)`
вычисляет ход с номером m (от 1) формулой за постоянное число операций.
Невалидные N, номер хода и алгоритм отвергаются.

Sanitizer-проверка (GCC/Clang):

```sh
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie'
cmake --build build-sanitize -j2
ctest --test-dir build-sanitize --output-on-failure
```

`-fno-pie/-no-pie` в этой Linux-команде предотвращает конфликты отображения
памяти ASan в некоторых контейнерных окружениях. Не требуется release-сборке.
