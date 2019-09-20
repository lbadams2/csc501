#include <conf.h>
#include <kernel.h>
#include <stdio.h>
#include <math.h>

volatile int acount = 0;
volatile int bcount = 0;
volatile int ccount = 0;

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
    for(i = 1; i < 21; i++) {
        if(i % 2 == 0)
            res -= pow((x - 1), i)/i;
        else
            res += pow((x - 1), i)/i;
    }
    return res;
}

double expdev(double lambda) {
    double dummy;
    do
        dummy= (double) rand() / RAND_MAX;
    while (dummy == 0.0);
    //kprintf("dummy is %d\n", (int)dummy*10);
    //double test = -log(dummy);
    //kprintf("log is %d\n", (int)test);
    double val = -log(dummy) / lambda;
    if(val <= 10)
        acount++;
    else if(val <= 20)
        bcount++;
    else
        ccount++;
    return val;
}
