#include <iostream>
#include <threadpool.hpp>
template <typename T>
decltype (auto) gcd(T a, T b) {
    T r;
    while(b!=0){
        r = a % b;
        a = b;
        b = r;
    }
    return a;
  }

int main () {
}
