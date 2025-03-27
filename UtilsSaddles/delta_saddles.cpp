// Find min delta between saddle values

#include "saddle.h"
#include <vector>
#include <algorithm>
#include <cmath>

// V being ordered, find the minimal positive difference between two elements.
double min_delta(const std::vector<Saddle>& V, Saddle& s1, Saddle& s2) {
    double min=1;
    std::vector<Saddle>::const_iterator it=V.begin(), itp=it++;
    while(it!=V.end()) {
        double d = double(*it)-double(*itp);
        if(d>0 && d<min) {
            min = d;
            s1=*it; s2=*itp;
        }
        itp=it++;
    }
    return min;
}

int main() {
    // Step 1: generate only saddle values with c=0
    std::vector<Saddle> V1;
    for(int a=0; a<256; a++)
        for(int d=0; d<=a; d++)
            for(int b=0; b<d; b++) {
                Saddle s(a,d,b);
                V1.push_back(s);
            }
    std::sort(V1.begin(), V1.end());
    Saddle s1, s2;
    double min = min_delta(V1, s1, s2);
    std::cout << "First phase min delta (with c=0): " << min << std::endl
              << s1 << std::endl << s2 << std::endl;

    // min value of denominator to have a lower delta
    int min_den = (int)ceil(1/min/(255+255));
    std::cout << "Minimal denominator: " << min_den << std::endl;

    // Step 2: generate all saddle values with sufficient denominator
    std::vector<Saddle> V2;
    for(int a=0; a<256; a++)
        for(int d=0; d<=a; d++) {
            if(a+d<min_den)
                continue;
            for(int b=0; b<d; b++) {
                if(a+d-b<min_den)
                    continue;
                for(int c=0; c<=b; c++) {
                    if(a+d-b-c<min_den)
                        continue;
                    V2.push_back(Saddle(a,d,b,c));
                }
            }
        }
    std::sort(V2.begin(), V2.end());

    // Merge the two sorted vectors, V1 and V2
    std::vector<Saddle> V(V1.size()+V2.size());
    std::vector<Saddle>::const_iterator it=
        std::set_union(V1.begin(), V1.end(), V2.begin(), V2.end(), V.begin());
    V.erase(it, V.end());

    // Final result
    min = min_delta(V, s1, s2);
    std::cout << "Final min delta: " << min << std::endl
              << s1 << std::endl << s2 << std::endl;

    return 0;
}
