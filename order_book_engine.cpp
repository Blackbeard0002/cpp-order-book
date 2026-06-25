#include "order_book.h"

#include <iostream>

namespace {

constexpr std::uint32_t kInvalid = ObjectPool<OrderNode>::kInvalid;

[[nodiscard]] int clamp_trade_qty(int a, int b) noexcept {
    return a < b ? a : b;
}

}  // namespace

void OrderBook::record_trade(std::int64_t buy_id, std::int64_t sell_id, int price, int quantity) {
    if (trade_count_ < trades_.size()) [[likely]] {
        trades_[trade_count_++] = Trade{buy_id, sell_id, price, quantity};
    }
}

std::uint32_t OrderBook::get_or_create_level(BookSide& side, std::int32_t price, bool is_bid_side) {
    if (std::uint32_t* existing = side.price_to_level.find(price)) [[likely]] {
        return *existing;
    }

    const std::uint32_t level_idx = level_pool_.acquire();
    if (level_idx == kInvalid) [[unlikely]] {
        return kInvalid;
    }

    PriceLevel& level = level_pool_.get(level_idx);
    level.price = price;
    level.total_qty = 0;
    level.head_order = kInvalid;
    level.tail_order = kInvalid;
    level.next_worse = kInvalid;

    side.price_to_level[price] = level_idx;

    if (side.best_level == kInvalid) {
        side.best_level = level_idx;
        return level_idx;
    }

    std::uint32_t prev = kInvalid;
    std::uint32_t cur = side.best_level;

    while (cur != kInvalid) {
        PriceLevel& cur_level = level_pool_.get(cur);
        const bool insert_before =
            is_bid_side ? (price > cur_level.price) : (price < cur_level.price);

        if (insert_before) {
            if (prev == kInvalid) {
                level.next_worse = side.best_level;
                side.best_level = level_idx;
            } else {
                PriceLevel& prev_level = level_pool_.get(prev);
                level.next_worse = prev_level.next_worse;
                prev_level.next_worse = level_idx;
            }
            return level_idx;
        }

        prev = cur;
        cur = cur_level.next_worse;
    }

    PriceLevel& prev_level = level_pool_.get(prev);
    prev_level.next_worse = level_idx;
    return level_idx;
}

void OrderBook::rest_order(std::uint32_t order_idx, BookSide& side, std::int32_t price) {
    const bool is_bid_side = &side == &bids_;
    const std::uint32_t level_idx = get_or_create_level(side, price, is_bid_side);
    if (level_idx == kInvalid) [[unlikely]] {
        return;
    }

    PriceLevel& level = level_pool_.get(level_idx);
    OrderNode& order = order_pool_.get(order_idx);

    if (level.tail_order == kInvalid) {
        level.head_order = order_idx;
        level.tail_order = order_idx;
        order.prev = kInvalid;
        order.next = kInvalid;
    } else {
        OrderNode& tail = order_pool_.get(level.tail_order);
        tail.next = order_idx;
        order.prev = level.tail_order;
        order.next = kInvalid;
        level.tail_order = order_idx;
    }

    level.total_qty += order.quantity;
}

void OrderBook::unlink_order_from_level(std::uint32_t level_idx, std::uint32_t order_idx) {
    PriceLevel& level = level_pool_.get(level_idx);
    OrderNode& order = order_pool_.get(order_idx);

    if (order.prev != kInvalid) {
        order_pool_.get(order.prev).next = order.next;
    } else {
        level.head_order = order.next;
    }

    if (order.next != kInvalid) {
        order_pool_.get(order.next).prev = order.prev;
    } else {
        level.tail_order = order.prev;
    }

    level.total_qty -= order.quantity;
    order.prev = kInvalid;
    order.next = kInvalid;
}

void OrderBook::remove_level_if_empty(BookSide& side, std::uint32_t level_idx) {
    PriceLevel& level = level_pool_.get(level_idx);
    if (level.head_order != kInvalid) [[likely]] {
        return;
    }

    side.price_to_level.erase(level.price);

    if (side.best_level == level_idx) {
        side.best_level = level.next_worse;
    } else {
        std::uint32_t cur = side.best_level;
        while (cur != kInvalid) {
            PriceLevel& cur_level = level_pool_.get(cur);
            if (cur_level.next_worse == level_idx) {
                cur_level.next_worse = level.next_worse;
                break;
            }
            cur = cur_level.next_worse;
        }
    }

    level_pool_.release(level_idx);
}

