#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <cstdint>
#include <vector>

#include "flat_hash_map.hpp"
#include "object_pool.hpp"

enum class OrderType {
    LIMIT,
    MARKET,
    IOC,
    FOK
};

struct Trade {
    std::int64_t buy_id;
    std::int64_t sell_id;
    int price;
    int quantity;
};

struct alignas(64) OrderNode {
    std::int64_t id = 0;
    std::int32_t price = 0;
    std::int32_t quantity = 0;
    std::uint32_t prev = ObjectPool<OrderNode>::kInvalid;
    std::uint32_t next = ObjectPool<OrderNode>::kInvalid;
    bool is_buy = false;
};

struct alignas(64) PriceLevel {
    std::int32_t price = 0;
    std::int32_t total_qty = 0;
    std::uint32_t head_order = ObjectPool<OrderNode>::kInvalid;
    std::uint32_t tail_order = ObjectPool<OrderNode>::kInvalid;
    std::uint32_t next_worse = ObjectPool<PriceLevel>::kInvalid;
};

struct BookSide {
    std::uint32_t best_level = ObjectPool<PriceLevel>::kInvalid;
    FlatHashMap<std::int32_t, std::uint32_t> price_to_level;
};

class OrderBook {
public:
    static constexpr std::size_t kDefaultMaxOrders = 500'000;
    static constexpr std::size_t kDefaultMaxLevels = 16'384;
    static constexpr std::size_t kDefaultMaxTrades = 2'000'000;

    explicit OrderBook(std::size_t max_orders = kDefaultMaxOrders,
                       std::size_t max_levels = kDefaultMaxLevels,
                       std::size_t max_trades = kDefaultMaxTrades);

    int add_order(bool is_buy, int price, int quantity, OrderType type);
    bool remove_order(std::int64_t id);
    bool modify_order(std::int64_t id, int new_price, int new_quantity);

    int best_bid() const;
    int best_ask() const;
    void print_order_book() const;
    void print_trades() const;

    void set_logging(bool enabled);

    [[nodiscard]] std::size_t trade_count() const noexcept { return trade_count_; }
    [[nodiscard]] const std::vector<Trade>& trades() const noexcept { return trades_; }

private:
    bool enable_logging_ = true;

    std::int64_t next_order_id_ = 1;

    ObjectPool<OrderNode> order_pool_;
    ObjectPool<PriceLevel> level_pool_;

    FlatHashMap<std::int64_t, std::uint32_t> order_by_id_;

    BookSide bids_;
    BookSide asks_;

    std::vector<Trade> trades_;
    std::size_t trade_count_ = 0;

    void match(std::uint32_t incoming_idx);
    void rest_order(std::uint32_t order_idx, BookSide& side, std::int32_t price);
    bool remove_order_at(std::uint32_t order_idx);
    void record_trade(std::int64_t buy_id, std::int64_t sell_id, int price, int quantity);

    std::uint32_t get_or_create_level(BookSide& side, std::int32_t price, bool is_bid_side);
    void remove_level_if_empty(BookSide& side, std::uint32_t level_idx);
    void unlink_order_from_level(std::uint32_t level_idx, std::uint32_t order_idx);

    bool can_fully_fill(bool is_buy, int price, int quantity) const;
    void add_order_with_id(std::int64_t id, bool is_buy, int price, int quantity);

    void match_against_asks(std::uint32_t incoming_idx);
    void match_against_bids(std::uint32_t incoming_idx);
};

#endif
