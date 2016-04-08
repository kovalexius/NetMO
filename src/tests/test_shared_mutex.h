#ifndef TEST_SHARED_MUTEX_H
#define TEST_SHARED_MUTEX_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <ctime>
#include "../netmmo/shared_mutex.h"

static std::string str("Pusto");
static std::shared_mutex mut;
static std::set<std::string> result;
static std::atomic<int> num_io(0);

static std::atomic<bool> is_run_progress(true);

void writeXyulo()
{
    //std::string str;
    for( int i = 0; i < 10000; i++ )
    {
        mut.lock();
        str = std::string("X");
        str += std::string("y");
        str += std::string("u");
        str += std::string("l");
        str += std::string("o");
        mut.unlock();
        
        num_io++;
    }
}

void writeBlyad()
{
    //std::string str;
    for( int i = 0; i < 10000; i++ )
    {
        mut.lock();
        str = std::string("B");
        str += std::string("l");
        str += std::string("y");
        str += std::string("a");
        str += std::string("d");
        mut.unlock();
        
        num_io++;
    }
}

void reader()
{
    //std::string str("Gusto");
    //std::set<std::string> result;
    for( int i = 0; i < 10000; i++ )
    {
        mut.lock_shared();
        result.insert(str);
        mut.unlock_shared();
        
        num_io++;
    }
}

void progress()
{
    while( is_run_progress )
    {
        //std::cout << "num io=" << num_io << std::endl;
    }
}

std::mutex notify;
std::condition_variable cond_var;

void test_sh_mutex()
{
    //std::unique_lock<std::mutex> lk( notify );
    //cond_var.wait(lk);
    
    std::cout << "Starting test..." << std::endl;
    
    std::vector<std::thread> ths;
    
    std::clock_t start;
    double duration;
    start = std::clock();
    
    std::thread pr_th( progress );
    
    for( int i = 0; i < 1000; i++ )
        ths.push_back( std::thread(reader) );
    for( int j = 0; j < 100; j++ )
        ths.push_back( std::thread(writeXyulo) );
    for( int k = 0; k < 100; k++ )
        ths.push_back( std::thread(writeBlyad) );
    for( int i = 0; i < ths.size(); i++ )
        ths[i].join();
    
    is_run_progress = false;
    pr_th.join();
    
    duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
    std::cout << "duration: " << duration << std::endl;
    
    for( auto it = result.begin(); it != result.end(); it++ )
    {
        for( int i = 0; i < (*it).size(); i++ )
            printf( "%d ", (*it).data()[i] );
        std::cout << *it << std::endl;
    }
}

#endif