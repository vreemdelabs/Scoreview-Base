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
#include <stdio.h>

float Bx(float s)
{
  float l = 1. / 8.;
  float a = 8. / 2.;
  float a1 = 8.;

  if (s >= 0. && s <= l)
    return (0.0 + s * a1);    // Changs from mathlab color function starts at 0 pitch black.
  if (s > l && s <= 2. * l)
    return (1.0);
  if (s > 2. * l && s < 3. * l)
    return (1. - (s - 2. * l) * a);
  if (s > 1.5)
    return 1.;
  return 0.;
}

float Gx(float s)
{
  float l = 1. / 8.;
  float a = 8. / 2.;

  if (s >= l && s <= 3. * l)
    return ((s - l) * a);
  if (s > 3. * l && s <= 5. * l)
    return (1.0);
  if (s > 5. * l && s < 7. * l)
    return (1. - (s - 5. * l) * a);
  if (s > 1.5)
    return 1.;
  return 0.;
}

float Rx(float s)
{
  float l = 1. / 8.;
  float a = 8. / 2.;

  if (s >= 3. * l && s <= 5. * l)
    return ((s - 3. * l) * a);
  if (s > 5. * l && s <= 7. * l)
    return (1.0);
  if (s > 7. * l && s < 8. * l)
    return (1. - (s - 7. * l) * a);
  if (s > 1.5)
    return 1.;
  return 0.;
}

// Normalises a value v in 0. and 1. and converts that to a rgb 565 short int
int value_to_color_565(float v, float max)
{
  int s;
  int R;
  int G;
  int B;

  float fact = max;

  v = v / fact;
  B = (int)31. * Bx(v);
  G = (int)63. * Gx(v);
  R = (int)31. * Rx(v);
  s = (R % 32) + ((G % 64) << 5) + ((B % 32) << 11);
//  if (v == 1.)
  //  s = 0xFE00;
  //printf("vla=%f, max=%f\n", v, max);
  return s;
}

// ABGR
int value_to_color_888(float v, float max)
{
  int s;
  int R;
  int G;
  int B;

  float fact = max;

  v = v / fact;
  B = (int)255. * Bx(v);
  G = (int)255. * Gx(v);
  R = (int)255. * Rx(v);
  s = (R & 0xFF) + ((G & 0xFF) << 8) + ((B & 0xFF) << 16);
  return s;
}
