/*
 Scoreview (R)
 Copyright (C) 2015 Patrick Areny
 All Rights Reserved.

 Scoreview is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <math.h>

double hann(double i, int nn)
{
  return (0.5 * (1.0 - cos(2.0 * M_PI * i / (double)(nn - 1))));
}

void apply_hann_window(int samples, double *pin)
{
  int s;

  for (s = 0; s < samples; s++)
    pin[s] = pin[s] * hann(s, samples);
}

void fapply_hann_window(int samples, float *pin)
{
  int s;

  for (s = 0; s < samples; s++)
    pin[s] = pin[s] * hann(s, samples);
}

float get_hann_integral(int N)
{
  int   n;
  float sum;

  for (n = 0, sum = 0; n < N; n++)
    {
      sum += 1. * hann(n, N);
    }
  return (sum);
}
