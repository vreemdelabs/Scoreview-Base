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
#include <stdlib.h>
#include <string>

#include "f2n.h"
#include "stringcolor.h"
#include "instrumenthand.h"

using namespace std;

// Only one hand, one string, used for trumpet or whatever
CInstrumentHand::CInstrumentHand():
  m_min_absnote(0),
  m_max_absnote(10000)
{
}

CInstrumentHand::~CInstrumentHand()
{
}
  
int CInstrumentHand::get_first_string(float f)
{
  return 0;
}

int CInstrumentHand::get_next_string(float f, int string_number)
{
  return -1;  
}

int CInstrumentHand::get_prev_string(float f, int string_number)
{
  return 0;
}

int CInstrumentHand::get_proper_string(float f, int string_number)
{
  return 0;
}

string CInstrumentHand::get_string_text(int string_number)
{
  return (string(""));
}

int CInstrumentHand::get_color(int string_number)
{
  return NOTE_COLOR;
}

bool CInstrumentHand::is_in_range(float f)
{
  float err;

  err = 0.4;
  return (f < m_max_absnote + err && f > m_min_absnote - err);
}

int CInstrumentHand::get_string_num()
{
  return 0;
}

int CInstrumentHand::get_string_abs_note(int string)
{
  return 0;
}
    
//-----------------------------------------------------------------------------------------------------
    
CInstrumentHandPiano::CInstrumentHandPiano()
{
  int octave;
  int note;

  octave = 0;
  note = 9;
  m_min_absnote = m_f2n.note2frequ(note, octave);
  octave = 8;
  note = 0;
  m_max_absnote = m_f2n.note2frequ(note, octave);
}

CInstrumentHandPiano::~CInstrumentHandPiano()
{
}

// Only hand selection, left right
int CInstrumentHandPiano::get_first_string(float f)
{
  int note;
  int octave;

  if (!is_in_range(f))
    return -1;
  octave = 3;
  note = 11;
  if (f > m_f2n.note2frequ(note, octave))
    return 1; // right
  return 0;   // left
}

// Just change hand
int CInstrumentHandPiano::get_next_string(float f, int string_number)
{
  if (!is_in_range(f))
    return -1;
  return (string_number == 1? 0 : 1);
}

int CInstrumentHandPiano::get_prev_string(float f, int string_number)
{
  if (!is_in_range(f))
    return -1;
  return get_next_string(f, string_number);
}

int CInstrumentHandPiano::get_proper_string(float f, int string_number)
{
  if (!is_in_range(f))
    return -1;
  if (string_number == -1)
    {
      string_number = get_first_string(f);
    }
  return string_number;
}

string CInstrumentHandPiano::get_string_text(int string_number)
{
  switch (string_number)
    {
    case 0:
      return string("left hand");
    case 1:
      return string("right hand");
    default:
      break;
    };
  return string("Out of range");
}

int CInstrumentHandPiano::get_color(int string_number)
{
  return string_number == 0? STRING0_COLOR : STRING1_COLOR;
}

int CInstrumentHandPiano::get_string_num()
{
  return 0;
}

int CInstrumentHandPiano::get_string_abs_note(int string)
{
  return 0;
}

//-----------------------------------------------------------------------------------------------------
// Violin string selection, the real shit

CInstrumentHandViolin::CInstrumentHandViolin():
  m_cnotes_per_string(30),
  m_cstring_num(4)
{
  int note, octave;

  octave = 8;
  note = 0;
  m_max_absnote = m_f2n.note2frequ(note, octave);
  // Mi
  octave = 5;
  note = 4;
  m_stringf[0] = m_f2n.note2frequ(note,  octave);
  // La
  octave = 4;
  note = 9;
  m_stringf[1] = m_f2n.note2frequ(note,  octave);
  // Ré
  octave = 4;
  note = 2;
  m_stringf[2] = m_f2n.note2frequ(note,  octave);  
  // Sol
  octave = 3;
  note = 7;
  m_stringf[3] = m_f2n.note2frequ(note,  octave);
  m_min_absnote = m_stringf[3];
}

CInstrumentHandViolin::~CInstrumentHandViolin()
{
}

