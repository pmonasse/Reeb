#ifndef SADDLE_H
#define SADDLE_H
#include <iostream>

struct Saddle {
    int a,b,c,d;
    Saddle() { a=b=c=d=0; }
    Saddle(int a, int d, int b, int c=0): a(a), d(d), b(b), c(c) {}
    int num() const { return (a*d-b*c); }
    int denom() const { return (a+d-b-c); }
    operator double() const { return num()/double(denom()); }
    bool operator<(const Saddle& s) const;
};
std::ostream& operator<<(std::ostream& str, const Saddle& s);

#endif
