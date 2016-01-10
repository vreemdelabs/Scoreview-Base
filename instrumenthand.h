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

#define OUTOFRANGE -1

class CInstrumentHand
{
 public:
  CInstrumentHand();
  virtual ~CInstrumentHand();
  
  virtual int get_first_string(float f);
  virtual int get_next_string(float f, int string_number);
  virtual int get_prev_string(float f, int string_number);
  virtual std::string get_string_text(int string_number);
  virtual int get_proper_string(float f, int string_number);
  virtual int get_color(int string_number);
  virtual int get_string_num();
  virtual int get_string_abs_note(int string);

 protected:
  bool   is_in_range(float f);

 protected:
  Cf2n  m_f2n;
  float m_min_absnote;
  float m_max_absnote;
};

class CInstrumentHandPiano: public CInstrumentHand
{
 public:
  CInstrumentHandPiano();
  ~CInstrumentHandPiano();

  // Only hand selection
  int get_first_string(float f);
  int get_next_string(float f, int string_number);
  int get_prev_string(float f, int string_number);
  int get_proper_string(float f, int string_number);
  std::string get_string_text(int string_number);
  int get_color(int string_number);
  int get_string_num();
  int get_string_abs_note(int string);
};

class CInstrumentHandViolin: public CInstrumentHand
{
 public:
  CInstrumentHandViolin();
  ~CInstrumentHandViolin();

  int get_first_string(float f);
  int get_next_string(float f, int string_number);
  int get_prev_string(float f, int string_number);
  int get_proper_string(float f, int string_number);
  std::string get_string_text(int string_number);
  int get_color(int string_number);
  int get_string_num();
  int get_string_abs_note(int string);

 private:
  const int m_cnotes_per_string;
  const int m_cstring_num;
  float m_stringf[40];
};

class CInstrumentHandGuitar: public CInstrumentHand
{
 public:
  CInstrumentHandGuitar();
  ~CInstrumentHandGuitar();

  int get_first_string(float f);
  int get_next_string(float f, int string_number);
  int get_prev_string(float f, int string_number);
  int get_proper_string(float f, int string_number);
  std::string get_string_text(int string_number);
  int get_color(int string_number);
  int get_string_num();
  int get_string_abs_note(int string);

 private:
  const int m_cnotes_per_string;
  const int m_cstring_num;
  float     m_stringf[40];
};

class CInstrumentHandGuitarDropD: public CInstrumentHand
{
 public:
  CInstrumentHandGuitarDropD();
  ~CInstrumentHandGuitarDropD();

  int get_first_string(float f);
  int get_next_string(float f, int string_number);
  int get_prev_string(float f, int string_number);
  int get_proper_string(float f, int string_number);
  std::string get_string_text(int string_number);
  int get_color(int string_number);
  int get_string_num();
  int get_string_abs_note(int string);

 private:
  const int m_cnotes_per_string;
  const int m_cstring_num;
  float     m_stringf[40];
};
