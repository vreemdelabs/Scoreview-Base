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
typedef struct s_kparam
{
  // Build time
  int          sampling_frequency;
  // Changes depending on space or time resolution
  int          N;
  // Used when recording to transform near the end of the track while it is recorded
  int          tracksize;
  // Changes depending on zoom factor
  float        minfreq;
  float        maxfreq;
  int          start_sample;
  int          stop_sample;
}              t_kparam;

float find_pixel_column_sample(int xpos, int width, __global t_kparam *sp)
{
  int   total_samples;
  int   pixel_sample;

  total_samples = sp->stop_sample - sp->start_sample;
  pixel_sample = (int)floor(((float)total_samples * (float)xpos) / (float)width);
  return sp->start_sample + pixel_sample;
}

// Number of samples inside the discrete transform interval
float get_T(__global t_kparam *sp, int N, int sampling_frequency)
{
  float T;

  T = ((float)N / (float)sampling_frequency);
  return T;
}

float get_slot_freq_from_pixel(int y, int height, __global t_kparam *sp)
{
  return (sp->minfreq + ((sp->maxfreq - sp->minfreq) / (float)height) * (float)y);
}

// Finds the k factor closer to f in the DFT of N samples
// f(1) = 1 / T   f(k) = (1 / T) x k
// f = k / T    k = f * T
int get_k(float fk, float T, int N)
{
  int k;

  k = floor(fk * T);
  if (k > N || k < 0)
    k = 0;
  return (k);
}

float hann(float i, int nn)
{
  return (0.5 * (1.0 - cos(2.0 * M_PI_F * i / (float)(nn - 1))));
}

float Aweight(float f)
{
  float f2, f4;
  float weight;

  if (f <= 0.)
    return -1.0e+32;
  f2 = pow(f, 2);
  f4 = pow(f2, 2);
  weight = 12200 * 12200 * f4;
  weight /= (f2 + 12200 * 12200) * (f2 * 20.6 * 20.6) * sqrt((f2 + 107.7 * 107.7) * (f2 + 737.9 * 737.9));
  return weight;
}

//#define BASIC_DFT
#ifdef BASIC_DFT
float frequency_strength(__global float *psamples, int N, int k, __global float *window)
{
  int   n;
  float out;
  __global float *X;
  float Xn;
  float O;
  float cosO, sinO;
  float Real, Im;

  // One k slot of a dft
  O = (2. * M_PI * (float)k) / (float)N;
  X = psamples;
  Real = 0.;
  Im = 0.;
  for (n = 0; n < N; n++)
    {
      cosO = cos(O * (float)n);
      sinO = sin(O * (float)n);
      //Xn = X[n] * hann(n, N);
      Xn = X[n] * window[n];
      Real += Xn * cosO;
      Im   += Xn * sinO;
    }
  out = sqrt(Real * Real + Im * Im);
  return out;
}
#else
float frequency_strength(__global float *psamples, int N, int k, __global float *window)
{
  int   n;
  float O;
  float2 res;
  float4 Xn;
  float4 hannv;
  float4 wave;
  float4 waven;
  float4 waveReal;
  float4 waveIm;
  float  cosO, sinO;

  // One k slot of a dft
  O = (2. * M_PI_F * (float)k) / (float)N;
  res = (float2)(0., 0.);
  wave =  (float4)(O, O, O, O);
  waven = (float4)(0, 1, 2, 3);
  for (n = 0; n < N - 4; n += 4)
    {
      waveIm = sincos(wave * waven, &waveReal);
      Xn = vload4(n / 4, psamples);
#define HANN
#ifdef HANN
      hannv = vload4(n / 4, window);
      Xn = Xn * hannv;
#endif
      res.x += dot(Xn, waveReal);
      res.y += dot(Xn, waveIm);
      waven += (float4)(4, 4, 4, 4);
    }
  for (; n < N; n++)
    {
      cosO = cos(O * (float)n);
      sinO = sin(O * (float)n);
#ifdef HANN
      Xn.x = psamples[n] * window[n];
#endif
      res.x += Xn.x * cosO;
      res.y += Xn.x * sinO;
    }
  return sqrt(dot(res, res));
}
#endif

// Hann preloaded local 15ms
// Hann calc  75ms

#define LAST_SAMPLES 64

// This kernel gives the spectrometer image from a segment of a sample to x columns of y frequencies from fbase to fmax
__kernel void dft(__global float *s, __global t_kparam *sp, __global float *window, __global float *spectre)
{
  int           sampleindex;
  int           xpos, ypos;
  int           N;
  float         T;
  int           line_size;
  int           colu_size;
  float         f;
  int           k;
  float         fv;
  int           dest;

  // Height in pixels of the spectrum
  colu_size = get_global_size(0);
  // Get the column index
  // the frequency depends on the needed pixel
  ypos = get_global_id(0);
  // Get the line index (time of the analysis), one work-group per line, samples are read once for every work-item
  // Number of pixels for a line of spectrum analysis
  line_size = get_global_size(1);
  xpos = get_global_id(1);
  sampleindex = find_pixel_column_sample(xpos, line_size, sp);
  // Samples used
  if (sampleindex >= sp->tracksize - LAST_SAMPLES)
    {
      fv = 0.; // Pass the end of the track after LAST_SAMPLES samples
      dest = xpos * colu_size + ypos;
      spectre[dest] = fv;
    }
  else
    {
      if (sp->tracksize - sampleindex > sp->N)
	N = sp->N;
      else
	N = sp->tracksize - sampleindex; // Not enough samples, close to the end of the recording
      T = get_T(sp, N, sp->sampling_frequency);
      // Frequency for this work item
      f = get_slot_freq_from_pixel(ypos, colu_size, sp);
      k = get_k(f, T, N);
      // And now the dft
      fv = frequency_strength(s + sampleindex - (N / 2), N, k, window);
      // Write the result on consecutive y/frequ values
      dest = xpos * colu_size + ypos;
      spectre[dest] = fv * Aweight(f);
      //spectre[dest] = 20 * log(fv);
      //spectre[dest] = 2. + 20 * log(fv * Aweight(f));
    }
}

