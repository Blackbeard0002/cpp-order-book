#include "order_book.h"

#include <iostream>

OrderBook::OrderBook(std::size_t max_orders, std::size_t max_levels, std::size_t max_trades)
    : order_pool_(max_orders),
      level_pool_(max_levels),
      order_by_id_(max_orders * 2),
      trades_(max_trades) {
    bids_.price_to_level.reserve(max_levels * 2);
    asks_.price_to_level.reserve(max_levels * 2);
}

int OrderBook::add_order(bool is_buy, int price, int quantity, OrderType type) {
    if (quantity <= 0) [[unlikely]] {
        std::cerr << "Quantity cannot be negative. Failed.\n";
        return -1;
    }

    if (type == OrderType::FOK && !can_fully_fill(is_buy, price, quantity)) [[unlikely]] {
        return -1;
    }

    const std::int64_t id = next_order_id_++;
    const std::uint32_t order_idx = order_pool_.acquire();
    if (order_idx == ObjectPool<OrderNode>::kInvalid) [[unlikely]] {
        return -1;
    }

    OrderNode& order = order_pool_.get(order_idx);
    order.id = id;
    order.is_buy = is_buy;
    order.price = price;
    order.quantity = quantity;
    order.prev = ObjectPool<OrderNode>::kInvalid;
    order.next = ObjectPool<OrderNode>::kInvalid;

    order_by_id_[id] = order_idx;

    match(order_idx);

    OrderNode& live = order_pool_.get(order_idx);
    if (live.quantity > 0) {
        if (type == OrderType::MARKET || type == OrderType::IOC) {
            remove_order_at(order_idx);
        } else {
            BookSide& side = is_buy ? bids_ : asks_;
            rest_order(order_idx, side, price);
        }
    } else {
        order_by_id_.erase(id);
        order_pool_.release(order_idx);
    }

    return id;
}

void OrderBook::add_order_with_id(std::int64_t id, bool is_buy, int price, int quantity) {
    if (quantity <= 0) [[unlikely]] {
        return;
    }
    if (order_by_id_.find(id) != nullptr) [[unlikely]] {
        return;
    }

    const std::uint32_t order_idx = order_pool_.acquire();
    if (order_idx == ObjectPool<OrderNode>::kInvalid) [[unlikely]] {
        return;
    }

    OrderNode& order = order_pool_.get(order_idx);
    order.id = id;
    order.is_buy = is_buy;
    order.price = price;
    order.quantity = quantity;
    order.prev = ObjectPool<OrderNode>::kInvalid;
    order.next = ObjectPool<OrderNode>::kInvalid;

    order_by_id_[id] = order_idx;
    match(order_idx);

    OrderNode& live = order_pool_.get(order_idx);
    if (live.quantity > 0) {
        BookSide& side = is_buy ? bids_ : asks_;
        rest_order(order_idx, side, price);
    } else {
        order_by_id_.erase(id);
        order_pool_.release(order_idx);
    }
}

bool OrderBook::remove_order(std::int64_t id) {
    const std::uint32_t* idx = order_by_id_.find(id);
    if (idx == nullptr) [[unlikely]] {
        return false;
    }
    return remove_order_at(*idx);
}

bool OrderBook::modify_order(std::int64_t id, int new_price, int new_quantity) {
    const std::uint32_t* idx = order_by_id_.find(id);
    if (idx == nullptr) [[unlikely]] {
        return false;
    }

    const OrderNode& existing = order_pool_.get(*idx);
    const bool is_buy = existing.is_buy;

    remove_order_at(*idx);
    if (new_quantity <= 0) {
        return true;
    }
    add_order_with_id(id, is_buy, new_price, new_quantity);
    return true;
}

int OrderBook::best_bid() const {
    if (bids_.best_level == ObjectPool<PriceLevel>::kInvalid) {
        return -1;
    }
    return level_pool_.get(bids_.best_level).price;
}

int OrderBook::best_ask() const {
    if (asks_.best_level == ObjectPool<PriceLevel>::kInvalid) {
        return -1;
    }
    return level_pool_.get(asks_.best_level).price;
}

void OrderBook::set_logging(bool enabled) { enable_logging_ = enabled; }

void OrderBook::print_order_book() const {
    std::cout << "======= Order Book =======\n";
    std::cout << "Bids(BUY)\n";

    if (bids_.best_level == ObjectPool<PriceLevel>::kInvalid) {
        std::cout << "...<empty>...\n";
    } else {
        std::cout << "Price       Qty\n";
        std::uint32_t level_idx = bids_.best_level;
        while (level_idx != ObjectPool<PriceLevel>::kInvalid) {
            const PriceLevel& level = level_pool_.get(level_idx);
            std::cout << level.price << "         " << level.total_qty << "\n";
            level_idx = level.next_worse;
        }
    }

    std::cout << "Asks(SELL)\n";
    if (asks_.best_level == ObjectPool<PriceLevel>::kInvalid) {
        std::cout << "...<empty>...\n";
    } else {
        std::cout << "Price       Qty\n";
        std::uint32_t level_idx = asks_.best_level;
        while (level_idx != ObjectPool<PriceLevel>::kInvalid) {
            const PriceLevel& level = level_pool_.get(level_idx);
            std::cout << level.price << "         " << level.total_qty << "\n";
            level_idx = level.next_worse;
        }
    }

    std::cout << "==========================\n";
}

void OrderBook::print_trades() const {
    if (trade_count_ == 0) {
        std::cout << "No Trades yet.\n";
        return;
    }

    std::cout << "======= Trade Log =======\n";
    for (std::size_t i = 0; i < trade_count_; ++i) {
        const Trade& t = trades_[i];
        std::cout << "Trade " << t.buy_id << " " << t.sell_id << " " << t.price << " "
                  << t.quantity << "\n";
    }
    std::cout << "========================\n";
}
