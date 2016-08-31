#ifndef MATH_HPP
#define MATH_HPP

template <typename T>
decltype (auto) gcd(T a, T b) {
    T r;
    while(b!=0) {
        r = a % b;
        a = b;
        b = r;
    }
    return a;
}

#endif // MATH_HPP
