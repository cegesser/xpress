

#include "xpress.h"

#include <iostream>

int main() {

    auto func0 = xpress::parse("4 ^ 2 ^ -1");
   // auto func0 = xpress::parse("2 + 3 * 5");
    auto func1 = xpress::parse("2 + 3 * x", "x");

    auto func2 = xpress::parse("x ^ y", "x", "y");

    double d0 = func0();
    double d1 = func1(25);
    double d2 = func2(2, 3);

    std::cout << d0 << " " << d1 << " " << d2 << std::endl;

    //std::string expr = "255.0 / ( 1.0 + exp( (-12 / 65536.0)*(x-(65536.0/2)) ) )";
    //std::string expr = "255 - 255 * log( 1 + ( ( (65535 - x) / 65535.0) * 9) )";

}
