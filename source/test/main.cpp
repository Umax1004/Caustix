#include <iostream>
#include <vector>

import Foundation.Assert;
import Foundation.Memory.Allocators.Allocator;
import Foundation.Memory.Allocators.LinearAllocator;

using StringBuffer = std::basic_string<char, std::char_traits<char>, Caustix::STLAdaptor<char>>;

int main( int argc, char** argv ) {
    int i = 0;
    Caustix::CASSERT(true);
    Caustix::LinearAllocator allocator(1024 * 1024);
    std::vector<int, Caustix::STLAdaptor<int>> thing(allocator);
    thing.reserve(6);
    thing.push_back(1);
    thing.push_back(2);
    thing.push_back(3);
    thing.push_back(4);
    thing.push_back(5);
    thing.push_back(6);
//    check check(allocator);
    StringBuffer string("Hello World!",allocator);
    return i;
}