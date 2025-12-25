#include "order_book.h"
#include <iostream>
#include <algorithm>

OrderBook::OrderBook(){
    order_map.reserve(200000);
}

int OrderBook::add_order(bool is_buy, int price, int quantity){
    if(quantity <= 0){
        std::cerr << "Quantity cannot be negative. Failed.\n";
        return -1;
    }
    long long id = next_order_id++;

    Order new_order(id, is_buy, price, quantity);
    match(new_order);

    if(new_order.quantity > 0){
        if(is_buy){
            auto& level = buy_orders[price];
            level.emplace_back(id,is_buy,price,new_order.quantity);
            auto it = std::prev(level.end());
            order_map[id] = OrderLoc{true, price, it};
        }else{
            auto & level = sell_orders[price];
            level.emplace_back(id,is_buy,price,new_order.quantity);
            auto it = std::prev(level.end());
            order_map[id] = OrderLoc{false, price, it};
        }
    }
    return id;

}

bool OrderBook::remove_order(long long id){
    auto it = order_map.find(id);
    if(it==order_map.end())return false;

    OrderLoc loc = it->second;

    if(loc.is_buy){
        auto it1 = buy_orders.find(loc.price);
        if(it1 != buy_orders.end()){
            auto & level = it1->second;
            level.erase(loc.order_iterator);
            if(level.empty()){
                buy_orders.erase(it1);
            }
        }
    }else{
        auto it1 = sell_orders.find(loc.price);
        if(it1 != sell_orders.end()){
            auto & level = it1->second;
            level.erase(loc.order_iterator);
            if(level.empty()){
                sell_orders.erase(it1);
            }
        }
    }
    order_map.erase(it);
    return true;
}

bool OrderBook::modify_order(long long id, int new_price, int new_quantity){
    auto it = order_map.find(id);
    if(it==order_map.end())return false;
    OrderLoc loc = it->second;
    bool is_buy = loc.is_buy;

    remove_order(id);
    if(new_quantity <=0)return true;
    add_order_id(id,is_buy,new_price,new_quantity);
    return true;
}

int OrderBook::best_bid() const{
    if(buy_orders.empty()){
        return -1;
    }
    return buy_orders.begin()->first;
}
int OrderBook::best_ask() const{
    if(sell_orders.empty()){
        return -1;
    }
    return sell_orders.begin()->first;
}
void OrderBook::print_order_book() const{
    std::cout << "======= Order Book =======\n";
    std::cout << "Bids(BUY)\n";
    if(buy_orders.empty()){
        std::cout << "...<empty>...\n";
    }else{
        std::cout<<"Price       Qty\n";
        for(const auto& [price, orders] : buy_orders){
            long long total = 0;
            for(const auto& order : orders){
                total += order.quantity;
            }
            std::cout<<price<<"         "<<total<<"\n";
        }
    }
    std::cout << "Asks(SELL)\n";
    if(sell_orders.empty()){
        std::cout << "...<empty>...\n";    
    }else{
        std::cout<<"Price       Qty\n";
        for(const auto& [price, orders] : sell_orders){ 
            long long total = 0;
            for(const auto& order : orders){
                total += order.quantity;
            }
            std::cout<<price<<"         "<<total<<"\n";
        }
    }

    std::cout<<"==========================\n";
}

void OrderBook::match(Order& new_order){
    if(new_order.quantity <= 0) return;

    if(new_order.is_buy){
        while(new_order.quantity >0 && !sell_orders.empty()){
            auto best_ask_it = sell_orders.begin();
            int ask_price = best_ask_it->first;

            if(ask_price > new_order.price) break;

            auto& level = best_ask_it->second;

            while(new_order.quantity >0 && !level.empty()){
                Order& sell_order = level.front();
                int trade_quantity = std::min(new_order.quantity, sell_order.quantity);
                new_order.quantity -= trade_quantity;
                sell_order.quantity -= trade_quantity;
                std::cout << "TRADE " 
                        << new_order.id<<" "
                        << sell_order.id <<" " 
                        << ask_price <<" "
                        << trade_quantity <<"\n";
                if(sell_order.quantity == 0){
                    order_map.erase(sell_order.id);
                    level.pop_front();
                }
            }
            if(level.empty()){
                sell_orders.erase(best_ask_it);
            }
        }
    }else{
        while(new_order.quantity > 0 && !buy_orders.empty()){
            auto best_bid_it = buy_orders.begin();
            int bid_price = best_bid_it -> first;

            if(bid_price < new_order.price)break;

            auto& level = best_bid_it->second;
            
            while(new_order.quantity > 0 && !level.empty()){
                Order& buy_order = level.front();
                int trade_quantity = std::min(buy_order.quantity,new_order.quantity);
                buy_order.quantity-=trade_quantity;
                new_order.quantity -= trade_quantity;
                std::cout << "TRADE " 
                        << buy_order.id <<" " 
                        << new_order.id <<" "
                        << bid_price <<" "
                        << trade_quantity<<"\n";
                if(buy_order.quantity==0){
                    order_map.erase(buy_order.id);
                    level.pop_front();
                }
            }
            if(level.empty()){
                buy_orders.erase(best_bid_it);
            }
        }
    }
}

void OrderBook::add_order_id(long long id, bool is_buy, int price, int quantity){
    if(quantity <= 0){
        return;
    }
    if(order_map.find(id) != order_map.end()){
        return;
    }

    Order new_order(id, is_buy, price, quantity);
    match(new_order);

    if(new_order.quantity > 0){
        if(is_buy){
            auto &level = buy_orders[price];
            level.emplace_back(id,is_buy,price,new_order.quantity);
            auto it = std::prev(level.end());
            order_map[id] = OrderLoc{true, price, it};
        }else{
            auto& level = sell_orders[price];
            level.emplace_back(id,is_buy,price,new_order.quantity);
            auto it  = std::prev(level.end());
            order_map[id] = OrderLoc{false, price, it};
        }
    }
}