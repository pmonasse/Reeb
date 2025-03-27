// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file saddle.cpp
 * @brief A saddle point inside a sample square.
 * 
 * (C) 2025 Pascal Monasse <pascal.monasse@enpc.fr>
 */

#include "saddle.h"

std::ostream& operator<<(std::ostream& str, const Saddle& s) {
    return str << s.a << ',' << s.b << ';' << s.c << ',' << s.d
               << ". " << s.num() << '/' << s.denom()
               << ": " << double(s);
}

bool Saddle::operator<(const Saddle& s) const {
    int v=denom()*s.num()-num()*s.denom();
    if(v!=0) return v>0;
    if(a!=s.a) return a<s.a;
    if(d!=s.d) return d<s.d;
    if(b!=s.b) return b<s.b;
    return c<s.c;
}
