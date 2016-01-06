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

typedef struct s_ckparam
{
  // Build time
  int          sampling_frequency;
  // Changes depending on space or time resolution
  int          N;
  // Used when recording to transform near the end of the track while it is recorded
  int          tracksize;
  // Changes depending on zoom factor
  int          start_sample;
  int          stop_sample;
}              t_ckparam;

class Copenclchroma : public Copencldevice
{
 public:
  Copenclchroma(cl_device_id deviceid, int samplingf, int wx, int wy, int maxsptime);
  ~Copenclchroma();

  int  init_opencl_chromagram();
  int  opencl_chromagram(float *psamples, int samplenum, int start, int stop, int updatewidth, unsigned long *ptime);
  void chromagram_to_img(unsigned int *pimg, int updatewidth, int circularindex, int imgheight);
  float get_needed_interval_time();
  void  calibration();
  int  note_id(int chroma_column, char *note_name, const int namestrsize);

 private:
  void init_hanwindows();
  float get_T(int note);
  int   get_N(float T);
  void set_kernel_prameters(int tracksize, int startsample, int stopsample);
  void create_kernels(t_ckparam *pKP);
  void release_opencl_chromagram();
  void filter_signal_zfactor(float *psamples, int *psamplenum);
  void partial_dft_to_note(float *dft_arr, float *pnotes, int note);
  void output_recomposition(int updatewidth);

 private:
  int   m_sampling_frequency;
  int   m_wx;
  int   m_output_height;
  int   m_recomp_height;
  Cf2n  m_f2n;
  int   m_max_needed_samples;
  float *m_windowdata;
  int   m_maxsptime;
  float m_maxdbvalue;
  int   m_usedoctaves;
  float *m_recomposebuffer;
  int   m_recomposebuffersize;

  // Opencl data
  cl_device_id     m_device;
  cl_context       m_context;
  cl_mem           m_samples;
  cl_program       m_program;
  cl_kernel        m_kernel;
  cl_kernel        m_kernelReorder;
  cl_command_queue m_queue;
  int              m_output_buffer_size;
  float            *m_outputbuffer;
  cl_mem           m_chroma_img;
  t_ckparam        m_kparams;
  cl_mem           m_kernelcfg;
  cl_mem           m_hannwindow;
};

