
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

typedef struct s_Ndividers
{
//  int          hann_decimation;    // If the octave is lower than octavesplit then
//  int          hann_chunk_offset;  // the processing is made in many steps
  int          Octave;         // N is divided depending on the octave
  int          N;
  int          OctaveN;
  int          Nsegmentsize;
  int          Nsegment;
  int          Nbasestep;
  int          Nstep;          // Segment of N / 128 in an OctaveN
  int          localn;
}              t_Ndividers;

float find_pixel_column_sample(int xpos, int width, __global t_ckparam *sp, int Nsegment, t_Ndividers *pdiv)
{
  int   total_samples;
  int   pixel_sample;
  int   N;
  int   Ns;
  int   div;
  int   base;

  pdiv->N = sp->N;
  N = sp->N;
  Ns = Nsegment;
  // While Ns is greter than 128, 128 + 64... divide N
  div = 0;
  base = 0;
  while (((Ns & 0x80) != 0) && (div < 8))
    {
      N = N / 2;
      Ns = (Ns << 1) & 0xFF;
      base += (1 << (7 - div));
      div++;
    }
  // Segment in the OctaveN range
  pdiv->Nsegment = Nsegment;
  pdiv->Nbasestep = base;
  pdiv->Nstep = Nsegment - base;
  pdiv->Octave = div;
  div = div > 7? 7 : div;
  pdiv->OctaveN = sp->N / (1 << div);
  pdiv->Nsegmentsize = sp->N / 128;
  pdiv->localn = pdiv->Nstep * pdiv->Nsegmentsize;
  // Samples for N / pow(2, div)
  total_samples = sp->stop_sample - sp->start_sample;
  pixel_sample = (int)(((float)total_samples * (float)xpos) / (float)width);
  return sp->start_sample + pixel_sample;
}

float hann(float i, int nn)
{
  return (0.5 * (1.0 - cos(2.0 * M_PI_F * i / (float)(nn - 1))));
}

// Number of samples inside the discrete transform interval
float get_T(__global t_ckparam *sp, int N, int sampling_frequency)
{
  float T;

  T = ((float)N / (float)sampling_frequency);
  return T;
}

#define NOTE_A4F 440.
float get_slot_freq_from_note(int note)
{
  float A4f = NOTE_A4F;

  return(A4f * pow(2., ((float)note - 57.) / 12.));
}

// Finds the k factor closer to f in the DFT of N samples
// f(1) = 1 / T   f(k) = (1 / T) x k
// f = k / T    k = f * T
float get_k(float fk, float T, int N)
{
  float k;

  k = fk * T;
  if (k > N || k < 0)
    k = 0;
  return (k);
}

#define HANN

//#define BASIC_DFT
#ifdef BASIC_DFT
float2 frequency_strength(__global float *psamples, int k, __global float *window, t_Ndividers *pdiv)
{
  int   n;
  float out;
  __global float *X;
  float Xn;
  float O;
  float cosO, sinO;
  float Real, Im;
  int   N;
  int   Ns;
  float2 res;

  Ns = pdiv->Nsegmentsize;
  N  = pdiv->OctaveN;
  // One k slot of a dft
  O = (2. * M_PI * (float)k) / (float)N;
  X = psamples;
  Real = 0.;
  Im = 0.;
  n = pdiv->localn;
  for (; n < pdiv->localn + Ns; n++)
    {
      cosO = cos(O * (float)n);
      sinO = sin(O * (float)n);
#ifdef HANN
      //Xn = X[n] * hann(n, Ns);
      Xn = X[n] * window[n];
#else
      Xn = X[n];
#endif
      Real += Xn * cosO;
      Im   += Xn * sinO;
    }
  //out = sqrt(Real * Real + Im * Im);
  //return out;
  res.x = Real;
  res.y = Im;
  return res;
}
#else
float2 frequency_strength(__global float *psamples, int k, __global float *window, t_Ndividers *pdiv)
{
  int   n;
  float O;
  float2 res;
  float4 Xn;
  float4 hannv;
  float4 wave;
  float4 waven;
  float4 waveReal;
  float4 waveIm;
  float  cosO, sinO;
  int    N;
  int    Ns;

  Ns = pdiv->Nsegmentsize;
  N  = pdiv->OctaveN;
  n = pdiv->localn;
  // One k slot of a dft
  O = (2. * M_PI_F * (float)k) / (float)N;
  res = (float2)(0., 0.);
  wave =  (float4)(O, O, O, O);
  waven = (float4)(n, n + 1, n + 2, n + 3);
  for (; n < pdiv->localn + Ns - 4; n += 4)
    {
      waveIm = sincos(wave * waven, &waveReal);
      Xn = vload4(n / 4, psamples);
#ifdef HANN
      hannv = vload4(n / 4, window);
      Xn = Xn * hannv;
#endif
      res.x += dot(Xn, waveReal);
      res.y += dot(Xn, waveIm);
      waven += (float4)(4, 4, 4, 4);
    }
  for (; n < pdiv->localn + Ns; n++)
    {
      cosO = cos(O * (float)n);
      sinO = sin(O * (float)n);
#ifdef HANN
      Xn.x = psamples[n] * window[n];
#else
      Xn.x = psamples[n];
#endif
      res.x += Xn.x * cosO;
      res.y += Xn.x * sinO;
    }
  //return sqrt(dot(res, res));
  return res;
}
#endif

