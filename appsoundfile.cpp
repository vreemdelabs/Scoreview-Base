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
#include <iterator>
#include <list>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <GL/gl.h>

#include <sndfile.h>
#include <tinyxml.h>

#include "audiodata.h"
#include "audioapi.h"
#include "shared.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "score.h"
#include "f2n.h"
#include "scorefile.h"
#include "pictures.h"
#include "keypress.h"
#include "instrumenthand.h"
#include "scoreplacement.h"
#include "scoreplacement_violin.h"
#include "scoreplacement_piano.h"
#include "scoreplacement_guitar.h"
#include "scoreedit.h"
#include "scorerenderer.h"
#include "card.h"
#include "messages.h"
#include "messages_network.h"
#include "app.h"
#include "sdlcalls.h"

using namespace std;

std::string check_extension(const std::string& filename, string extension)
{
  size_t lastdot = filename.find_last_of(".");
  size_t len = filename.size();

  if (lastdot == std::string::npos)
    return (filename + extension);
  if (filename.substr(lastdot, len) == extension)
    return filename;
  return (filename.substr(0, lastdot) + extension);
}

std::string change_extension(const std::string& filename, string extension)
{
  size_t lastdot = filename.find_last_of(".");

  if (lastdot == std::string::npos)
    return (filename + extension);
  return (filename.substr(0, lastdot) + extension);
}

void Cappdata::load_score(string path)
{
  CScoreFile scoref;
  string     sound_path;

  delete m_pscore;
  m_pscore = scoref.read_score_from_xml_file(path);
  if (m_pscore == NULL)
    {
      printf("Critical error loading the score file \"%s\".\n", sound_path.c_str());
      exit(EXIT_FAILURE);
    }
  m_pcurrent_instrument = m_pscore->get_first_instrument();
  set_score_renderer(m_pcurrent_instrument->get_name());
  assert(m_pscore != NULL);
  sound_path = change_extension(path, string(".wav"));
  if (load_sound_buffer(sound_path))
    {
      printf("Critical error loading the sound file \"%s\".\n", sound_path.c_str());
      exit(EXIT_FAILURE);
    }
}

void Cappdata::save_xml_score(string path)
{
  CScoreFile scoref;

  scoref.create_xml_file_from_score(m_pscore, string(path));
}

void Cappdata::save_score(string path)
{
  string sound_path;

  path = check_extension(path, ".xml");
  sound_path = change_extension(path, string(".wav"));
  if (save_sound_buffer(sound_path))
    {
      printf("Error saving the sound to \"%s\".\n", sound_path.c_str());
    }
  else
    {
      save_xml_score(path);
    }
}

void Cappdata::clear_sound()
{
  t_shared         *pshared_data = (t_shared*)&m_shared_data;
  string            instrument_name;
  t_audio_track_cmd acmd;
  int               i;
  bool              bpurge;

  //play_record_enabled(false);
  //play_practice_enabled(false);
  LOCK;
  // Clear the track view cache, the sound memory and reset every timecode to 0.
  acmd.v = 0;
  acmd.command = ac_reset;
  acmd.direction = ad_app2threadaudio;
  pshared_data->audio_cmds.push_back(acmd);
  acmd.direction = ad_app2threadspectr;
  pshared_data->audio_cmds.push_back(acmd);
  UNLOCK;
  // Update the spectrogram image
  wakeup_spectrogram_thread();
  // Wait for the command list to be purged
  bpurge = false;
  i = 0;
  while (!bpurge && i < 100)
    {
      usleep(20000);
      LOCK;      
      bpurge = pshared_data->audio_cmds.size() == 0;
      UNLOCK;
      i++;
    }
}

void Cappdata::reopen_stream(t_channel_select_strings *pchs)
{
  t_shared         *pshared_data = (t_shared*)&m_shared_data;
  t_audio_track_cmd acmd;

  LOCK;
  pshared_data->chs = *pchs;
  // Reopens the audio streams with the required api and device
  acmd.v = 0;
  acmd.command = ac_change_io;
  acmd.direction = ad_app2threadaudio;
  pshared_data->audio_cmds.push_back(acmd);
  UNLOCK;
  // Update the spectrogram image
  wakeup_spectrogram_thread();
}

