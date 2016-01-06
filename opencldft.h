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

typedef struct s_fir_params
{
  int          intracksize;
  int          decimation;
}              t_fir_params;

typedef struct s_kparam
{
  // Build time
  int          sampling_frequency;
  // Changes depending on space or time resolution
  int          N;
  // Used when recording to transform near the end of the track while it is recorded
  int          tracksize;
  // Changes depending on zoom factor
  float        minfreq;
  float        maxfreq;
  int          start_sample;
  int          stop_sample;
}              t_kparam;

#define USEFIR_KERNEL
#define FIR_SIZE 516

class Copenclspectrometer : public Copencldevice
{
public:
  Copenclspectrometer(int samlingf, int wx, int wy, float fbase, float fmax);
  ~Copenclspectrometer();

  void calibration();
  int  init_opencl_dft();
  void set_start_stop_sample(int start, int stop);
  // start stop gives the spectrometer interval
  int  opencl_dft(float *psamples, int samplenum, int start, int stop, int updatewidth, float fmax, unsigned long *ptime);
  void dft_to_img(unsigned int *pimg, int updatewidth, int circularindex, bool bdecibelview);
  float get_analysis_interval_T(float maxdispf);
  void reset_attack(float *pattackbuffer, int width);
  void get_attack(float *pattackbuffer, int width, int circularindex);
  cl_device_id get_opencl_deviceid();

private:
  int   get_N(int sampling_frequency);
  void  set_kernel_prameters(int tracksize, int startsample, int stopsample, float fmax);
  void  create_kernels();
  void  han_window_setup();
  void  release_opencl_dft();
  void  img_float_to_RGB32(float *fbuffer, unsigned int *pimg, int width, int height, int updatewidth, int circularindex, bool bdecibelview);
  void  img_float_to_attack(float *fbuffer, int width, int height, int updatewidth, int circularindex);
  void  reverse(float *buffer, int size);
  void  filterloop(float *samples, float *pcoeffs, t_fir_params *params);
  void  downsample(float *psamples, int samplenum, int scut);
  // FIR
  //void downsample(float *psamples, int samplenum, int scut);
  void allocate_fir_buffers();
  void filter_signal_zfactor(float *psamples, int *psamplenum, int start, int stop, float fmax);
  void set_decimation_arguments();
  bool enqueue_fir_kernels(cl_event *ptimingevents);

  int   m_sampling_frequency;
  float m_fbase;
  float m_fmax;

  int   m_wx;
  int   m_wy;

  float m_maxdbvalue;
  int   m_dbvalueN;
  float *m_pfir_coefs;
  bool  m_bhalfband;

  // Opencl data
  cl_mem           m_samples;
  cl_program       m_program;
  cl_kernel        m_kernel;
  cl_command_queue m_queue;
  int              m_output_buffer_size;
  float            *m_outputbuffer;
  cl_mem           m_spectre_img;
  t_kparam         m_kparams;
  cl_mem           m_kernelcfg;
  cl_mem           m_hannwindow;
  float            *m_windowdata;
  int              m_maxwindowsize;
  // FIR filtering
  cl_mem           m_fircoef;
  cl_mem           m_firsamplebuffer;
  cl_mem           m_fircfg;
  cl_kernel        m_firkernel;
  t_fir_params     m_kfirparams;

  // Freq var
  float            *m_pattack;
  float            *m_pdecay;
};

