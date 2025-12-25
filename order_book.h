#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <iostream>
#include <list>
#include <map>
#include <unordered_map>

struct Order {
    long long id;
    bool is_buy;
    int price;
    int quantity;

    Order(long long id, bool is_buy, int price, int quantity)
        : id(id), is_buy(is_buy), price(price), quantity(quantity) {}
};

struct OrderLoc{
    bool is_buy;
    int price;
    std::list<Order>::iterator order_iterator;
};

class OrderBook{
public:
    OrderBook();

    int add_order(bool is_buy, int price, int quantity);
    bool remove_order(long long id);
    bool modify_order(long long id, int new_price, int new_quantity);

    int best_bid() const;
    int best_ask() const;
    void print_order_book() const;

private:
    long long next_order_id = 1;
    std::unordered_map<long long, OrderLoc> order_map;
    std::map<int, std::list<Order>, std::greater<int>> buy_orders;
    std::map<int, std::list<Order>> sell_orders;

    void match(Order& new_order);
    void add_order_id(long long id, bool is_buy, int price, int quantity);
};

#endif // ORDER_BOOK_H