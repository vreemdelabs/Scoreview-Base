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

#include <iterator>
#include <vector>
#include <list>
#include <vector>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <GL/gl.h>

#include <tinyxml.h>

#include "env.h"
#include "audiodata.h"
#include "audioapi.h"
#include "shared.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "score.h"
#include "scorefile.h"
#include "f2n.h"
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
#include "cardnames.h"
#include "messages.h"
#include "messages_network.h"
//#include "messages_decoder.h"
#include "app.h"
#include "sdlcalls.h"

void Cappdata::create_cards_layout()
{
  Ccard    *pc;
  t_fcoord  pos, dim;
  t_coord   wsize;
  Cgfxarea area(pos, dim, wsize, (char*)"empty");
  string   title;
  string   text;
  string   basedir;
  string   cardimg;
  string   cardcontour;
  TTF_Font     *title_font;
  TTF_Font     *font;
  SDL_Surface  *contour;
  SDL_Surface  *transparent_surface;
  Uint32       *pixels;
  int           i;
  float         xposoffset;
  float         xpos;
  bool          bhelper;

  pos.z = dim.z = 0;
  wsize.z = 0;
  //
  xposoffset = 7.;
  wsize.x = m_width;
  wsize.y = m_height;
  m_cardlayout = new CCardList();
  //
  basedir = string(IMGPATH);
  // % of size
  dim.x = 4.4;
  dim.y = 6.2;

  //-------------------------------------------------------------------
  // Top
  //-------------------------------------------------------------------
  // Recording
  float hypos = 0.4;
  set_coord(&pos, 3., hypos);
  area = Cgfxarea(pos, dim, wsize, (char*)CARD_RECORD);
  title = "Recording";
  cardimg = basedir + string("cardrecord.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives);
  pc->add_info_line(string("State"));
  pc->add_info_line(string("Records the input source"));
  pc->add_info_line(string("selected in the config"));
  pc->add_info_line(string("dialog."));
  pc->add_info_line(string("(key: space bar)"));
  //pc->activate_card(true);
  m_cardlayout->add(pc);

  // Dialog: Save/Open the work and track
  set_coord(&pos, 10., hypos);
  area = Cgfxarea(pos, dim, wsize, (char*)CARD_OPENSAVE);
  title = "Open/Save dialog";
  cardimg = basedir + string("cardopensave.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives);
  pc->add_info_line(string("Action"));
  pc->add_info_line(string("Opens a dialog allowing to"));
  pc->add_info_line(string("open or to save the work."));
  m_cardlayout->add(pc);

  // Changes the spectrometer display
  xpos = 73.;

  set_coord(&pos, xpos, hypos);
  area = Cgfxarea(pos, dim, wsize, (char*)CARD_LOGVIEW);
  title = "Spectrogram change";
  cardimg = basedir + string("cardlogsp.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives);
  pc->add_info_line(string("State"));
  pc->add_info_line(string("When activated, the"));
  pc->add_info_line(string("spectrogram is in decibels"));
  pc->add_info_line(string("instead of enhanced"));
  pc->add_info_line(string("details mode."));
  m_cardlayout->add(pc);
  xpos += xposoffset;

  // Disable the help system
  set_coord(&pos, xpos, hypos);
  area = Cgfxarea(pos, dim, wsize, (char*)CARD_HELPERS);
  title = "Infos";
  cardimg = basedir + string("cardhelp.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives);
  pc->add_info_line(string("State"));
  pc->add_info_line(string(" "));
  pc->add_info_line(string("Displays helper cards "));
  pc->add_info_line(string("on the interface."));
  m_cardlayout->add(pc);
  xpos += xposoffset;

  // Disable the help system
  set_coord(&pos, xpos, hypos);
  area = Cgfxarea(pos, dim, wsize, (char*)CARD_CONFIG);
  title = "Configuration";
  cardimg = basedir + string("cardconfig.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives);
  pc->add_info_line(string("Action"));
  pc->add_info_line(string("Starts the config dialog"));
  pc->add_info_line(string(" "));
  pc->add_info_line(string("- general configuration"));
  pc->add_info_line(string("- input selection"));
  m_cardlayout->add(pc);
  xpos += xposoffset;

  //-------------------------------------------------------------------
  // Bottom left
  //-------------------------------------------------------------------
  hypos = 93;
  xpos = 3.;

  // Dialog: Add an instrument
  set_coord(&pos, xpos, hypos);
  area = Cgfxarea(pos, dim, wsize, (char*)CARD_ADDINSTR);
  title = "Add an instrument";
  cardimg = basedir + string("cardaddinstr.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives);
  pc->add_info_line(string("Action"));
  pc->add_info_line(string(" "));
  pc->add_info_line(string("Add an instrument to"));
  pc->add_info_line(string("the composition.")); 
  m_cardlayout->add(pc);
  xpos += xposoffset;

/*
  // Stop on notes during practice
  set_coord(&pos, xpos, hypos);
  area = Cgfxarea(pos, dim, wsize, (char*)CARD_NOTEBLOCK);
  title = "Note barriers";
  cardimg = basedir + string("notebloc.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives, m_sdlRenderer);
  pc->add_info_line(string("State"));
  pc->add_info_line(string("When parcticing, the correct"));
  pc->add_info_line(string("notes must be played in"));
  pc->add_info_line(string("order to continue to the"));
  pc->add_info_line(string("next note."));
  m_cardlayout->add(pc);
  xpos += xposoffset;
*/
  
  // Practice session
  set_coord(&pos, xpos, hypos);
  area = Cgfxarea(pos, dim, wsize, (char*)CARD_PRACTICE);
  title = "Practice";
  cardimg = basedir + string("cardpractice.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives);
  pc->add_info_line(string("Action"));
  pc->add_info_line(string(" "));
  pc->add_info_line(string("Starts / Stops a practice"));
  pc->add_info_line(string("session."));
  m_cardlayout->add(pc);
  xpos += xposoffset;

  // Open a view of the notes on the instrument
  set_coord(&pos, xpos, hypos);
  area = Cgfxarea(pos, dim, wsize, (char*)CARD_PLACEMENT);
  title = "Finger placement";
  cardimg = basedir + string("cardpracticew.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives);
  pc->add_info_line(string("Action"));
  pc->add_info_line(string(" "));
  pc->add_info_line(string("Opens a window showing"));
  pc->add_info_line(string("where to play the notes"));
  pc->add_info_line(string("on the real instrument."));
  m_cardlayout->add(pc);
  xpos += xposoffset;

  // Practice session
  set_coord(&pos, xpos, hypos);
  area = Cgfxarea(pos, dim, wsize, (char*)CARD_PRACSPEED);
  title = "Slow down";
  cardimg = basedir + string("cardpracticespeed.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives);
  pc->add_info_line(string("State"));
  pc->add_info_line(string(" "));
  pc->add_info_line(string("Halfs practice speed."));
  m_cardlayout->add(pc);
  xpos += xposoffset;

  //
  set_coord(&pos, xpos, hypos);
  area = Cgfxarea(pos, dim, wsize, (char*)CARD_AUTOBEAM);
  title = "Auto beam";
  cardimg = basedir + string("cardautobeam.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives);
  pc->add_info_line(string("State"));
  pc->add_info_line(string(" "));
  pc->add_info_line(string("The notes between bars"));
  pc->add_info_line(string("are automaticly"));
  pc->add_info_line(string("connected and"));
  pc->add_info_line(string("the rests are displayed."));
  if (m_card_states.autobeam)
    pc->activate_card(true);
  m_cardlayout->add(pc);
  xpos += xposoffset;

  //
  set_coord(&pos, xpos, hypos);
  area = Cgfxarea(pos, dim, wsize, (char*)CARD_CHORDFUSE);
  title = "Chord creation";
  cardimg = basedir + string("cardchordfuse.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives);
  pc->add_info_line(string("State"));
  pc->add_info_line(string(" "));
  pc->add_info_line(string("Overlaping notes can be"));
  pc->add_info_line(string("fused into chords."));
  if (m_card_states.chordfuse)
    pc->activate_card(true);
  m_cardlayout->add(pc);
  xpos += xposoffset;

  //-------------------------------------------------------------------
  // Hidden help cards
  //-------------------------------------------------------------------
  bhelper = true;

  // Spectrometer
  set_coord(&pos, 20, 60 - dim.y / 2);
  area = Cgfxarea(pos, dim, wsize, (char*)"spectre");
  title = "Spectrometer";
  cardimg = basedir + string("cardspectre.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives, bhelper);
  pc->add_info_line(string("Helper"));
  pc->add_info_line(string("- select: play a zone"));
  pc->add_info_line(string("- R click: add a note"));
  pc->add_info_line(string("- e + L click: add a bar"));
  pc->add_info_line(string("- w + L click: play a note"));
  pc->add_info_line(string("- wheel: fequency zoom"));
  m_cardlayout->add(pc);

  // Score
  set_coord(&pos, 15 + dim.x / 2, 95 - dim.y / 2);
  area = Cgfxarea(pos, dim, wsize, (char*)"score");
  title = "Score";
  cardimg = basedir + string("cardscore.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives, bhelper);
  pc->add_info_line(string("Helper"));
  pc->add_info_line(string("- select: practice area"));
  pc->add_info_line(string("          play the notes"));
  //pc->add_info_line(string("s + select: play the track without the notes"));
  pc->add_info_line(string("- wheel:  zoom"));
  pc->add_info_line(string("- s + wheel:  string selection"));
  pc->add_info_line(string("- d + select: delete "));
  m_cardlayout->add(pc);

  // Time bar
  set_coord(&pos, 50 - dim.x / 2, 75 - dim.y / 2);
  area = Cgfxarea(pos, dim, wsize, (char*)"timescale");
  title = "Time scale";
  cardimg = basedir + string("cardtscale.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives, bhelper);
  pc->add_info_line(string("Helper"));
  pc->add_info_line(string("- Mouse wheel: time zoom,"));
  pc->add_info_line(string("  changes the view time"));
  pc->add_info_line(string("- Right click: grab"));
  pc->add_info_line(string("(The blue peaks are the"));
  pc->add_info_line(string(" note attacks)"));
  m_cardlayout->add(pc);

  // Freq bar
  set_coord(&pos, 70 - dim.x, 60 - dim.y / 2);
  area = Cgfxarea(pos, dim, wsize, (char*)"freqscale");
  title = "Frequency scale";
  cardimg = basedir + string("cardfscale.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives, bhelper);
  pc->add_info_line(string("Helper"));
  pc->add_info_line(string("- Right click: change from"));
  pc->add_info_line(string("  frequency scale to piano"));
  pc->add_info_line(string("  keys"));
  pc->add_info_line(string("- Mouse wheel: frequency"));
  pc->add_info_line(string("  zoom"));
  m_cardlayout->add(pc);

  // Track visualisation
  set_coord(&pos, 50, 50 /*- dim.y / 2*/);
  area = Cgfxarea(pos, dim, wsize, (char*)"track");
  title = "Track view";
  cardimg = basedir + string("cardtrack.png");
  pc = new Ccard(area, title, cardimg, m_gfxprimitives, bhelper);
  pc->add_info_line(string("Helper"));
  pc->add_info_line(string("- Right click: select the"));
  pc->add_info_line(string("  timecode."));
  pc->add_info_line(string("- Mouse wheel: time zoom"));
  pc->add_info_line(string("- Left click: listen"));
  m_cardlayout->add(pc);

  //-------------------------------------------------------------------
  // Draw all the cards in a texture
  //-------------------------------------------------------------------
  title_font = TTF_OpenFont(DATAPATH BIGFONT, 36);
  font = TTF_OpenFont(DATAPATH BIGFONT, 32);
  cardcontour = basedir + string("contour.png");
  try
    {
      contour = IMG_Load(cardcontour.c_str());
      if (contour == NULL)
	throw 1;
    }
  catch (int err)
    {
      printf("User interface Error: could not load the images from \"%s\".\n", cardcontour.c_str());
      exit(1);
    }
  transparent_surface = SDL_ConvertSurfaceFormat(contour, SDL_PIXELFORMAT_ARGB8888, 0);
  SDL_FreeSurface(contour);
  pixels = (Uint32*)transparent_surface->pixels;
  for (i = 0; i < transparent_surface->w * transparent_surface->h; i++)
    {
      if (pixels[i] == 0xFF30FF00)
	pixels[i] = pixels[i] & 0x00FFFFFF;
    }
  pc = m_cardlayout->get_first();
  while (pc != NULL)
    {
      pc->create_texture(title_font, font, transparent_surface);
      pc = m_cardlayout->get_next();
    }
  SDL_FreeSurface(transparent_surface);
  TTF_CloseFont(font);
  TTF_CloseFont(title_font);
}

void Cappdata::render_cards()
{
  Ccard    *pc;

  pc = m_cardlayout->get_first();
  while (pc != NULL)
    {
      pc->display_card_reduced(m_width, m_height, m_card_states.bhelpers_enabled, m_last_time);
      pc = m_cardlayout->get_next();
    }
  pc = m_cardlayout->get_first();
  while (pc != NULL)
    {
      pc->display_card_big(m_width, m_height, m_card_states.bhelpers_enabled, m_last_time);
      pc = m_cardlayout->get_next();
    }
}

void Cappdata::check_card_inspection(int x, int y, erclick brclick, bool bmove)
{
  t_coord  pos;
  Ccard    *pc;

  pos.x = x;
  pos.y = y;
  pc = m_cardlayout->get_first();
  while (pc != NULL)
    {
      if (pc->is_helper())
	{
	  if (m_layout->is_in(x, y, (char*)pc->get_name().c_str()))
	    {
	      pc->mouse_over(true, m_last_time, erclickunknown, bmove);
	    }
	  else
	    pc->mouse_over(false, m_last_time, erclickunknown, bmove);
	}
      else
	{
	  pc->mouse_over(pc->is_in(pos), m_last_time, brclick, bmove);
	}
      pc = m_cardlayout->get_next();
    }
}

void Cappdata::card_mousemove(int x, int y)
{
  check_card_inspection(x, y, erclickunknown, true);
}

// On right click, just show the card
void Cappdata::card_mouseclickright(int x, int y)
{
  check_card_inspection(x, y, erclickstart);
}

void Cappdata::card_mouseupright(int x, int y)
{
  check_card_inspection(x, y, erclickstop);
}

void Cappdata::enable_card_function(Ccard *pc)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  bool     bactive, bactivenext;
  double   timecode, endtimecode, viewtime, duration, practicespeed;

  bactive = pc->is_activated();
  bactivenext = !bactive;
  if (pc->same_name((char*)CARD_PRACTICE))
    {
      if (bactivenext)
	{
	  // Fixme: should send an internal message elsewhere because this function is called from a lot of places
	  LOCK;
	  play_practice_enabled_in_locked_area(true);
	  timecode = pshared_data->timecode;
	  endtimecode = pshared_data->trackend;
	  viewtime = pshared_data->viewtime;
	  practicespeed = pshared_data->practicespeed;
	  UNLOCK;
	  if (m_appstate == statepractice)
	    {
	      send_practice_message(practice_update_countdown, timecode - viewtime / 2, endtimecode, MAXNOTEF, MINNOTEF, viewtime / 2, practicespeed);
	    }
	  else
	    {
	      if (m_appstate == statepracticeloop)
		{
		  // This will only start the countdown, the effective practice event be sent after the end of the timeout and the filtering
		  duration = m_note_selection.box.stop_tcode - m_note_selection.box.start_tcode;
		  send_practice_message(practice_area_loop_countdown, m_note_selection.box.start_tcode, m_note_selection.box.stop_tcode,
					m_note_selection.box.hif, m_note_selection.box.lof, duration, practicespeed);
		  // Stop playing the selection, and flush all the playing audio strips
		  stop_note_selection(&m_note_selection);
		}
	    }
	}
      else
	{
	  LOCK;
	  timecode = pshared_data->timecode;
	  endtimecode = pshared_data->trackend;
	  viewtime = pshared_data->viewtime;
	  practicespeed = pshared_data->practicespeed;
	  UNLOCK;
	  if (m_appstate == statepracticeloop)
	    {
	      // Stop playing the selection, and flush all the playing audio strips
	      stop_note_selection(&m_note_selection);
	    }
	  send_practice_message(practice_stop_at, timecode - viewtime, endtimecode, MAXNOTEF, MINNOTEF, viewtime, practicespeed);
	  LOCK;
	  play_practice_enabled_in_locked_area(false);
	  UNLOCK;
	}
    }
  if (pc->same_name((char*)CARD_HELPERS))
    {
      pc->activate_card(bactivenext);
      // This card enables helper cards
      m_card_states.bhelpers_enabled = bactivenext;
    }
  if (pc->same_name((char*)CARD_LOGVIEW))
    {
      pc->activate_card(bactivenext);
      LOCK;
      pshared_data->blogdb = bactivenext;
      UNLOCK;
    }
  if (pc->same_name((char*)CARD_AUTOBEAM))
    {
      pc->activate_card(bactivenext);
      m_card_states.autobeam = bactivenext;
      m_pScorePlacement->set_autobeam(m_card_states.autobeam);
    }
  // Do not care of other cards if in practicing
  if (m_appstate == statepractice || m_appstate == statepracticeloop)
    return;
  //
  if (pc->same_name((char*)CARD_PRACSPEED))
    {
      LOCK;
      pshared_data->practicespeed = bactivenext? 1. / 2. : 1.;
      UNLOCK;
      pc->activate_card(bactivenext);
    }
  if (pc->same_name((char*)CARD_RECORD))
    {
      play_record_enabled(bactivenext);
    }
  if (pc->same_name((char*)CARD_CONFIG))
    {
      if (bactivenext)
	{
	  printf("opening config dialog\n");
	  m_pserver->Send_to_dialogs(message_open_config_dialog, "", 0);
	  pc->activate_card(bactivenext);
	}
    }
  // Action disabled by a return message only
  if (pc->same_name((char*)CARD_OPENSAVE))
    {
      if (bactivenext)
	{
	  m_pserver->Send_to_dialogs(message_open_storage_dialog, "", 0);
	  pc->activate_card(bactivenext);
	}
    }
  if (pc->same_name((char*)CARD_ADDINSTR))
    {
      if (bactivenext)
	{
	  m_pserver->Send_to_dialogs(message_open_addinstrument_dialog, "", 0);
	  pc->activate_card(bactivenext);
	}
    }
  if (pc->same_name((char*)CARD_NOTEBLOCK))
    {
      pc->activate_card(bactivenext);
    }
  if (pc->same_name((char*)CARD_PLACEMENT))
    {
      if (bactivenext)
	{
	  m_pserver->Send_to_dialogs(message_open_practice_dialog, "", 0);
	  pc->activate_card(bactivenext);
	}
    }
  if (pc->same_name((char*)CARD_CHORDFUSE))
    {
      pc->activate_card(bactivenext);
      m_card_states.chordfuse = bactivenext;
      m_pScoreEdit->set_chord_fuse(m_card_states.chordfuse);
    }
}

void Cappdata::card_mouseclickleft(int x, int y)
{
  Ccard    *pc;
  t_coord  pos;

  pos.x = x;
  pos.y = y;
  pc = m_cardlayout->get_first();
  while (pc != NULL)
    {
      if (pc->is_in(pos) && !pc->is_helper())
	{
	  enable_card_function(pc);
	}
      pc = m_cardlayout->get_next();
    }
}

void Cappdata::toggle_card(const char *card_name)
{
  Ccard *pc;

  pc = m_cardlayout->get_card((char*)card_name);
  if (pc != NULL)
    {
      pc->activate_card(!pc->is_activated());
    }
}

void Cappdata::set_record_card(bool bactive)
{
  Ccard *pc;

  pc = m_cardlayout->get_card((char*)CARD_RECORD);
  if (pc != NULL)
    {
      pc->activate_card(bactive);
    }
}


void Cappdata::send_start_stop(audio_track_command cmd)
{
  t_shared         *pshared_data = (t_shared*)&m_shared_data;
  t_audio_track_cmd acmd;
  int               i;
  bool              bpurge;

  LOCK;
  acmd.v = 0;
  acmd.command = cmd;
  acmd.direction = ad_app2threadaudio;
  pshared_data->audio_cmds.push_back(acmd);
  UNLOCK;
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

void Cappdata::play_record_enabled(bool enabled)
{
  if (!enabled && m_opened_a_file && m_bdonotappendtoopen)
    return ;
  if (enabled)
    {
      set_practice_card(false);
      send_start_stop(ac_record);
    }
  else
    {
      send_start_stop(ac_stop);
    }
  set_record_card(enabled);
}

void Cappdata::set_practice_card(bool bactive)
{
  Ccard *pc;

  pc = m_cardlayout->get_card((char*)CARD_PRACTICE);
  if (pc != NULL)
    {
      pc->activate_card(bactive);
    }
}

void Cappdata::play_practice_enabled_in_locked_area(bool enabled)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  bool      bdisable_record_card;
  double    interval;

  bdisable_record_card = false;
  if (enabled)
    {
      bdisable_record_card = (pshared_data->play_State == state_record);
      pshared_data->practice_countdown_tick = m_last_time;
      // Check if loop practice, if notes are selected
      if (m_note_selection.cmdlist.size() > 0)
	{
	  interval = m_note_selection.box.stop_tcode - m_note_selection.box.start_tcode;
	  pshared_data->practice_start = pshared_data->timecode;
	  pshared_data->timecode = m_note_selection.box.stop_tcode + (pshared_data->viewtime - interval) / 2.; // Center the view on the box
	  pshared_data->play_State = state_wait_practiceloop;
	  pshared_data->practice_loop_duration = interval;
	  m_appstate = statepracticeloop;
	}
      else
	{
	  if (pshared_data->timecode == pshared_data->trackend)
	    pshared_data->timecode = 0;
	  pshared_data->practice_start = pshared_data->timecode;
	  pshared_data->play_State = state_wait_practice;
	  m_appstate = statepractice;
	}
    }
  else
    {
      pshared_data->timecode = pshared_data->practice_start;
      pshared_data->play_State = state_stop;
      m_appstate = statewait;
    }
  set_practice_card(enabled);
  if (bdisable_record_card)
    set_record_card(false);
}

