#include <conf.h>
#include <kernel.h>
#include <stdio.h>
#include <math.h>

double pow(double x, int y) {
    if(y == 0)
        return 1;
    else if(y % 2 == 0)
        return pow(x, y/2)*pow(x, y/2);
    else
        return x*pow(x, y/2)*pow(x, y/2);
}

double log(double x) {
    int i;
    double res = 0;
    for(i = 1; i < 20; i++) {
        if(i % 2 == 0)
            res -= pow((x - 1), i)/i;
        else
            res += pow((x - 1), i)/i;
    }
    return res / LN_10;
}

double expdev(double lambda) {
    double dummy;
    do
        dummy= (double) rand() / RAND_MAX;
    while (dummy == 0.0);
    return -log(dummy) / lambda;
}