// This kernel makes a frequency analysis where the slots are the Piano notes frequencies
__kernel void chromagramme(__global float *s, __global t_ckparam *sp, __global float *window, __global float *chromagram)
{
  int           sampleindex;
  int           xpos;
  int           note;
  int           Nsegment;
  float         T;
  int           line_size;
  int           col_size;
  float         f;
  float         k;
  float         fv;
  __global float *pwindow;
  t_Ndividers   sdiv;
  float2        res;

  // Height of the analysis in segments of N/128 = 256
  col_size = get_global_size(0);
  // Get the column index
  Nsegment = get_global_id(0);
  // Get the note
  note = get_global_id(1);
  // width of the analysis
  line_size = get_global_size(2);
  // Get the line index (time of the analysis), one work-group per line, samples are read once for every work-item
  xpos = get_global_id(2);
  sampleindex = find_pixel_column_sample(xpos, line_size, sp, Nsegment, &sdiv);
  //
  if (sampleindex + (sdiv.OctaveN / 2) >= sp->tracksize)
    {
      fv = 0;
      chromagram[2 * (xpos * 256 * 12 + note * 256 + Nsegment)] = fv;
      chromagram[2 * (xpos * 256 * 12 + note * 256 + Nsegment) + 1] = fv;
    }
  else
  {
    T = get_T(sp, sdiv.OctaveN, sp->sampling_frequency);
    f = get_slot_freq_from_note((sdiv.Octave * 12) + note);
    k = get_k(f, T, sdiv.OctaveN);
    pwindow = &window[note * sdiv.N * 2 + sdiv.Nbasestep * sdiv.Nsegmentsize];
    // And now the dft
    res = frequency_strength(s + sampleindex - (sdiv.OctaveN / 2), k, pwindow, &sdiv);
    // Store it
    //fv = (sdiv.Nstep) + (sdiv.localn * 10000);
    chromagram[2 * (xpos * 256 * 12 + note * 256 + Nsegment)] = res.x;
    chromagram[2 * (xpos * 256 * 12 + note * 256 + Nsegment) + 1] = res.y;
  }
  //barrier( CLK_GLOBAL_MEM_FENCE);
}

__kernel void last_sum(__global t_ckparam *sp, __global float *chromagram)
{
  int           col_size;
  int           i;
  int           xpos;
  int           note;
  int           line_size;
  t_Ndividers   sdiv;
  float2        res;
  int           Octave;
  int           start;
  int           octaved[9];
  int           segments[] = {128, 64, 32, 16, 8, 4, 2, 1, 1};
  float         fv;

  octaved[0] = 0;
  for (i = 1; i < 9; i++)
    {
      octaved[i] = octaved[i - 1] + segments[i - 1];
    }
//  const size_t global_work_size[3] = {9, notes, updatew};  // Total numer of work items = octaves * 12 * width
//  const size_t local_work_size[3] = {9, 12, 1}; // 
  // Height of the analysis in segments of N/128 = 256
  col_size = get_global_size(2);
  // Get the column index
  Octave = get_global_id(0);
  // Get the note
  note = get_global_id(1);
  // width of the analysis
  line_size = get_global_size(2);
  // Get the line index (time of the analysis), one work-group per line, samples are read once for every work-item
  xpos = get_global_id(2);
  res = (float2)(0, 0);
  start = octaved[Octave];
  for (i = 0; i < 128; i++)
    {
      if (i < segments[Octave])
	{
	  res.x += chromagram[2 * (xpos * 256 * 12 + note * 256 + start + i)];
	  res.y += chromagram[2 * (xpos * 256 * 12 + note * 256 + start + i) + 1];
	}
    }
  fv = sqrt(dot(res, res)) / (float)segments[Octave];
  chromagram[2 * (xpos * 256 * 12 + note * 256) + Octave] = fv;
}

