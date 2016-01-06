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
#include <assert.h>

#include <iterator>
#include <list>

#include "score.h"

using namespace std;

CMesure::CMesure(double start, double stop):
  m_bpm(4),
  m_times(4)
{
  m_start = start < stop? start : stop;
  m_stop = start > stop? start : stop;
}

CMesure::CMesure(const CMesure& m)
{
  m_start = m.m_start;
  m_stop = m.m_stop;
  m_bpm = m.m_bpm;
  m_times = m.m_times;
}

double CMesure::duration() const
{
  return (m_stop - m_start);
}

double CMesure::start_time(int index) const
{
  return (m_start + (double)index * duration());
}

bool t_notefreq::operator<(const s_notefreq &nf) const
{
  return (f < nf.f);
}

bool t_notefreq::operator==(const s_notefreq &nf) const
{
  return (f == nf.f && string == nf.string);
}

int CNote::m_ident_seed = 1;

CNote::CNote(double timecode, double duration, float f, int string):
  m_time(timecode),
  m_duration(duration),
  m_lconnected(false)
{
  t_notefreq nf;

  m_freq_list.clear();
  nf.f = f;
  nf.string = string;
  //if (f != -1)
  m_freq_list.push_front(nf);
  m_ident = m_ident_seed++;
  //printf("new note, identifier=%d.\n", m_ident); // Uncomment that to check if identifiers do not grow continually
}

// Copy constructor
CNote::CNote(const CNote& note):
  m_time(note.m_time),
  m_duration(note.m_duration),
  m_lconnected(note.m_lconnected)
{
  m_freq_list.clear();
  m_freq_list = note.m_freq_list;
  m_ident = note.identifier();
}

CNote::~CNote()
{
  m_freq_list.clear();
}

bool CNote::is_frequ_in(float freq)
{
  std::list<t_notefreq>::iterator iter;

  iter = m_freq_list.begin();
  while (iter != m_freq_list.end())
    {
      assert((*iter).f != -1);
      if ((*iter).f == freq)
	return true;
      iter++;
    }
  return false;
}

int CNote::identifier() const
{
  return m_ident;
}

void CNote::set_identifier(const int ident)
{
  m_ident = ident;
}

CInstrument::CInstrument(string name)
{
  m_name = name;
  m_Note_list.clear();
}

CInstrument::~CInstrument()
{
  m_iter = m_Note_list.begin();
  while (m_iter != m_Note_list.end())
    {
      delete *m_iter;
      m_iter++;
    }
}

// Compares the timecodes
bool CInstrument::compare(const CNote *pfirst, const CNote *psecond)
{
  return (pfirst->m_time < psecond->m_time);
}

void CInstrument::sort()
{
  m_Note_list.sort(CInstrument::compare);
}

const string CInstrument::get_name()
{
  return m_name;
}

void CInstrument::set_identifier(int ident)
{
  m_ident = ident;
}

const int CInstrument::identifier()
{
  return m_ident;
}

const int CInstrument::note_count()
{
  return m_Note_list.size();
}

CScore::CScore()
{
  m_mesure_list.clear();
  m_Instrument_list.clear();
}

CScore::CScore(string instrument_name)
{
  CInstrument *pins;

  m_mesure_list.clear();
  m_Instrument_list.clear();
  pins = new CInstrument(instrument_name);
  add_instrument(pins);
}

CScore::~CScore()
{
  list<CInstrument*>::iterator it;
  std::list<CMesure*>::iterator mit;

  it = m_Instrument_list.begin();
  while (it != m_Instrument_list.end())
    {
      delete *it;
      it++;
    }
  mit = m_mesure_list.begin();
  while (mit != m_mesure_list.end())
    {
      delete *mit;
      mit++;
    }
}

CInstrument* CScore::get_instrument(string name, const int identifier)
{
  list<CInstrument*>::iterator iter;

  for (iter = m_Instrument_list.begin(); iter != m_Instrument_list.end(); iter++)
    {
      if ((*iter)->m_name.compare(name) == 0 && (*iter)->identifier() == identifier)
	{
	  return (*iter);
	}
    }
  return (NULL);
}

void CScore::add_instrument(CInstrument* pins)
{
  list<CInstrument*>::iterator iter;
  int                          ident;
  bool                         bfound;

  for (ident = 0; ident < 32; ident++) // Find a free indentifier and insert
    {
      bfound = false;
      for (iter = m_Instrument_list.begin(); iter != m_Instrument_list.end(); iter++)
	{
	  if ((*iter)->identifier() == ident)
	    {
	      bfound = true;
	      break;
	    }
	}
      if (!bfound)
	{
	  pins->set_identifier(ident);
	  m_Instrument_list.push_back(pins);
	  return;
	}
    }
}

