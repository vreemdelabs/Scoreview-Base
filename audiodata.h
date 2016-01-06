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

#define AUDIO_CHUNK_SIZE (512 * 1024)

class CaudioChunk
{
 public:
  CaudioChunk();
  ~CaudioChunk();
  
  int add_data(float *pdata, int size);
  bool spaceavailable();
 private:
  int m_size;
 public:
  int m_used;
  float *m_pdata;
};

typedef struct s_ret_samples
{
  int obtained;
  int missing_left;
  int missing_right; 
}              t_ret_samples;

class Caudiodata
{
 public:
  Caudiodata();
  ~Caudiodata();

  int add_data(float *pdata, int size);
  t_ret_samples get_data(int samplepos, float *pdataout, int size);
  long get_samplenum();
  float get_sample(int n);

  CaudioChunk *get_first();
  CaudioChunk *get_next();
 private:
  std::list<CaudioChunk*> m_CaudioChunk_list;
  std::list<CaudioChunk*>::iterator m_iter;
};

