#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "latency_histogram.hpp"
#include "order_book.h"

namespace {

struct BenchOrder {
    bool is_buy;
    int price;
    int quantity;
};

void run_benchmark(OrderBook& ob, int n) {
    ob.set_logging(false);

    using clock = std::chrono::high_resolution_clock;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> price_dist(90, 110);
    std::uniform_int_distribution<int> qty_dist(1, 10);
    std::uniform_int_distribution<int> side_dist(0, 1);

    std::vector<BenchOrder> orders(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        orders[static_cast<std::size_t>(i)] = BenchOrder{
            side_dist(rng) == 0,
            price_dist(rng),
            qty_dist(rng),
        };
    }

    constexpr int kWarmup = 10'000;
    const int warmup_count = n < kWarmup ? n : kWarmup;
    for (int i = 0; i < warmup_count; ++i) {
        const BenchOrder& o = orders[static_cast<std::size_t>(i)];
        ob.add_order(o.is_buy, o.price, o.quantity, OrderType::LIMIT);
    }

    LatencyHistogram histogram;
    const auto bench_start = clock::now();

    for (int i = 0; i < n; ++i) {
        const BenchOrder& o = orders[static_cast<std::size_t>(i)];
        const auto t0 = clock::now();
        ob.add_order(o.is_buy, o.price, o.quantity, OrderType::LIMIT);
        const auto t1 = clock::now();
        const auto ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        histogram.record(ns);
    }

    const auto bench_end = clock::now();
    ob.set_logging(true);

    const double elapsed =
        std::chrono::duration<double>(bench_end - bench_start).count();

    std::cout << "Benchmark Completed:\n";
    std::cout << "Orders: " << n << '\n';
    std::cout << "Time:   " << elapsed << " seconds\n";
    std::cout << "Throughput: " << static_cast<long long>(n / elapsed) << " ops/sec\n";
    histogram.print(std::cout);
}

}  // namespace

int main() {
    OrderBook ob;

    std::string cmd;
    std::cout << "Welcome to the Order Book System!\n";
    std::cout << "Available commands: ADD, REMOVE, MODIFY, BEST, PRINT, TRADES, BENCH, EXIT\n";

    while (std::cin >> cmd) {
        if (cmd == "ADD") {
            std::string side;
            int price, quantity;
            std::string price_str;
            std::string type_str = "LIMIT";
            std::cin >> side >> price_str >> quantity;

            if (std::cin.peek() == ' ') {
                std::cin >> type_str;
            }

            OrderType type;
            if (side != "BUY" && side != "SELL") {
                std::cout << "Invalid side. Use BUY or SELL.\n";
                continue;
            }

            const bool is_buy = (side == "BUY");
            if (price_str == "MKT") {
                type = OrderType::MARKET;
                price = is_buy ? 1'000'000'000 : 0;
            } else {
                price = std::stoi(price_str);
                if (type_str == "IOC") {
                    type = OrderType::IOC;
                } else if (type_str == "FOK") {
                    type = OrderType::FOK;
                } else {
                    type = OrderType::LIMIT;
                }
            }

            const int id = ob.add_order(is_buy, price, quantity, type);
            if (id != -1) {
                std::cout << "Order Accepted. ID: " << id << '\n';
            } else {
                std::cout << "Order Killed.\n";
            }
        } else if (cmd == "REMOVE") {
            long long id;
            std::cin >> id;
            if (ob.remove_order(id)) {
                std::cout << "Order Removed.\n";
            } else {
                std::cout << "Order Not Found.\n";
            }
        } else if (cmd == "MODIFY") {
            long long id;
            int new_price, new_quantity;
            std::cin >> id >> new_price >> new_quantity;

            if (ob.modify_order(id, new_price, new_quantity)) {
                std::cout << "Order Modified.\n";
            } else {
                std::cout << "Order Not Found.\n";
            }
        } else if (cmd == "BEST") {
            const int best_bid = ob.best_bid();
            const int best_ask = ob.best_ask();
            if (best_bid == -1) {
                std::cout << "Best Bid: N/A\n";
            } else {
                std::cout << "Best Bid: " << best_bid << '\n';
            }
            if (best_ask == -1) {
                std::cout << "Best Ask: N/A\n";
            } else {
                std::cout << "Best Ask: " << best_ask << '\n';
            }
        } else if (cmd == "PRINT") {
            ob.print_order_book();
        } else if (cmd == "EXIT") {
            std::cout << "Exiting the Order Book System. Goodbye!\n";
            break;
        } else if (cmd == "TRADES") {
            ob.print_trades();
        } else if (cmd == "BENCH") {
            int n;
            std::cin >> n;
            run_benchmark(ob, n);
        } else {
            std::cout << "Unknown command. Please try again.\n";
        }
    }
    return 0;
}
