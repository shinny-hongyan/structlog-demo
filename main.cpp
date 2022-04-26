#include <iostream>
#include <thread>
#include <random>
#include "structlog/structlog.h"

int main(){
 structlog::Logger logger = structlog::Logger::Root();

 std::thread t1([&](){
   structlog::Logger l = logger.With("thread", 1).Clone();
   l.Info("in thread 1.");
 });
 //t1.detach();  

 for (int i = 0; i < 10; ++i){
  logger.Info("halo");
 } 
 t1.join();
 return 0;
}
