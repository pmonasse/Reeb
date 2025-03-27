// Find primitive saddle values that are integers.
// If a saddle value can be obtained by affine transform  px+q with p,q>0 of all
// four coefficients from an integer saddle value, it is not primitive.

#include "saddle.h"
#include <vector>
#include <algorithm>

int main() {
    int primes[] = {2,3,5,7,11,13,17,19,23,29,31,37,41,43,47,
                    53,59,61,67,71,73,79,83,89,97,
                    101,103,107,109,113,127};
    const int n=sizeof(primes)/sizeof(primes[0]);
    std::vector<Saddle> V;
    for(int a=0; a<256; a++)
        for(int d=0; d<=a; d++)
            for(int b=0; b<d; b++) {
                int c = 0;
                int num = a*d-b*c;
                int denom = a+d-b-c;
                if(num%denom == 0) {
                    int i=0;
                    for(; i<n; i++) {
                        int p = primes[i];
                        if(a%p==0 && b%p==0 && c%p==0 && d%p==0 &&
                           (num/denom)%p==0)
                            break;
                    }
                    if(i==n)
                        V.push_back(Saddle(a,d,b,c));
                }
            }
    std::sort(V.begin(),V.end());
    for(std::vector<Saddle>::const_iterator it=V.begin(); it!=V.end(); ++it)
        std::cout << *it << std::endl;
    return 0;
}
