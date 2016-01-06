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

// when a kernel execution take something more than 20ms it makes a mess of the sound driver
// and nothing is smooth. Plus sound comes from the read function in many multiples of the 
// buffer size and when displaying it, it does not look fluid.
// This class cuts a spectrometer refresh in many chunks taking less than 20ms, and averages
// the timecode in order to progress with fluidity.

#define MAX_KERNEL_TIME  10000
#define DFTBANDLIMIT        32

class Cdftspliter
{
 public:
  Cdftspliter(Copenclspectrometer *poclSpectrometer, int width, int samplerate);
  ~Cdftspliter();
  
  bool update_spectrometer_image(t_shared *pshared_data, double last_time);
  
 private:

  typedef struct s_dft_image_update
  {
    double       tracklen;
    double       screen_begin_timecode;
    double       screen_end_timecode;
    double       update_begin_timecode;
    double       update_end_timecode;
  }              t_dft_image_update;

  enum echunk_type
    {
      chunk_out_of_track,
      chunk_on_track
    };

  typedef struct s_dft_chunk
  {
    echunk_type  type;
    int          sample_start;
    int          sample_stop;
    double       tcode_start;
    double       tcode_stop;
    int          pixel_xstart;
    int          pixel_xstop;
    float        max_freq;
    int          totalsamples;
    double       creation_time;
    bool         bforward;
  }              t_dft_chunk;

  enum eviewtype
    {
      view_enhanced,
      view_calibrated,
      view_unknown
    };

  void draw_out_of_track_chunks(t_shared *pshared_data, int updatewidth, int cut, int xstart, int width, int height, bool view_calibrated);
  void draw_next_chunks(t_shared *pshared_data, double last_time, double imgtcode, double T, int width, int height, eviewtype viewtype);
  void cut_out_of_track_chunks(double last_time, t_dft_image_update *pup, double viewtime, int width, bool bforward);
  void cut_dft_to_chunks(double last_time, t_dft_image_update *pup, double viewtime, int width, float max_freq, bool bforward);
  void get_needed_samples(Caudiodata *pad, t_dft_chunk *pch, double T);
  void remove_sample_to_pixel_aliasing(double &end_timecode, double viewtime, int width);
  static bool compare(const t_dft_chunk& first, const t_dft_chunk& second);

 private:
  Copenclspectrometer *m_pdft;
  double m_previous_end_timecode;
  double m_previous_missing_samples;
  float  m_previous_max_frequ;
  float  m_previous_viewtime;
  eviewtype m_previouss_view_type;
  int    m_processed_pixels;
  int    m_circularindex;
  int    m_samplerate;
  float  *m_psamples;
  float  *m_preorganise_buffer;
  int    m_buffer_size;
  std::list<t_dft_chunk> m_dftclist;
  std::list<t_dft_chunk>::iterator m_dftciter;
};