// The lowest string available for the note
int CInstrumentHandViolin::get_first_string(float f)
{
  int i;

  if (!is_in_range(f))
    return -1;
  for (i = 0; i < m_cstring_num; i++)
    {
      if (m_stringf[i] < f + 0.4)
	return i;
    }
  return -1;
}

int CInstrumentHandViolin::get_next_string(float f, int string_number)
{
  int string_absnote;
  int absnote;

  if (!is_in_range(f))
    return -1;
  if (string_number < 0 || string_number >= m_cstring_num)
    return string_number;
  string_absnote = m_f2n.frequ2note(m_stringf[string_number + 1]);
  absnote = m_f2n.frequ2note(f);
  if (abs(absnote - string_absnote) < m_cnotes_per_string) // Check if is not to high on the string
    return string_number + 1;
  return string_number;
}

int CInstrumentHandViolin::get_prev_string(float f, int string_number)
{
  if (!is_in_range(f))
    return -1;
  if (string_number <= 0)
      return 0;
  printf("compare string %d freq=%f with f=%f\n", string_number - 1, m_stringf[string_number - 1], f + 0.4);
  if (m_stringf[string_number - 1] < f + 0.4)
    return string_number - 1;
  return string_number;
}

int CInstrumentHandViolin::get_proper_string(float f, int string_number)
{
  int string_absnote;
  int absnote;
  int diff;

  if (!is_in_range(f))
    return -1;
  if (string_number < 0 || string_number >= m_cstring_num)
    return get_first_string(f);
  // If f can be played on this string, keep it
  string_absnote = m_f2n.frequ2note(m_stringf[string_number]);
  absnote = m_f2n.frequ2note(f);
  diff = (absnote - string_absnote);
  if (diff < m_cnotes_per_string && diff >= 0) // Check if is not to high on the string
    return string_number;
  // Else return the first possible string
  return get_first_string(f);
}

string CInstrumentHandViolin::get_string_text(int string_number)
{
  if (string_number == 0)
    return string("String 1 E5");
  if (string_number == 1)
    return string("String 2");
  if (string_number == 2)
    return string("String 3");
  if (string_number == 3)
    return string("String 4");
  return string("Out of range");
}

int CInstrumentHandViolin::get_color(int string_number)
{
  int color;

  switch (string_number)
    {
    case 0:
      color = STRING0_COLOR;
      break;
    case 1:
      color = STRING1_COLOR;
      break;
    case 2:
      color = STRING2_COLOR;
      break;
    case 3:
      color = STRING3_COLOR;
      break;
    default:
      color = NOTE_COLOR;
      break;
    }
  return color;
}

int CInstrumentHandViolin::get_string_num()
{
  return m_cstring_num;
}

int CInstrumentHandViolin::get_string_abs_note(int string)
{
  float freq;
  int   absnote;

  if (string >= m_cstring_num)
    return 0;
  freq = m_stringf[string];
  absnote = m_f2n.frequ2note(freq);
  return absnote;
}

//-----------------------------------------------------------------------------------------------------
// Guitar, wohooo

CInstrumentHandGuitar::CInstrumentHandGuitar():
  m_cnotes_per_string(24),
  m_cstring_num(6)
{
  int note, octave;

  octave = 6;
  note = 4;
  m_max_absnote = m_f2n.note2frequ(note, octave);
  // Mi E4
  octave = 4;
  note = 4;
  m_stringf[0] = m_f2n.note2frequ(note,  octave);
  // Si B3
  octave = 3;
  note = 11;
  m_stringf[1] = m_f2n.note2frequ(note,  octave);
  // Sol G3
  octave = 3;
  note = 7;
  m_stringf[2] = m_f2n.note2frequ(note,  octave);  
  // Ré  D3
  octave = 3;
  note = 2;
  m_stringf[3] = m_f2n.note2frequ(note,  octave);
  m_min_absnote = m_stringf[3];
  // La  A2
  octave = 2;
  note = 9;
  m_stringf[4] = m_f2n.note2frequ(note,  octave);
  m_min_absnote = m_stringf[3];
  // Mi  E2
  octave = 2;
  note = 4;
  m_stringf[5] = m_f2n.note2frequ(note,  octave);
  m_min_absnote = m_stringf[5];
}

