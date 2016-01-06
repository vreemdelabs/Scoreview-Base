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
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <iterator>
#include <list>

#include <pthread.h>

#include <SDL2/SDL.h>

#include "portaudio.h"
#include "pa_ringbuffer.h"

#include "audioapi.h"
#include "audiodata.h"
#include "shared.h"
#include "thrportaudio.h"
#include "audiofifo.h"

//#define USE_BLOCKINGIO
#define SHOW_INTERFACES
#ifdef _DEBUG
#define SHOW_TIME_LOCKS
#endif

using namespace std;

// Everything linked to audio io

int g_thrportaretval;

int audioCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
  CaudioFifo   *paudiofifo;
  unsigned int  i;
  int           ret;
  float  buffers;
  //float         dump[FRAMES_PER_BUFFER];
//#define TESTOUTPUT
#ifdef TESTOUTPUT
  static int    phase = 0;
  unsigned long e;
  float        *pout = (float*)output;

  for (e = 0; e < frameCount; e++)
    pout[e] = 0.1 * sin((float)(phase + e) * 50. * 2. * 3.14);
  phase += e;
  ret = paContinue;
  return ret;
#endif
  paudiofifo = (CaudioFifo*)userData;
  if ((frameCount % FRAMES_PER_BUFFER) != 0 || frameCount == 0)
    {
      printf("Framecount critical error in portaudio callback!\n");
      ret = paComplete;
      paudiofifo->set_quit(true);
    }
  else
    {
      ret = paContinue;
      buffers = frameCount / FRAMES_PER_BUFFER;
      for (i = 0; i < buffers; i++)
	{
	  paudiofifo->push_input_samples((float*)input + i * FRAMES_PER_BUFFER, FRAMES_PER_BUFFER);
	  paudiofifo->pop_output_samples((float*)output + i * FRAMES_PER_BUFFER, FRAMES_PER_BUFFER);
	}
      // Dump if it is late FIXME
      /*
      while (available > 1)
	{
	  available = paudiofifo->pop_output_samples(dump, FRAMES_PER_BUFFER);
	}
      */
    }
  return ret;
}

int get_next_chunk(float *pb, t_audio_strip *ps, int offset, int chunksize)
{
  int e;
  int res;
  int samplenum = ps->samplenum;

  for (e = 0; e < offset + chunksize; e++)
    {
      if (e < offset || (ps->current_sample + e - offset) >= samplenum)
	pb[e] = 0; // Fill with 0, portaudio will say it's an underflow... FIXME???
      else
	pb[e] = ps->samples[ps->current_sample + e - offset];
    }
  for (; e < FRAMES_PER_BUFFER; e++)
    {
      pb[e] = 0; // Fill with 0, portaudio will say it's an underflow... FIXME???
    }
  ps->current_sample += chunksize;
  if (ps->current_sample >= samplenum)
      return 0;
  res = samplenum - ps->current_sample;
  return res;
}

void flush_filtered_bands(t_shared* pshared_data)
{
  std::list<t_audio_strip>::iterator siter;

  FLOCK;
  if (pshared_data->filtered_sound_list.size())
    {
      siter = pshared_data->filtered_sound_list.begin();
      while (siter != pshared_data->filtered_sound_list.end())
	{
	  delete[] (*siter).samples;
	  siter++;
	}
      pshared_data->filtered_sound_list.clear();
    }
  FUNLOCK;
}

