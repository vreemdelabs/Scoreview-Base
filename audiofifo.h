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

class CaudioFifo
{
 public:
  CaudioFifo(int samplerate, int framesize);
  ~CaudioFifo();

  // Used by the audio input calback
  void push_input_samples(float *psamples, int size);
  bool pop_input_samples(float *psamples, int size);

  // Used by the audio output calback
  void push_output_samples(float *psamples, int size);
  int  pop_output_samples(float *psamples, int size);

  void set_quit(bool bquit);
  bool get_quit();

 private:
  PaUtilRingBuffer m_rbufin;
  PaUtilRingBuffer m_rbufout;
  int   m_frame_size;
  int   m_frame_count;
  int   m_buffers_size;
  float *m_psamples_in;
  float *m_psamples_out;
  int   m_index;
  bool  m_bquit;
};