CInstrumentHandGuitar::~CInstrumentHandGuitar()
{
}

// The lowest string available for the note
int CInstrumentHandGuitar::get_first_string(float f)
{
  int i;

  if (!is_in_range(f))
    return -1;
  for (i = 0; i < m_cstring_num; i++)
    {
      if (m_stringf[i] < f + 0.4)
	return i;
    }
  return -1;
}

int CInstrumentHandGuitar::get_next_string(float f, int string_number)
{
  int string_absnote;
  int absnote;

  if (!is_in_range(f))
    return -1;
  if (string_number < 0 || string_number >= m_cstring_num)
    return string_number;
  string_absnote = m_f2n.frequ2note(m_stringf[string_number + 1]);
  absnote = m_f2n.frequ2note(f);
  if (abs(absnote - string_absnote) < m_cnotes_per_string) // Check if is not to high on the string
    return string_number + 1;
  return string_number;
}

int CInstrumentHandGuitar::get_prev_string(float f, int string_number)
{
  if (!is_in_range(f))
    return -1;
  if (string_number <= 0)
      return 0;
  printf("compare string %d freq=%f with f=%f\n", string_number - 1, m_stringf[string_number - 1], f + 0.4);
  if (m_stringf[string_number - 1] < f + 0.4)
    return string_number - 1;
  return string_number;
}

int CInstrumentHandGuitar::get_proper_string(float f, int string_number)
{
  int string_absnote;
  int absnote;
  int diff;

  if (!is_in_range(f))
    return -1;
  if (string_number < 0 || string_number >= m_cstring_num)
    return get_first_string(f);
  // If f can be played on this string, keep it
  string_absnote = m_f2n.frequ2note(m_stringf[string_number]);
  absnote = m_f2n.frequ2note(f);
  diff = (absnote - string_absnote);
  if (diff < m_cnotes_per_string && diff >= 0) // Check if is not to high on the string
    return string_number;
  // Else return the first possible string
  return get_first_string(f);
}

string CInstrumentHandGuitar::get_string_text(int string_number)
{
  if (string_number == 0)
    return string("string 1"); // E4
  if (string_number == 1)
    return string("string 2"); // B3
  if (string_number == 2)
    return string("string 3"); // G3
  if (string_number == 3)
    return string("string 4"); // D3
  if (string_number == 4)
    return string("string 5"); // A2
  if (string_number == 5)
    return string("string 6"); // E2
  return string("Out of range");
}

int CInstrumentHandGuitar::get_color(int string_number)
{
  int color;

  switch (string_number)
    {
    case 0:
      color = STRING0_COLOR;
      break;
    case 1:
      color = STRING1_COLOR;
      break;
    case 2:
      color = STRING2_COLOR;
      break;
    case 3:
      color = STRING3_COLOR;
      break;
    case 4:
      color = STRING4_COLOR;
      break;
    case 5:
      color = STRING5_COLOR;
      break;
    default:
      color = NOTE_COLOR;
      break;
    }
  return color;
}

int CInstrumentHandGuitar::get_string_num()
{
  return m_cstring_num;
}

int CInstrumentHandGuitar::get_string_abs_note(int string)
{
  float freq;
  int   absnote;

  if (string >= m_cstring_num)
    return 0;
  freq = m_stringf[string];
  absnote = m_f2n.frequ2note(freq);
  return absnote;
}

//-----------------------------------------------------------------------------------------------------
// Guitar, wohooo