void play_filtered_bands(PaStream* stream, t_shared* pshared_data, float *mix, int samplerate, CaudioFifo *paudiofifo)
{
  std::list<t_audio_strip> *stlist;
  float                     soundstrip[FRAMES_PER_BUFFER];
  int                       numsamples = 0;
  int                       i, offset, needed_samples;
  float                     listsize;
  t_audio_strip             strip;
  std::list<t_audio_strip>::iterator siter;
  double                    current_timecode;
  int                       t1, t2, t3, t4;

  t1 = SDL_GetTicks();
  FLOCK;
  stlist = &pshared_data->filtered_sound_list;
  t3 = SDL_GetTicks();
  if (stlist->size() > 0)
    {
      for (i = 0; i < FRAMES_PER_BUFFER; i++)
	mix[i] = 0;
      current_timecode = 0.001 * SDL_GetTicks();
      listsize = 0;
      siter = stlist->begin();
      while (siter != stlist->end())
	{
	  strip = *siter;
	  if (strip.playtimecode <= current_timecode)
	    {
	      needed_samples = (int)((current_timecode - strip.playtimecode) * samplerate);
	      if (needed_samples >= FRAMES_PER_BUFFER)
		{
		  needed_samples = FRAMES_PER_BUFFER;
		  offset = 0;
		}
	      else
		offset = FRAMES_PER_BUFFER - needed_samples;
	      numsamples = get_next_chunk(soundstrip, &(*siter), offset, needed_samples);
	      if (numsamples > 0)
		{
		  //printf("numsamples is  %d should be %d\n", numsamples, FRAMES_PER_BUFFER);
		  for (i = 0; i < FRAMES_PER_BUFFER; i++)
		    mix[i] += soundstrip[i];
		  listsize++;
		}
	      if (numsamples < FRAMES_PER_BUFFER)
		{
		  //printf("deleting filter object\n");
		  delete[] (*siter).samples;
		  siter = stlist->erase(siter);
		}
	      else
		siter++;
	    }
	  else
	    siter++;
	}
    }
  else
    {
      for (i = 0; i < FRAMES_PER_BUFFER; i++)
	mix[i] = 0;
    }
  FUNLOCK;
//#define TESTOUTPUT
#ifdef TESTOUTPUT
  for (i = 0; i < FRAMES_PER_BUFFER; i++)
    mix[i] += 0.001 * sin((float)i * 100. * 2. * 3.14);
#endif
  t4 = SDL_GetTicks();
  // Output the mixed sound chunks
#ifdef USE_BLOCKINGIO
  int err = Pa_WriteStream(stream, mix, FRAMES_PER_BUFFER);
  if (err)
    {
      printf("Pa stream wr error: \"%s\"\n", Pa_GetErrorText(err));
    }
#else
  paudiofifo->push_output_samples(mix, FRAMES_PER_BUFFER);
#endif
  t2 = SDL_GetTicks();
  if (t2 - t1 > 8) printf("Playfiltered bands toook %d   and %d in locked and %d outputing the stream\n", t2 - t1, t3 - t1, t2 - t4);
}

// Using the api and device names because some apis like pulse crash
// and disapear sometimes so indexes are not reliable.
bool save_audio_parameters(t_channel_select_strings *pchs, PaHostApiIndex api_index, PaDeviceIndex input_device, PaDeviceIndex output_device)
{
  const PaDeviceInfo *deviceInfo;
  long                deviceID;
  bool                binputfound, boutputfound;

  const PaHostApiInfo* pinfo = Pa_GetHostApiInfo(api_index);
  pchs->api_name = string(pinfo->name);
  binputfound = boutputfound = false;
  for (int d = 0; d < pinfo->deviceCount; d++)
    {
      deviceID = Pa_HostApiDeviceIndexToDeviceIndex(api_index, d);
      deviceInfo = Pa_GetDeviceInfo(deviceID);
      if (deviceID == input_device)
	{
	  pchs->in_device_name = string(deviceInfo->name);
	  binputfound = true;
	}
      if (deviceID == output_device)
	{
	  pchs->out_device_name = string(deviceInfo->name);
	  boutputfound = true;
	}
    }
  return binputfound && boutputfound;
}

bool get_audio_parameters(t_channel_select_strings *pchs, PaHostApiIndex *papi_index, PaDeviceIndex *pinput_device, PaDeviceIndex *poutput_device)
{
  int                 apiinterfaces = Pa_GetHostApiCount();
  const PaDeviceInfo *deviceInfo;
  long                deviceID;
  int                 i, d;
  bool                binputfound, boutputfound;

#ifdef SHOW_INTERFACES
  printf("get audio parameters from:\n%s\n%s\n%s\n", pchs->api_name.c_str(),  pchs->in_device_name.c_str(),  pchs->out_device_name.c_str());
#endif
  binputfound = boutputfound = false;
  for (i = 0; i < apiinterfaces; i++)
    {
      const PaHostApiInfo* pinfo = Pa_GetHostApiInfo(i);
      if (pinfo->deviceCount > 0)
	{
	  if (pchs->api_name == string(pinfo->name))
	    {
	      *papi_index = i;
	      for (d = 0; d < pinfo->deviceCount; d++)
		{
		  deviceID = Pa_HostApiDeviceIndexToDeviceIndex(i, d);
		  deviceInfo = Pa_GetDeviceInfo(deviceID);
		  if (pchs->in_device_name == string(deviceInfo->name) &&
		      deviceInfo->maxInputChannels > 0)
		    {
		      *pinput_device = deviceID;
		      binputfound = true;
		    }
		  if (pchs->out_device_name == string(deviceInfo->name) &&
		      deviceInfo->maxOutputChannels > 0)
		    {
		      *poutput_device = deviceID;
		      boutputfound = true;
		    }
		}
	    }
	}
    }
  return binputfound && boutputfound;
}

