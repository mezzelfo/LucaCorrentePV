#ifndef LOWPASSFILTER
#define LOWPASSFILTER

#define FILTERLEN 16
// Coefficienti magici creati con Mathematica
const double magicCoeff[] = { 0.00974767, 0.0145182, 0.0278979, 0.0479747, 
                              0.071439, 0.0941983, 0.112165, 0.122059, 
                              0.122059, 0.112165, 0.0941983, 0.071439, 
                              0.0479747, 0.0278979, 0.0145182, 0.00974767
                           };

class LowPassFilter
{
  public:
    double values[FILTERLEN];
    LowPassFilter(double initialvalue);
    double filter(double inputvalue);
    void rotate();
};

#endif