bool OrderBook::remove_order_at(std::uint32_t order_idx) {
    if (!order_pool_.in_use(order_idx)) [[unlikely]] {
        return false;
    }

    OrderNode& order = order_pool_.get(order_idx);
    BookSide& side = order.is_buy ? bids_ : asks_;
    const std::uint32_t* level_idx_ptr = side.price_to_level.find(order.price);
    if (level_idx_ptr != nullptr) {
        unlink_order_from_level(*level_idx_ptr, order_idx);
        remove_level_if_empty(side, *level_idx_ptr);
    }

    order_by_id_.erase(order.id);
    order_pool_.release(order_idx);
    return true;
}

void OrderBook::match(std::uint32_t incoming_idx) {
    OrderNode& incoming = order_pool_.get(incoming_idx);
    if (incoming.quantity <= 0) [[unlikely]] {
        return;
    }

    if (incoming.is_buy) {
        match_against_asks(incoming_idx);
    } else {
        match_against_bids(incoming_idx);
    }
}

void OrderBook::match_against_asks(std::uint32_t incoming_idx) {
    OrderNode& incoming = order_pool_.get(incoming_idx);

    while (incoming.quantity > 0 && asks_.best_level != kInvalid) [[likely]] {
        const std::uint32_t level_idx = asks_.best_level;
        PriceLevel& level = level_pool_.get(level_idx);
        const int ask_price = level.price;

        if (ask_price > incoming.price) {
            break;
        }

        while (incoming.quantity > 0 && level.head_order != kInvalid) [[likely]] {
            const std::uint32_t resting_idx = level.head_order;
            OrderNode& resting = order_pool_.get(resting_idx);

            const int trade_qty = clamp_trade_qty(incoming.quantity, resting.quantity);
            incoming.quantity -= trade_qty;
            resting.quantity -= trade_qty;
            level.total_qty -= trade_qty;

            if (enable_logging_) {
                std::cout << "TRADE " << incoming.id << " " << resting.id << " " << ask_price
                          << " " << trade_qty << "\n";
            }

            record_trade(incoming.id, resting.id, ask_price, trade_qty);

            if (resting.quantity == 0) [[unlikely]] {
                unlink_order_from_level(level_idx, resting_idx);
                order_by_id_.erase(resting.id);
                order_pool_.release(resting_idx);
            }
        }

        if (level.head_order == kInvalid) [[unlikely]] {
            remove_level_if_empty(asks_, level_idx);
        }
    }
}

void OrderBook::match_against_bids(std::uint32_t incoming_idx) {
    OrderNode& incoming = order_pool_.get(incoming_idx);

    while (incoming.quantity > 0 && bids_.best_level != kInvalid) [[likely]] {
        const std::uint32_t level_idx = bids_.best_level;
        PriceLevel& level = level_pool_.get(level_idx);
        const int bid_price = level.price;

        if (bid_price < incoming.price) {
            break;
        }

        while (incoming.quantity > 0 && level.head_order != kInvalid) [[likely]] {
            const std::uint32_t resting_idx = level.head_order;
            OrderNode& resting = order_pool_.get(resting_idx);

            const int trade_qty = clamp_trade_qty(incoming.quantity, resting.quantity);
            resting.quantity -= trade_qty;
            incoming.quantity -= trade_qty;
            level.total_qty -= trade_qty;

            if (enable_logging_) {
                std::cout << "TRADE " << resting.id << " " << incoming.id << " " << bid_price
                          << " " << trade_qty << "\n";
            }

            record_trade(resting.id, incoming.id, bid_price, trade_qty);

            if (resting.quantity == 0) [[unlikely]] {
                unlink_order_from_level(level_idx, resting_idx);
                order_by_id_.erase(resting.id);
                order_pool_.release(resting_idx);
            }
        }

        if (level.head_order == kInvalid) [[unlikely]] {
            remove_level_if_empty(bids_, level_idx);
        }
    }
}

bool OrderBook::can_fully_fill(bool is_buy, int price, int quantity) const {
    int remaining = quantity;

    if (is_buy) {
        std::uint32_t level_idx = asks_.best_level;
        while (remaining > 0 && level_idx != kInvalid) {
            const PriceLevel& level = level_pool_.get(level_idx);
            if (level.price > price) {
                break;
            }
            remaining -= level.total_qty;
            if (remaining <= 0) {
                return true;
            }
            level_idx = level.next_worse;
        }
    } else {
        std::uint32_t level_idx = bids_.best_level;
        while (remaining > 0 && level_idx != kInvalid) {
            const PriceLevel& level = level_pool_.get(level_idx);
            if (level.price < price) {
                break;
            }
            remaining -= level.total_qty;
            if (remaining <= 0) {
                return true;
            }
            level_idx = level.next_worse;
        }
    }

    return false;
}