void open_stream(t_shared* pshared_data, PaStream** pstream, CaudioFifo *paudiofifo)
{
  PaStreamParameters inputParameters;
  PaStreamParameters outputParameters;
  PaError            err = paNoError;
  PaHostApiIndex     api_index;

  if (!get_audio_parameters(&pshared_data->chs, &api_index, &inputParameters.device, &outputParameters.device))
    {
      inputParameters.device = Pa_GetDefaultInputDevice(); // default input device, in absolute padevice index
      if (inputParameters.device == paNoDevice)
	{
	  fprintf(stderr, "Error: No default input device.\n");
	  g_thrportaretval = PO_ERR_NOINPUT;
	  exit(1);
	}
      outputParameters.device = Pa_GetDefaultOutputDevice(); // default output device
      if (outputParameters.device == paNoDevice)
	{
	  fprintf(stderr,"Error: No default output device.\n");
	  g_thrportaretval = PO_ERR_NOOUTPUT;
	  exit(1);
	}
      // Store if in the form (API, device)
      if (!save_audio_parameters(&pshared_data->chs, Pa_GetDefaultHostApi(), inputParameters.device, outputParameters.device))
	{
	  printf("Warning: saving the audio config failed.\n");
	}
    }
#ifdef SHOW_INTERFACES
  printf("openingconv in api=\"%s\"\n in =\"%s\"\n out=\"%s\"\n", pshared_data->chs.api_name.c_str(), pshared_data->chs.in_device_name.c_str(), pshared_data->chs.out_device_name.c_str());
  printf(" input device=%d output device=%d\n", inputParameters.device, outputParameters.device);
#endif
  //-------------------------------------------------------------------------
  //
  // Open the stream
  //
  //-------------------------------------------------------------------------
  inputParameters.channelCount = 1; // mono input
  inputParameters.sampleFormat = paFloat32;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL; // Host api whatever, not used
  // Out
  outputParameters.channelCount = 1;
  outputParameters.sampleFormat = paFloat32;
  outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowInputLatency;
  outputParameters.hostApiSpecificStreamInfo = NULL; // Host api whatever, not used
  // Start recording
  err = Pa_OpenStream(pstream,
		      &inputParameters,
		      &outputParameters,
		      pshared_data->samplerate,
		      FRAMES_PER_BUFFER,
		      paNoFlag,
#ifdef USE_BLOCKINGIO
		      NULL,
#else
		      audioCallback,
#endif
		      paudiofifo);
  if (err != paNoError)
    {
      printf("Error opening the api=\"%s\"\n input =\"%s\"\n output=\"%s\"\n", pshared_data->chs.api_name.c_str(), pshared_data->chs.in_device_name.c_str(), pshared_data->chs.out_device_name.c_str());
      printf("Error, failed to open the stream: \"%s\"\n", Pa_GetErrorText(err));
      g_thrportaretval = PO_ERR_OPENS;
      //LOCK;
      //pshared_data->bquit = true;
      //UNLOCK;
      exit(EXIT_FAILURE);
      //pthread_exit(&g_thrportaretval);
    }
  err = Pa_StartStream(*pstream);
  if (err != paNoError)
    {
      printf("Error: Failed to start the stream: \"%s\"\n", Pa_GetErrorText(err));
      g_thrportaretval = PO_ERR_OPENS;
      exit(1);
      //pthread_exit(&g_thrportaretval);
    }
}

void close_stream(PaStream** pstream)
{
  PaError errcode;

  errcode = Pa_StopStream(*pstream);
  if (errcode != paNoError)
    {
      printf("Error: stoping the audio stream.\n");
    }
  usleep(40000);
  errcode = Pa_CloseStream(*pstream);
  if (errcode != paNoError)
    {
      printf("Error: closing the audio stream.\n");
    }
}