int CScore::instrument_number()
{
  return m_Instrument_list.size();
}

CInstrument* CScore::get_first_instrument()
{
  m_iiter = m_Instrument_list.begin();
  if (m_iiter == m_Instrument_list.end())
    return NULL;
  return *m_iiter;
}

CInstrument* CScore::get_next_instrument()
{
  m_iiter++;
  if (m_iiter == m_Instrument_list.end())
    return NULL;
  return *m_iiter;
}

CInstrument* CScore::remove_instrument(CInstrument* pins)
{
  list<CInstrument*>::iterator iter;

  iter = m_Instrument_list.begin();
  while (iter != m_Instrument_list.end())
    {
      if ((*iter) == pins)
	{
	  iter = m_Instrument_list.erase(iter);
	  if (iter == m_Instrument_list.end())
	    return NULL;
	  else
	    return *iter;
	}
      else
	iter++;
    }
  return NULL;
}

void CScore::add_measure(double start, double stop)
{
  std::list<CMesure*>::iterator it;
  CMesure *pm;

  pm = new CMesure(start, stop);
  /*
  // Clear every measure after this one
  if (m_mesure_list.size() > 0)
    {
      it =   m_mesure_list.begin();
      while (it != m_mesure_list.end())
	{
	  if (start < (*it)->m_start)
	    {
	      it = m_mesure_list.erase(it);
	    }
	  else
	    it++;
	}
    }
  */
  add_measure(pm);
}

void CScore::add_measure(CMesure *pm)
{
  m_mesure_list.push_back(pm);
  sort_measures();
}

void CScore::delete_measure(CMesure* pm)
{
  std::list<CMesure*>::iterator it;

  it = m_mesure_list.begin();
  while (it != m_mesure_list.end())
    {
      if (pm == (*it))
	{
	  delete pm;
	  it = m_mesure_list.erase(it);
	}
      else
	it++;
    }
}

bool CScore::compare_bars(const CMesure *pfirst, const CMesure *psecond)
{
  return (pfirst->m_start < psecond->m_start);
}

void CScore::sort_measures()
{
  m_mesure_list.sort(CScore::compare_bars);
}

void CInstrument::add_note(CNote *pn)
{
  list<CNote*>::iterator iter;

  //printf("notelistsize=%d\n", (int)m_Note_list.size());
  if (m_Note_list.size() == 0)
    {
      m_Note_list.push_back(pn);
      return ;
    }
  iter = m_Note_list.begin();
  while (iter != m_Note_list.end())
    {
      if (pn->m_time <= (*iter)->m_time)
	{
	  // TODO If equal time, fuse two notes into a chord
	  // insert before
	  iter = m_Note_list.insert(iter, pn);
	  return ;
	}
      iter++;
    }
  m_Note_list.push_back(pn);
}

void CInstrument::delete_note(CNote *pn)
{
  list<CNote*>::iterator iter;

  if (m_Note_list.size() == 0)
      return ;
  for (iter = m_Note_list.begin(); iter != m_Note_list.end(); iter++)
    {
      if (pn == (*iter))
	{
	  delete pn;
	  iter = m_Note_list.erase(iter);
	  return ;
	}
    }
}

CNote* CInstrument::get_first_note(double timecode)
{
  if (m_Note_list.size() == 0)
    return NULL;
  m_iter = m_Note_list.begin();
  while (m_iter != m_Note_list.end() && (*m_iter)->m_time < timecode)
    m_iter++;
  if (m_iter == m_Note_list.end())
    return NULL;
  return (*m_iter);
}

CNote* CInstrument::get_next_note()
{
  if (m_iter != m_Note_list.end())
    m_iter++;
  if (m_iter == m_Note_list.end())
    {
      return (NULL);
    }
  return (*m_iter);
}

CNote* CInstrument::get_first_note_reverse(double timecode)
{
  if (m_Note_list.size() == 0)
    return NULL;
  m_riter = m_Note_list.rbegin();
  while (m_riter != m_Note_list.rend() && (*m_riter)->m_time > timecode)
    m_riter++;
  if (m_riter == m_Note_list.rend())
    return NULL;
  return (*m_riter);
}

CNote* CInstrument::get_next_note_reverse()
{
  if (m_riter != m_Note_list.rend())
    m_riter++;
  if (m_riter == m_Note_list.rend())
    {
      return (NULL);
    }
  return (*m_riter);
}

