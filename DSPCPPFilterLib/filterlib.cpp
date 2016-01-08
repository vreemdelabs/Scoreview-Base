
#include "DspFilters/Dsp.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdio.h>

#include <iterator>
#include <list>

#include "filterlib.h"

#define ORDER   8
#define STOPDB 32.

void DSPCPPfilter_pass_bandCHII(float *psamples, int samplenum, int samplerate, float frequency, float bandwidth)
{
  // create a two channel audio buffer
  float* audioData[1];
  audioData[0] = psamples;
  
  // create a 1-channel Inverse Chebyshev Low Shelf of order 5
  // and passband ripple 0.1dB, without parameter smoothing and apply it.
  {
    Dsp::FilterDesign<Dsp::ChebyshevII::Design::BandPass <ORDER>, 1> f;
    Dsp::Params params;
    params[0] = 44100; // sample rate
    params[1] = ORDER;   // order
    params[2] = frequency;  // center frequency
    params[3] = bandwidth;   // bandwidth
    params[4] = STOPDB;   // stop DB
    f.setParams(params);
    f.process(samplenum, audioData);
  }
}

void DSPCPPfilter_pass_bandCHI(float *psamples, int samplenum, int samplerate, float frequency, float bandwidth)
{
  // create a two channel audio buffer
  float* audioData[1];
  audioData[0] = psamples;
  
  // create a 1-channel Inverse Chebyshev Low Shelf of order 5
  // and passband ripple 0.1dB, without parameter smoothing and apply it.
  {
    Dsp::FilterDesign<Dsp::ChebyshevI::Design::BandPass <ORDER>, 1> f;
    Dsp::Params params;
    params[0] = 44100; // sample rate
    params[1] = ORDER;   // order
    params[2] = frequency;  // center frequency
    params[3] = bandwidth;   // bandwidth
    params[4] = STOPDB;   // stop DB
    f.setParams(params);
    f.process(samplenum, audioData);
  }
}

void DSPCPPfilter_pass_band_Butterworth(float *psamples, int samplenum, int samplerate, float frequency, float bandwidth)
{
  // create a two channel audio buffer
  float* audioData[1];
  audioData[0] = psamples;

  {
    Dsp::FilterDesign<Dsp::Butterworth::Design::BandPass <ORDER>, 1> f;
    Dsp::Params params;
    params[0] = samplerate;
    params[1] = ORDER;   // order
    params[2] = frequency;  // center frequency
    params[3] = bandwidth;   // bandwidth
    params[4] = STOPDB;   // stop DB
    f.setParams(params);
    f.process(samplenum, audioData);
  }
}

void DSPCPPfilter_low_pass_Butterworth(float *psamples, int samplenum, int samplerate, float cutfrequency)
{
  // create a two channel audio buffer
  float* audioData[1];
  audioData[0] = psamples;

  {
    Dsp::FilterDesign<Dsp::Butterworth::Design::LowPass <ORDER>, 1> f;
    Dsp::Params params;
    params[0] = samplerate;
    params[1] = ORDER;   // order
    params[2] = cutfrequency;  // cut off frequency
    f.setParams(params);
    f.process(samplenum, audioData);
  }
}