CInstrumentHandGuitarDropD::CInstrumentHandGuitarDropD():
  m_cnotes_per_string(24),
  m_cstring_num(6)
{
  int note, octave;

  octave = 6;
  note = 4;
  m_max_absnote = m_f2n.note2frequ(note, octave);
  // Mi E4
  octave = 4;
  note = 4;
  m_stringf[0] = m_f2n.note2frequ(note,  octave);
  // Si B3
  octave = 3;
  note = 11;
  m_stringf[1] = m_f2n.note2frequ(note,  octave);
  // Sol G3
  octave = 3;
  note = 7;
  m_stringf[2] = m_f2n.note2frequ(note,  octave);  
  // Ré  D3
  octave = 3;
  note = 2;
  m_stringf[3] = m_f2n.note2frequ(note,  octave);
  m_min_absnote = m_stringf[3];
  // La  A2
  octave = 2;
  note = 9;
  m_stringf[4] = m_f2n.note2frequ(note,  octave);
  m_min_absnote = m_stringf[3];
  // Ré  D2
  octave = 2;
  note = 2;
  m_stringf[5] = m_f2n.note2frequ(note,  octave);
  m_min_absnote = m_stringf[5];
}

CInstrumentHandGuitarDropD::~CInstrumentHandGuitarDropD()
{
}

// The lowest string available for the note
int CInstrumentHandGuitarDropD::get_first_string(float f)
{
  int i;

  if (!is_in_range(f))
    return -1;
  for (i = 0; i < m_cstring_num; i++)
    {
      if (m_stringf[i] < f + 0.4)
	return i;
    }
  return -1;
}

int CInstrumentHandGuitarDropD::get_next_string(float f, int string_number)
{
  int string_absnote;
  int absnote;

  if (!is_in_range(f))
    return -1;
  if (string_number < 0 || string_number >= m_cstring_num)
    return string_number;
  string_absnote = m_f2n.frequ2note(m_stringf[string_number + 1]);
  absnote = m_f2n.frequ2note(f);
  if (abs(absnote - string_absnote) < m_cnotes_per_string) // Check if is not to high on the string
    return string_number + 1;
  return string_number;
}

int CInstrumentHandGuitarDropD::get_prev_string(float f, int string_number)
{
  if (!is_in_range(f))
    return -1;
  if (string_number <= 0)
      return 0;
  //printf("compare string %d freq=%f with f=%f\n", string_number - 1, m_stringf[string_number - 1], f + 0.4);
  if (m_stringf[string_number - 1] < f + 0.4)
    return string_number - 1;
  return string_number;
}

int CInstrumentHandGuitarDropD::get_proper_string(float f, int string_number)
{
  int string_absnote;
  int absnote;
  int diff;

  if (!is_in_range(f))
    return -1;
  if (string_number < 0 || string_number >= m_cstring_num)
    return get_first_string(f);
  // If f can be played on this string, keep it
  string_absnote = m_f2n.frequ2note(m_stringf[string_number]);
  absnote = m_f2n.frequ2note(f);
  diff = (absnote - string_absnote);
  if (diff < m_cnotes_per_string && diff >= 0) // Check if is not to high on the string
    return string_number;
  // Else return the first possible string
  return get_first_string(f);
}

string CInstrumentHandGuitarDropD::get_string_text(int string_number)
{
  if (string_number == 0)
    return string("string 1"); // E4
  if (string_number == 1)
    return string("string 2"); // B3
  if (string_number == 2)
    return string("string 3"); // G3
  if (string_number == 3)
    return string("string 4"); // D3
  if (string_number == 4)
    return string("string 5"); // A2
  if (string_number == 5)
    return string("string 6"); // D2
  return string("Out of range");
}

int CInstrumentHandGuitarDropD::get_color(int string_number)
{
  int color;

  switch (string_number)
    {
    case 0:
      color = STRING0_COLOR;
      break;
    case 1:
      color = STRING1_COLOR;
      break;
    case 2:
      color = STRING2_COLOR;
      break;
    case 3:
      color = STRING3_COLOR;
      break;
    case 4:
      color = STRING4_COLOR;
      break;
    case 5:
      color = STRING5_COLOR;
      break;
    default:
      color = NOTE_COLOR;
      break;
    }
  return color;
}

int CInstrumentHandGuitarDropD::get_string_num()
{
  return m_cstring_num;
}

int CInstrumentHandGuitarDropD::get_string_abs_note(int string)
{
  float freq;
  int   absnote;

  if (string >= m_cstring_num)
    return 0;
  freq = m_stringf[string];
  absnote = m_f2n.frequ2note(freq);
  return absnote;
}

