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
#include <math.h>

#include "f2n.h"

Cf2n::Cf2n(float A, float frequency_ratio):
  m_A(A),
  m_f_ratio(frequency_ratio)
{
  float p;

  // B
  p = 59.;
  m_B = note2frequ(p);
  // C
  p = 48.;
  m_C = note2frequ(p);
  sprintf(m_noteleters[0], "C ");
  sprintf(m_noteleters[1], "C#");
  sprintf(m_noteleters[2], "D ");
  sprintf(m_noteleters[3], "Eb");
  sprintf(m_noteleters[4], "E ");
  sprintf(m_noteleters[5], "F ");
  sprintf(m_noteleters[6], "F#");
  sprintf(m_noteleters[7], "G ");
  sprintf(m_noteleters[8], "Ab");
  sprintf(m_noteleters[9], "A ");
  sprintf(m_noteleters[10], "Bb");
  sprintf(m_noteleters[11], "B ");
  //printf("C=%f A=%f B=%f\n", m_C, m_A, m_B);
  //for (int i = 0; i < 12; i++)
  //  printf("i=%d f=%f\n", i, note2frequ(i));
}

// A4 = 4 x 12 + 9 = 57  C4 = 4 x 12
int Cf2n::frequ2note(float f)
{
  // From wikipedia pitch
  return (int)round(57. + 12. * log2(f / m_A));
}

float Cf2n::note2frequ(int absnote)
{
  // Given note number p = 57 + 12 x log2(f / 440)
  // f = 440 x pow(2, (p - 57) / 12)
  return (m_A * pow(2., ((float)absnote - 57.) / 12.));
}

// 8 octaves
// 12 notes begining with C/do ending with B
// m_A must be la3 / A4, octave = 4
void Cf2n::convert(float f, int *poctave, int *pnote, float *error)
{
  int   i;
  float lo;
  float hi;
  float notef;
  float C;
  float B;
  int   note;

  *poctave = 0;
  *pnote = -1;
  //po = pow(m_f_ratio, 4);
  //C = m_C / po;
  //B = m_B / po;
  //printf("loref=%f hiref=%f\n", C, B);
  for (i = 0; i <= 8; i++) // All piano octaves
    {
      // Low boundary
      C = fabs(note2frequ(i * 12));
      lo = C / TWENTYFOURTH_ROOT_OF_2C;
      // Hight boundary
      B = fabs(note2frequ(i * 12 + 11));
      hi = B * TWENTYFOURTH_ROOT_OF_2C;
      // C 2 Cd 2 D 2 Dd 2 E 2 F 2 Fd 2 G 2 Gd 2 A 2 Bb 2 B 2
      // C 2 D 2 E 1 F 2 G 2 A 2 B 1
      if (lo <= f && f < hi)
	{	  
	  //printf("i=%d f=%f\n", i, f);
	  *poctave = i;
	  note = round(frequ2note(f));
	  if (note > notenum(8, 0))
	    return ;
	  *pnote = note - frequ2note(C);
#ifdef DEBUG_ROUND_NOTE
	  if (*pnote == -1)
	    {
	      printf("---- f=%f\n", f);
	      printf("C=%f B=%f freqn=%f roundfreq=%f\n", C, B, frequ2note(C), round(note2frequ(frequ2note(C))));
	      printf("lo=%f hi=%f note=%d octavenote=%d\n", lo, hi, note, *pnote);
	    }
#endif
	  notef = note2frequ(note);
	  if ((f - notef) < 0)
	    {
	      *error = (f - notef) / fabs(notef - note2frequ(note - 1));
	    }
	  else
	    {
	      *error = (f - notef) / fabs(note2frequ(note + 1) - notef);
	    }
	  break;
	}
    }
}

// TODO + French notation
void Cf2n::notename(int octave, int note, char *str, int buffersize)
{
  if (note < 0 || octave < 0)
    snprintf(str, buffersize, "--");
  snprintf(str, buffersize, "%s%d", m_noteleters[note % 12], octave);
}

// Returns a note number from C0 = 0 to Bd8 = (8 * 8) = 64
int Cf2n::notenum(int octave, int note)
{
  return (octave * 12) + note;
}

// Returns the octave and note from a 0 to 12 * 8 note
void Cf2n::noteoctave(int absnote, int *note, int *octave)
{
  *note = absnote % 12;
  *octave = absnote / 12;
}

float Cf2n::note2frequ(int note, int octave)
{
  // note in (0, 12)
  note = notenum(octave, note);
  return note2frequ(note);
}

void Cf2n::note_frequ_boundary(float *pflo, float *pfhi, int note, int octave)
{
  float f;

  f = note2frequ(note, octave);
  *pfhi = f * TWENTYFOURTH_ROOT_OF_2C;
  *pflo = f / TWENTYFOURTH_ROOT_OF_2C;
}

void Cf2n::absnote_frequ_boundary(float *pflo, float *pfhi, int absnote)
{
  int   note, octave;

  noteoctave(absnote, &note, &octave);
  note_frequ_boundary(pflo, pfhi, note, octave);
}

void Cf2n::note_harmonics(int absnote, t_note_harmonic *ph, const int harmonic_number)
{
  float           absnotef;
  t_note_harmonic h;
  int             i;
  int             note;
  int             octave;

  absnotef = note2frequ(absnote);
  for (i = 0; i < harmonic_number; i++)
    {
      h.mult = i + 1;
      h.harmonicf = absnotef * (float)h.mult;
      h.note = frequ2note(h.harmonicf);
      if (h.note < 0)
	{
	  printf("error note=%d hamonic=%d, absnote=%d absnotef=%f\n", h.note, i, absnote, h.harmonicf);
	}
      h.notef = note2frequ(h.harmonicf);
      h.error = h.harmonicf - h.notef;
      noteoctave(absnote, &note, &octave);
      note_frequ_boundary(&h.flo, &h.fhi, note, octave);
      ph[i] = h;
    }
}

// Returns a piano note number (-1 if not a piano note)
int Cf2n::pianonotenum(int octave, int note)
{
  int n;

  n = (octave * 12.);
  n += note;
  n -= 9;
  return n;
}

float Cf2n::f2nearest_note(float f)
{
  int   octave, note;
  float error;

  convert(f, &octave, &note, &error);
  return note2frequ(note, octave);
}
