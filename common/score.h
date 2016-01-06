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


enum enotelen
  {
    ronde = 1,
    blanche,
    noire = 4,
    croche = 8,
    doublecroche = 16,
    triplecroche = 32,
    quadruplecroche = 64
  };

class CMesure
{
 public:
  CMesure(double start = -1, double stop = -1);
  CMesure(const CMesure& m);

  double duration() const;
  double start_time(int index) const; // Start of the nth measure interval

 public:
  int m_bpm;
  int m_times;
  double m_start;
  double m_stop;
};

typedef struct s_notefreq
{
  float        f;
  int          string;
  bool operator<(const s_notefreq &nf) const;
  bool operator==(const s_notefreq &nf) const;
}              t_notefreq;

class CNote
{
 public:
  CNote(double timecode = -1, double duration = -1, float f = -1, int string = 0);
  CNote(const CNote& note);
  ~CNote();
  bool is_frequ_in(float freq);
  int  identifier() const;
  void set_identifier(const int ident); // Only used for network exchange

  double m_time;
  double m_duration;
  bool   m_lconnected;
  std::list<t_notefreq> m_freq_list;

 private:
  int         m_ident;
  static int  m_ident_seed;
};

class CInstrument
{
 public:
  CInstrument(std::string name);
  ~CInstrument();

  void add_note(CNote *pn);
  void delete_note(CNote *pn);
  CNote* get_first_note(double timecode);
  CNote* get_next_note();
  CNote* get_first_note_reverse(double timecode);
  CNote* get_next_note_reverse();
  void   sort();
  const std::string get_name();
  void  set_identifier(int ident);
  const int identifier();
  const int note_count();

 private:
  static bool compare(const CNote *pfirst, const CNote *psecond);
 public:
  std::string                         m_name;
  std::list<CNote*>                   m_Note_list;
 private:
  std::list<CNote*>::iterator         m_iter;
  std::list<CNote*>::reverse_iterator m_riter;

  int         m_ident;
};

class CScore
{
 public:
  CScore();
  CScore(std::string instrument_name);
  ~CScore();

  CInstrument* get_instrument(std::string name, const int identifier);
  void add_instrument(CInstrument* pins);
  int  instrument_number();
  CInstrument* get_first_instrument();
  CInstrument* get_next_instrument();
  CInstrument* remove_instrument(CInstrument* pins);
  void add_measure(double start, double stop);
  void add_measure(CMesure *pm);
  void delete_measure(CMesure *pm);
  static bool compare_bars(const CMesure *pfirst, const CMesure *psecond);
  void sort_measures();
  
  std::list<CMesure*> m_mesure_list;
 private:
  std::list<CInstrument*>           m_Instrument_list;
  std::list<CInstrument*>::iterator m_iiter;
};

