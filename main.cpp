#include <iostream>
#include <string>
#include "order_book.h"

int main(){
    OrderBook ob;

    std::string cmd;
    std::cout << "Welcome to the Order Book System!\n";
    std::cout << "Available commands: ADD, REMOVE, MODIFY, BEST, PRINT, EXIT\n";
    
    while(std::cin>>cmd){
        if(cmd == "ADD"){
            std::string side;
            int price, quantity;
            std::cin >> side >> price >> quantity;
            if(side != "BUY" && side != "SELL"){
                std::cout << "Invalid side. Use BUY or SELL.\n";
                continue;
            }else{
                bool is_buy = (side == "BUY");
                int id = ob.add_order(is_buy, price, quantity);
                if(id != -1){
                    std::cout << "Order Accepted. ID: " << id << "\n";
                }else{
                    std::cout << "Failed.\n";
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
        }else{
            std::cout << "Unknown command. Please try again.\n";
        }
    }
    return 0;
}