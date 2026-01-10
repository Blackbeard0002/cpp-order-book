#include <iostream>
#include <string>
#include<chrono>
#include<random>
#include "order_book.h"

int main(){
    OrderBook ob;

    std::string cmd;
    std::cout << "Welcome to the Order Book System!\n";
    std::cout << "Available commands: ADD, REMOVE, MODIFY, BEST, PRINT, TRADES, BENCH, EXIT\n";
    
    while(std::cin>>cmd){
        if(cmd == "ADD"){
            std::string side;
            int price, quantity;
            std::string price_str;
            std::string type_str = "LIMIT";//default
            std::cin >> side >> price_str >> quantity;

            if(std::cin.peek() == ' '){
                std::cin>>type_str;
            }

            OrderType type;
            if(side != "BUY" && side != "SELL"){
                std::cout << "Invalid side. Use BUY or SELL.\n";
                continue;
            }else{
                bool is_buy = (side == "BUY");
                if(price_str == "MKT"){
                    type = OrderType::MARKET;
                    price = is_buy?1e9:0;
                }else{
                    price = std::stoi(price_str);
                    if(type_str == "IOC"){
                        type = OrderType::IOC;
                    }else if(type_str == "FOK"){
                        type = OrderType::FOK;
                    }else{
                        type = OrderType::LIMIT;
                    }
                }
                
                int id = ob.add_order(is_buy, price, quantity,type);
                if(id != -1){
                    std::cout << "Order Accepted. ID: " << id << "\n";
                }else{
                    std::cout << "Order Killed.\n";
                }
            }
        }else if(cmd == "REMOVE"){
            long long id;
            std::cin >> id;
            if(ob.remove_order(id)){
                std::cout<<"Order Removed.\n";
            }else{
                std::cout<<"Order Not Found.\n";
            }
        }else if(cmd == "MODIFY"){
            long long id;
            int new_price, new_quantity;
            std::cin >> id >> new_price >> new_quantity;
            
            if(ob.modify_order(id, new_price, new_quantity)){
                std::cout << "Order Modified.\n";
            }else{
                std::cout<< "Order Not Found.\n";
            }
            
        }else if(cmd == "BEST"){
            int best_bid = ob.best_bid();
            int best_ask = ob.best_ask();
            if(best_bid==-1){
                std::cout<<"Best Bid: N/A\n";
            }else{
                std::cout<<"Best Bid: "<<best_bid<<"\n";
            }
            if(best_ask==-1){
                std::cout<<"Best Ask: N/A\n";
            }else{
                std::cout<<"Best Ask: "<<best_ask<<"\n";
            }
        }else if(cmd == "PRINT"){
            ob.print_order_book();
        }else if(cmd == "EXIT"){
            std::cout << "Exiting the Order Book System. Goodbye!\n";
            break;
        }else if(cmd == "TRADES"){
            ob.print_trades();
        }else if(cmd == "BENCH"){
            int n;
            std::cin>>n;

            ob.set_logging(false);

            using clock = std::chrono::high_resolution_clock;

            std::mt19937 rng(42);
            std::uniform_int_distribution<int> price_dist(90,110);
            std::uniform_int_distribution<int> qty_dist(1,10);
            std::uniform_int_distribution<int> side_dist(0,1);

            auto start = clock::now();

            for(int i=0;i<n;i++){
                bool is_buy = side_dist(rng);
                int price = price_dist(rng);
                int qty = qty_dist(rng);

                ob.add_order(is_buy, price, qty, OrderType::LIMIT);
            }
            
            auto end = clock::now();

            ob.set_logging(true);

            std::chrono::duration<double> elapsed = end - start;

            std::cout<<"Benchmark Completed:\n";
            std::cout<<"Orders "<<n<<"\n";
            std::cout<<"Time: "<<elapsed.count()<<" seconds\n";
            std::cout<<"Throughput: "
                    << static_cast<long long> (n / elapsed.count())
                    << " ops/sec\n";

        }else{
            std::cout << "Unknown command. Please try again.\n";
        }
    }
    return 0;
}