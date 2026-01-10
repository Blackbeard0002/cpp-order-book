# C++ Limit Order Book Matching Engine

A high-performance single-threaded limit order book (LOB) matching engine written in modern C++ (C++17), supporting multiple order types, price-time priority matching, trade logging, and throughput benchmarking.

## Features

- Supports multiple order types:
  - LIMIT
  - MARKET
  - IOC (Immediate-Or-Cancel)
  - FOK (Fill-Or-Kill)

- Price-time priority matching (FIFO within price levels)

- Partial fills where applicable

- Order management:
  - Add orders
  - Cancel orders
  - Modify orders

- Persistent trade log with replay support

- Built-in benchmark mode to measure matching throughput

## Design Overview

The matching engine is built around a price-level order book with strict price-time priority.

- Orders are grouped by price levels.
- Each price level maintains a FIFO queue.
- Best bid and best ask are always accessible in O(1) time.
- Orders are indexed by ID to allow fast cancellation and modification.
- The matching engine processes incoming orders synchronously and guarantees deterministic execution order.

## Core Data Structures

| Component | Data Structure | Reason |
|---------|---------------|--------|
Buy side | `std::map<int, std::list<Order>, std::greater<int>>` | Fast best-bid lookup |
Sell side | `std::map<int, std::list<Order>>` | Fast best-ask lookup |
Price level | `std::list<Order>` | FIFO execution and O(1) erase |
Order index | `std::unordered_map<OrderID, OrderLoc>` | O(1) cancel and modify |
Trade log | `std::vector<Trade>` | Sequential execution history |

## Order Type Semantics

- **LIMIT**  
  Matches against the opposing book up to its limit price. Any remaining quantity rests in the book.

- **MARKET**  
  Matches immediately against the best available prices. Remaining quantity is discarded.

- **IOC (Immediate-Or-Cancel)**  
  Attempts immediate execution up to its limit price. Any unfilled quantity is cancelled.

- **FOK (Fill-Or-Kill)**  
  Executes only if the full quantity can be matched immediately. Otherwise, the order is cancelled without any trades.

## Time Complexity

| Operation | Complexity |
|---------|------------|
Add order | O(log P) |
Cancel order | O(1) average |
Modify order | O(log P) |
Best bid / ask | O(1) |
Matching | O(number of matched orders) |

Where P is the number of price levels.

## Benchmark

The engine includes a built-in benchmark mode to measure limit-order throughput.

Command:

```BENCH 100000```

Sample result:
```Throughput: ~5.3 million orders/sec```

Logging is disabled during benchmarking to ensure measurements reflect matching and data-structure performance rather than I/O overhead.

## Build and Run

### Compile (Windows)
```g++ -std=c++17 -O2 main.cpp order_book.cpp -o ob.exe```

### Run
```ob.exe```
### Run using input file
```ob.exe < test.txt```

## Notes

This project focuses on correctness, clean design, and measurable performance rather than premature optimization. It is intended as a foundational matching engine suitable for further extensions such as multithreading, custom memory allocators, or market data simulation.
