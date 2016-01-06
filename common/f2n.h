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

#define TWELVTH_ROOT_OF_2  (1.0594630943592)
#define TWELVTH_ROOT_OF_2C (pow(2., 1. / 12.))
#define TWENTYFOURTH_ROOT_OF_2C (pow(2., 1. / 24.))

// 12 note frequencies per octave but 7 notes ABCDEFG
#define BAROQUE_NOTES_PER_OCTAVE 7

#define PIANO_NOTE_NUMBER   (88)

#define MINNOTEF   15.
#define MAXNOTEF 7920.

#define F_BASE     15.
#define F_MAX   20000

typedef struct s_note_harmonic
{
  int   mult;
  float harmonicf;
  int   note;
  float notef;
  float error;
  float flo;
  float fhi;
}              t_note_harmonic;

class Cf2n
{
 public:
  Cf2n(float A = 440, float frequency_ratio = 2);

  int frequ2note(float f);
  float note2frequ(int absnote);
  float note2frequ(int note, int octave);
  void convert(float f, int *poctave, int *pnote, float *error);
  void notename(int octave, int note, char *str, int buffersize);
  int notenum(int octave, int note);
  void noteoctave(int absnote, int *note, int *octave);
  void note_frequ_boundary(float *pflo, float *pfhi, int note, int octave);
  void absnote_frequ_boundary(float *pflo, float *pfhi, int absnote);
  void note_harmonics(int absnote, t_note_harmonic *ph, const int harmonic_number);
  int  pianonotenum(int octave, int note);
  float f2nearest_note(float f);

 private:
  float m_A;
  float m_f_ratio;
  float m_C;
  float m_B;
  char  m_noteleters[12][4];
};

