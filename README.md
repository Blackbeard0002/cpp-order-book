# C++ Limit Order Book

Single-threaded limit order book matching engine in C++17.

## Features

- Order types: LIMIT, MARKET, IOC, FOK
- Price-time priority (FIFO within each price level)
- Add, cancel, and modify orders
- In-memory trade log
- Built-in benchmark with throughput and latency percentiles

## Build

```bash
mkdir -p build && cd build
cmake ..
cmake --build . -j
```

Manual build:

```bash
g++ -std=c++17 -O3 -march=native -flto -Iinclude \
    main.cpp order_book.cpp order_book_engine.cpp -o ob
```

## Run

Interactive CLI:

```bash
./ob
```

Scripted input:

```bash
./ob < test.txt
```

### Commands

| Command | Example |
|---------|---------|
| Add limit order | `ADD BUY 100 10` |
| Add with type | `ADD SELL 100 5 FOK` |
| Market order | `ADD BUY MKT 10` |
| Cancel | `REMOVE 3` |
| Modify | `MODIFY 4 101 7` |
| Best bid/ask | `BEST` |
| Print book | `PRINT` |
| Print trades | `TRADES` |
| Benchmark | `BENCH 1000000` |
| Quit | `EXIT` |

## Order semantics

- **LIMIT** — match, then rest any remainder in the book
- **MARKET** — match immediately; unfilled quantity is discarded
- **IOC** — match up to the limit; unfilled quantity is cancelled
- **FOK** — fill entirely or reject with no trades

## Benchmark

`BENCH N` runs `N` LIMIT orders with:

- 50/50 buy/sell split
- uniform price in [90, 110]
- uniform quantity in [1, 10]
- RNG seed 42
- 10,000-order warmup before timing

Logging is disabled during the timed section. Each `add_order` is timed individually; results include throughput and latency percentiles.

Example on a typical Linux build (`-O3 -march=native -flto`):

```
Orders: 1000000
Throughput: 15400000 ops/sec
p50 <= 64 ns
p99 <= 128 ns
```

Exact numbers vary by CPU and system load.

## Layout

```
cpp-order-book/
├── include/
│   ├── flat_hash_map.hpp
│   ├── latency_histogram.hpp
│   └── object_pool.hpp
├── order_book.h
├── order_book.cpp
├── order_book_engine.cpp
├── main.cpp
├── CMakeLists.txt
└── test.txt
```

## Design notes

- Orders and price levels live in pre-allocated object pools
- Price levels are kept in sorted intrusive lists; best bid/ask is O(1)
- Per-level FIFO queues use index-based doubly linked lists
- Order lookup uses an open-addressing `FlatHashMap`
- Trades are stored in a pre-sized in-memory buffer (no disk persistence)
- Default pool sizes: 500k orders, 16k levels, 2M trades