void process_audio_commands_in_locked_area(t_shared* pshared_data, PaStream** pstream, CaudioFifo *paudiofifo, double *pSDL_start_timecode, double *pSDL_start_sample_time)
{
  std::list<t_audio_track_cmd>::iterator iter;
  double                                 endtimecode;

  iter = pshared_data->audio_cmds.begin();
  while (iter != pshared_data->audio_cmds.end())
    {
      if ((*iter).direction == ad_app2threadaudio)
	{
	  switch ((*iter).command)
	    {
	    case ac_reset:
	      {
		// Free the previously recorded data
		delete pshared_data->pad;
		pshared_data->pad = new Caudiodata();
		// Reset timecodes in the shared memory
		endtimecode  = 0.;
		{
		  pshared_data->timecode = endtimecode;
		  pshared_data->lastimgtimecode = endtimecode;
		  pshared_data->trackend = endtimecode;
		}
		pshared_data->play_State = state_stop;
	      }
	      break;
	    case ac_record:
	      {
		*pSDL_start_timecode = SDL_GetTicks() / 1000.;
		*pSDL_start_sample_time = (double)pshared_data->pad->get_samplenum() / (double)pshared_data->samplerate;
		pshared_data->play_State = state_record;
	      }
	      break;
	    case ac_stop:
	      {
		pshared_data->play_State = state_stop;
	      }
	      break;
	    case ac_change_io:
	      {
		printf("Closing stream\n");
		close_stream(pstream);
		usleep(4000);
		open_stream(pshared_data, pstream, paudiofifo);
		*pSDL_start_timecode = SDL_GetTicks() / 1000.;
		*pSDL_start_sample_time = (double)pshared_data->pad->get_samplenum() / (double)pshared_data->samplerate;
		printf("Stream opened\n");
	      }
	      break;
	    case ac_flush_strips:
	      {
		flush_filtered_bands(pshared_data);
	      }
	      break;
	    default:
	      break;
	    }
	  iter = pshared_data->audio_cmds.erase(iter);
	}
      else
	iter++;
    }
}

void send_event_to_app(int type)
{
  SDL_Event event;

  SDL_zero(event);
  event.type = type;
  event.user.code = 0;
  event.user.data1 = 0;
  if (SDL_PushEvent(&event) == 0)
    printf("SDL error, pushing the user event failed in \"%s\".\n", __FILE__);
}

void probe_audio_APIs_in_locked_area(t_shared* pshared_data)
{
  int             apiinterfaces = Pa_GetHostApiCount();
  t_portaudio_api api;
  t_pa_device     dev;

  pshared_data->pa_api_list.clear();
  for (int i = 0; i < apiinterfaces; i++)
    {
      const PaHostApiInfo* pinfo = Pa_GetHostApiInfo(i);
#ifdef SHOW_INTERFACES
      printf("%d interface name is=%s device count is %d type is=%d\n", i, pinfo->name, pinfo->deviceCount, pinfo->type);
#endif
      if(pinfo->deviceCount > 0)
	{
	  api.name = string(pinfo->name);
	  api.device_list.clear();
	  for (int d = 0; d < pinfo->deviceCount; d++)
	    {
	      const PaDeviceInfo *deviceInfo;
	      long deviceID = Pa_HostApiDeviceIndexToDeviceIndex(i, d);
	      deviceInfo = Pa_GetDeviceInfo(deviceID);
#ifdef SHOW_INTERFACES
	      printf("   %d device info: name=%s\n", d, deviceInfo->name);
	      printf("   padevice: %d\n", Pa_HostApiDeviceIndexToDeviceIndex(i, d));
	      printf("   maxInputChannels=%d\n   maxOutputChannels=%d\n\n", deviceInfo->maxInputChannels, deviceInfo->maxOutputChannels);
#endif
	      dev.name = string(deviceInfo->name);
	      dev.inputs  = deviceInfo->maxInputChannels;
	      dev.outputs = deviceInfo->maxOutputChannels;
	      dev.device_api_code = d;
	      //dev.device_code = deviceID;
	      api.device_list.push_back(dev);
	    }
	  pshared_data->pa_api_list.push_back(api);
	}
    }
}

// 2 solutions to avoid threads messing with the audio input without mutexes (not allowed in callbacks):
// a) use the lockfree PaUtilRingBuffer in the callback
// b) use the blocking Pa_ReadStream inside the thread with callback function pointer set to NULL

