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

#define DIV_LIMIT 1024

class Ctrack_render
{
 public:
  Ctrack_render(int w, int h);
  ~Ctrack_render();

  void add_new_data(Caudiodata *pad);
  void render_track_data(int dispsize, int currentsample, int *pimg, int imgsz);
  void reset();

  typedef struct s_trackdata
  {
    s_trackdata()
    {
      pmax = 0;
      pmid = 0;
      nmax = 0;
      nmid = 0;
    }

    float pmax;
    float pmid;
    float nmax;
    float nmid;
  }             t_track_data;

 private:
  void merge_work_data(t_track_data *pwrk);
  void get_group_mid_e_max(Caudiodata *pad, int total_size, int groupsize, int lastgroup);
  int  get_backgcolor(int max, int h2, int v, bool bcurrent_disp);
  int  psrandom(int max);
  void get_antialiased_values(int i, int &max, int &mid, int &nmax, int &nmid);

 private:
  t_track_data m_pwrksamples[2 * DIV_LIMIT];
  int          m_wrk_size;
  int          m_divf;
  int          m_group_size;
  int          m_width;
  int          m_height;
};

