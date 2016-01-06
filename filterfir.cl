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
typedef struct s_fir_params
{
  int          intracksize;
  int          decimation;
}              t_fir_params;

#define FIR_SIZE 516 // 4 * 129 for float4 in opencl
#define HALF_FIR_SIZE 256

// |FIR_SIZE / 2 samples| T | samples | T |FIR_SIZE / 2 samples|
//#define USE_FLOAT4
#ifdef USE_FLOAT4
__kernel void fir(__global float *s, __global float *sfiltered, __global t_fir_params *param, __global float4 *gcoef)
{
  int intracksize;
  int sampleout;
  int samplein;
  int step;
  int i;
  float  result;
  float4 in;

  intracksize = param->intracksize;
  sampleout = get_global_id(0);
  samplein = sampleout * param->decimation;
  result = 0;
  for (step = 0; step < FIR_SIZE; step += 4)
    {
      i = samplein - HALF_FIR_SIZE + step;
      if (i >= 0 && i < intracksize - 4)
	{
	  in = vload4(i / 4, s);
	  result += dot(gcoef[step / 4], in);
	}
    }
  if (sampleout < param->intracksize / param->decimation)
    sfiltered[sampleout] = result;
}
#else
__kernel void fir(__global float *s, __global float *sfiltered, __global t_fir_params *param, __global float *gcoef)
{
  int   intracksize;
  int   sampleout;
  int   samplein;
  int   step;
  int   i;
  float result;

  intracksize = param->intracksize;
  sampleout = get_global_id(0);
  samplein = sampleout * param->decimation;
  result = 0;
  for (step = 0; step < FIR_SIZE; step++)
    {
      i = samplein - HALF_FIR_SIZE + step;
      if (i >= 0 && i < intracksize)
	{
	  result += gcoef[step] * s[i];
	}
    }
  if (sampleout < param->intracksize / param->decimation)
    sfiltered[sampleout] = result;
}
#endif

