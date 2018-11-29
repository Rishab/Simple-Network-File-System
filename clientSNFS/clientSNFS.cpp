// We should only need these two headers for C++ stuff. Everything else can
// be in regular C.
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    // Examples of printing to stdout and the std::string class.
    // "std::" is basically the C++ equivalent of "System." in Java. So it
    // allows you to use anything from the C++ standard library (if you
    // included the header for it).

    
    // cout = print to stdout. endl = insert a newline character.
    std::cout << "Hello world" << std::endl; 

    // string = C++ string class. Works pretty much like Java strings.
    std::string hello = "Hello world";
    std::cout << hello + "\n";

    // You can also print C-style strings (char *). But you can't append
    // stuff with + when you use them.
    char *c_hello = "Hello";
    std::cout << c_hello << std::endl;

    return 0;
}