/*static*/ void* thr_portaudio(void *p_data)
{
  t_shared*          pshared_data = (t_shared*)p_data;
  bool               bquit;
  PaStream*          stream;
  PaError            err = paNoError;
  unsigned long      samplecount;
  double             endtimecode;
  double             timecode;
  double             practice_start_tick;
  float              sampleblock[FRAMES_PER_BUFFER];
  float              mix[FRAMES_PER_BUFFER];
  t_audio_track_cmd  acmd;
  bool               bsuccess;
  bool               boutput;
  int                samplerate;
  CaudioFifo         *paudiofifo;
  //PaTime             paudio_start_timecode;
  double             SDL_start_timecode;
  double             last;
  double             SDL_start_sample_time;
  bool               bOpenClEnabled;
#ifdef SHOW_TIME_LOCKS
  int                t1, t2;
#endif

  g_thrportaretval = 0;
  bOpenClEnabled = false;
  while (!bOpenClEnabled)  // Because it's "je t'aime moi non plus" between the sound chip and the graphics card drivers.
    {
      LOCK;
      bOpenClEnabled = pshared_data->bOpenClEnabled;
      UNLOCK;
      usleep(10000);
    }
  // Atomic init
  LOCK;
  printf("Initialising the portaudio thread\n");
  // Init portaudio
  err = Pa_Initialize();
  if (err != paNoError)
    {
      g_thrportaretval = PO_ERR_INIT;
      pthread_exit(&g_thrportaretval);
    }
  UNLOCK;
  //
  samplecount = 0;
  LOCK;
  probe_audio_APIs_in_locked_area(pshared_data);
  samplerate = pshared_data->samplerate;
  paudiofifo = new CaudioFifo(samplerate, FRAMES_PER_BUFFER);
  open_stream(pshared_data, &stream, paudiofifo);
  practice_start_tick = 0;
  //paudio_start_timecode = Pa_GetStreamTime(&stream);
  //SDL_start_timecode = SDL_GetTicks() / 1000.;
  SDL_start_timecode = -1;
  SDL_start_sample_time = 0;
  bquit = pshared_data->bquit;
  UNLOCK;
  while (!bquit)
    {
      // Get entering audio data
      boutput = true;
#ifdef USE_BLOCKINGIO
      err = Pa_ReadStream(stream, sampleblock, FRAMES_PER_BUFFER);
      // Checkoverflow is disabled in portaudio, do not exit the thread on error
      bsuccess = true;
      if (err)
	{
	  bsuccess = false;
	  g_thrportaretval = PO_ERR_OVERFLOW;
	  printf("Pa stream rd error: \"%s\"\n", Pa_GetErrorText(err));
	}
#else
      bsuccess = paudiofifo->pop_input_samples(sampleblock, FRAMES_PER_BUFFER);
      if (paudiofifo->get_quit())
	{
	  printf("Error in the audio calback.\n");
	  exit(EXIT_FAILURE);
	}
      boutput = bsuccess;
#endif
#ifdef SHOW_TIME_LOCKS
      t1 = SDL_GetTicks();
#endif
      //printf("recorded samples = %ld\n", samplecount);
      // Fill the shared buffer
      LOCK;
      // Check for audio command messages
      if (pshared_data->audio_cmds.size() > 0)
	{
	  process_audio_commands_in_locked_area(pshared_data, &stream, paudiofifo, &SDL_start_timecode, &SDL_start_sample_time);
	}
      switch (pshared_data->play_State)
	{
	case state_record:
	  {
	    if (bsuccess)
	      {
		// Add the new samples to the track
		pshared_data->pad->add_data(sampleblock, FRAMES_PER_BUFFER);
		// Set the timecode on the last sample
		samplecount = pshared_data->pad->get_samplenum();
		endtimecode  = (double)samplecount / (double)pshared_data->samplerate;
		pshared_data->trackend = endtimecode;
	      }
	    //---------------------------------------------------------------------------------------
	    // Place the last sample timecode on a theoritical smooth sample rate placement.
	    // Do not care if it passes tracklen.
	    //---------------------------------------------------------------------------------------
	    if (SDL_start_timecode == -1)
	      {
		SDL_start_timecode = SDL_GetTicks() / 1000.;
	      }
	    last = SDL_GetTicks() / 1000.;
	    pshared_data->timecode = SDL_start_sample_time + (last - SDL_start_timecode);
	    /* 
	      if (pshared_data->timecode > endtimecode)
	      {
	      printf("overflowed by %f samples\n", (pshared_data->timecode - (endtimecode - tperframe)) * (double)pshared_data->samplerate);
	      pshared_data->timecode = endtimecode - tperframe / 3.;
	      }
	    */
	    if (pshared_data->timecode < 0)
	      pshared_data->timecode = 0;
	  }
	  break;
	case state_wait_practiceloop:
	  {
	    timecode = SDL_GetTicks() / 1000.;
	    //printf("comparinf %f to %f\n", timecode, pshared_data->practice_countdown_tick);
	    if (fabs(timecode - pshared_data->practice_countdown_tick) > 3.)
	      {
		// Set this value so it starts playing now
		pshared_data->practice_countdown_tick = timecode - ((pshared_data->practice_loop_duration / pshared_data->practicespeed) + PRACTICE_BETWEEN_REPLAY_TIME);
		pshared_data->play_State = state_practiceloop;
	      }
	  }
	  break;
	case state_practiceloop:
	  {
	    timecode = SDL_GetTicks() / 1000.;
	    if (fabs(timecode - pshared_data->practice_countdown_tick) > (pshared_data->practice_loop_duration / pshared_data->practicespeed) + PRACTICE_BETWEEN_REPLAY_TIME)
	      {
		pshared_data->practice_countdown_tick = timecode;
		// Send a practice loop command to the main app
		acmd.v = 0;
		acmd.command = ac_practice_loop;
		acmd.direction = ad_thread2app;
		pshared_data->audio_cmds.push_back(acmd);
		// Force it's treatment with an SDL user event
		send_event_to_app(pshared_data->audio_cmd_sdlevent);
	      }
	  }
	  break;
	case state_wait_practice:
	  {
	    timecode = SDL_GetTicks() / 1000.;
	    //printf("comparinf %f to %f\n", timecode, pshared_data->practice_countdown_tick);
	    if (fabs(timecode - pshared_data->practice_countdown_tick) > 3.)
	      {
		pshared_data->practice_start = pshared_data->timecode;
		practice_start_tick = timecode;
		pshared_data->play_State = state_practice;
	      }
	  }
	  break;
	case state_practice:
	  {
	    timecode = SDL_GetTicks() / 1000.;
	    pshared_data->timecode = pshared_data->practice_start + (timecode - practice_start_tick) * pshared_data->practicespeed;
	    if (pshared_data->timecode > pshared_data->trackend + pshared_data->viewtime / 2)
	      {
		pshared_data->timecode = pshared_data->practice_start;
		pshared_data->play_State = state_stop;
		// Send a stop command to the main app
		acmd.v = 0;
		acmd.command = ac_stop;
		acmd.direction = ad_thread2app;
		pshared_data->audio_cmds.push_back(acmd);
		// Force it's treatment with an SDL user event
		send_event_to_app(pshared_data->audio_cmd_sdlevent);
	      }
	    //samplecount = 0;
	  }
	  break;
	case state_stop:
	  {
	    samplecount = 0;
	  }
	  break;
	default:
	  break;
	}
      bquit = pshared_data->bquit;
      samplerate = pshared_data->samplerate;
      UNLOCK;
#ifdef SHOW_TIME_LOCKS
      t2 = SDL_GetTicks();
      if (t2 - t1 > 4) printf("Warning: audio thread locked for more than %dms\n", t2 - t1);
#endif
      // Filter and play selected sounds or just write null sound
      if (boutput)
	{
	  play_filtered_bands(stream, pshared_data, mix, samplerate, paudiofifo);
	}
#ifdef SHOW_TIME_LOCKS
      t1 = SDL_GetTicks();
      if (t1 - t2 > 4) printf("Warning: audio thread locked for more than %dms\n", t1 - t2 );
#endif
#ifndef USE_BLOCKINGIO
      double framespersecond = (double)samplerate / (double)FRAMES_PER_BUFFER;
      double pausetime = 1. / (2. * framespersecond);
      pausetime = pausetime * 1000000;
      usleep((int)pausetime);
      //usleep(1000);
#endif
    }
  // Free everything
  close_stream(&stream);
  Pa_Terminate();
#ifdef _DEBUG
  printf("Audio thread closing\n");
#endif
  return &g_thrportaretval;
}