bool Cappdata::save_sound_buffer(string file_name)
{
  t_shared     *pshared_data = (t_shared*)&m_shared_data;
  SF_INFO       sndInfo;
  SNDFILE      *sndFile;
  int           numframes;
  float         snd_buffer[4096];
  long          writtenFrames;
  t_ret_samples ret;
  int           samplepos;
  int           readsize;

  play_record_enabled(false);
  // Open sound file for writing
  numframes = pshared_data->pad->get_samplenum();
  sndInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
  sndInfo.channels = 1;
  sndInfo.samplerate = pshared_data->samplerate;
  sndFile = sf_open(file_name.c_str(), SFM_WRITE, &sndInfo);
  if (sndFile == NULL)
    {
      fprintf(stderr, "Error opening sound file '%s': %s\n", file_name.c_str(), sf_strerror(sndFile));
      return true;
    }
  LOCK;
  samplepos = 0;
  while (numframes > 0)
    {
      readsize = numframes < 4096? numframes : 4096;
      ret = pshared_data->pad->get_data(samplepos, snd_buffer, readsize);
      writtenFrames = sf_writef_float(sndFile, snd_buffer, ret.obtained);
      if (ret.obtained != writtenFrames)
	{
	  printf("Wave file writing error.\n");
	  UNLOCK;
	  return true;
	}
      samplepos += writtenFrames;
      numframes -= writtenFrames;
    }
  UNLOCK;
  return false;
}

bool Cappdata::load_sound_buffer(string file_name)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  SF_INFO   sndInfo;
  SNDFILE  *sndFile;
  int       numframes;
  int       readframes;
  int       readsize;
  float     snd_buffer[4096];
  double    endtimecode;

  play_record_enabled(false);
  LOCK;
  play_practice_enabled_in_locked_area(false);
  UNLOCK;
  m_opened_a_file = true;
  // Open sound file
  sndFile = sf_open(file_name.c_str(), SFM_READ, &sndInfo);
  if (sndFile == NULL)
    {
      fprintf(stderr, "Error reading source file '%s': %s\n", file_name.c_str(), sf_strerror(sndFile));
      return true;
   }
  // Check format - 16bit PCM
  if (sndInfo.format != (SF_FORMAT_WAV | SF_FORMAT_PCM_16))
    {
      fprintf(stderr, "Input should be 16bit Wav\n");
      sf_close(sndFile);
      return true;
   }
   // Check channels - mono
  if (sndInfo.channels != 1)
    {
      fprintf(stderr, "Wrong number of channels\n");
      sf_close(sndFile);
      return true;
   }
  // Free the previously recorded data and clear the track image
  clear_sound();
  LOCK;
  // Read the sound from the wav file
  numframes = sndInfo.frames;
  while (numframes > 0)
    {
      readsize = numframes < 4096? numframes : 4096;
      readframes = sf_readf_float(sndFile, snd_buffer, readsize);
      pshared_data->pad->add_data(snd_buffer, readframes);
      numframes -= readframes;
    }
  numframes = pshared_data->pad->get_samplenum();
  // Reset timecodes in the shared memory
  endtimecode = (double)numframes / (double)pshared_data->samplerate;
  // endtimecode = pshared_data->viewtime;
  {
    pshared_data->timecode = endtimecode;
    pshared_data->trackend = endtimecode;
    pshared_data->lastimgtimecode = endtimecode;
    pshared_data->bspectre_img_updated = false; // Force spectrum rewrite test
  }
  UNLOCK;
  // Check correct number of samples loaded
  if (numframes != sndInfo.frames)
    {
      fprintf(stderr, "Did not read enough frames for source\n");
      sf_close(sndFile);
      return true;
    }
  sf_close(sndFile);
  // Update the spectrogram image
  wakeup_spectrogram_thread();
  return false;
}

