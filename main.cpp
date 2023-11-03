#include <iostream>
#include "source/external/tlsf/tlsf.h"
int main()
{
    void * memory = malloc( 10 );
    tlsf_t tlsf_handle = tlsf_create_with_pool( memory, 10 );
    std::cout << "Hello world";
}