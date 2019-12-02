#include "LowPassFilter.h"

LowPassFilter::LowPassFilter(double initialvalue)
{
  for (int i = 0; i < FILTERLEN; i++)
  {
    values[i] = initialvalue;
  }
}

double LowPassFilter::filter(double inputvalue)
{
  rotate();
  values[FILTERLEN-1] = inputvalue;
  double filtered = 0.0;
  for (int i = 0; i < FILTERLEN; i++)
    filtered += magicCoeff[i] * values[i];
  return filtered;
}

void LowPassFilter::rotate()
{
  for (int i = 0; i < FILTERLEN-1; i++)
    values[i] = values[i+1];
  values[FILTERLEN-1] = 0.0;
}
