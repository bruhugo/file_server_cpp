#include <iostream>
#include "ThreadPool.hpp"
#include <stdlib.h>
#include <chrono>
#include <mutex>

int main(int argc, char *argv[]){

    ThreadPool threadPool(5);

    for (int i = 0; i < 10000; i++){
        threadPool.submit([]{
            for (int i = 0; i < 10000; ++i){}
        });
    }
    return 0;
}