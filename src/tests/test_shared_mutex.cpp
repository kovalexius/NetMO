#include "test_shared_mutex.h"

int main()
{
    test_sh_mutex();
    
    std::cout << "Press any key" << std::endl;
    std::cin.get();
    
    return 0;
}