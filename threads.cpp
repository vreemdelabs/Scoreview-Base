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
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <iterator>
#include <list>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <GL/gl.h>

#include "audiodata.h"
#include "audioapi.h"
#include "shared.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "score.h"
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
#include "messages.h"
#include "messages_network.h"
#include "app.h"
#include "thrspectrometre.h"
#include "thrbandpass.h"
#include "thrportaudio.h"
#include "thrdialogs.h"

int create_threads(Cappdata *app)
{
  int ret;

  try
    {
      //---------------------------------------------------
      // Opencl dft spectrometer and track data image thread
      //---------------------------------------------------
      ret = pthread_create(&app->m_threadspectrometre, NULL,
			   thr_spectrometre, (void*)&app->m_shared_data);
      if (ret)
	throw (ret);

      //---------------------------------------------------
      // Bandpass filtering thread
      //---------------------------------------------------
      ret = pthread_create(&app->m_threadbandpass, NULL,
			   thr_bandpass, (void*)&app->m_shared_data);
      if (ret)
	throw (ret);

      //---------------------------------------------------
      // Portaudio audio input thread
      //---------------------------------------------------
      ret = pthread_create(&app->m_threadaudio, NULL,
			   thr_portaudio, (void*)&app->m_shared_data);
      if (ret)
	throw (ret);

      //---------------------------------------------------
      // Dialog start/stop network server thread
      //---------------------------------------------------
      ret = pthread_create(&app->m_threaddialogs, NULL,
			   thr_dialogs, (void*)app->m_pserver);
      if (ret)
	throw (ret);
    }
  catch (int e)
    {
      fprintf (stderr, "%s", strerror(e));
      return 1;
    }
  return 0;
}

int release_threads(Cappdata *app)
{
  t_shared* pshared_data = &app->m_shared_data;
  int*      pretval;

  //---------------------------------------------------
  // Dialog start/stop thread (uses libevent messaging)
  //---------------------------------------------------
  app->m_pserver->Send_to_dialogs(message_close_dialogs, "", 0);
  //sleep(1);
  usleep(500000);
  app->m_pserver->Send_to_dialogs(message_quit_its_over, "", 0);
  pthread_join(app->m_threaddialogs, (void**)&pretval);
  if (*pretval)
    printf("Something in the Network dialog thread failed %d\n", *pretval);
  LOCK;
  pshared_data->bquit = true;
  UNLOCK;
  usleep(10000);
  //---------------------------------------------------
  // Opencl dft spectrometer image thread
  //---------------------------------------------------
  app->wakeup_spectrogram_thread();
  pthread_join(app->m_threadspectrometre, (void**)&pretval);
  if (*pretval)
    printf("Something in the Opencl code failed %d\n", *pretval);

  //---------------------------------------------------
  // Sound filtering thread
  //---------------------------------------------------
  app->wakeup_bandpass_thread();
  pthread_join(app->m_threadbandpass, (void**)&pretval);
  if (*pretval)
    printf("Something in the Bandpass thread failed %d\n", *pretval);

  //---------------------------------------------------
  // Portaudio audio input thread
  //---------------------------------------------------
  pthread_join(app->m_threadaudio, (void**)&pretval);
  switch(*pretval)
    {
    case PO_ERR_INIT:
      printf("Portaudio initialisation failed\n");
      break;
    case PO_ERR_NOINPUT:
      printf("No input device found\n");
      break;
    case PO_ERR_OPENS:
      printf("Opening audio input failed\n");
      break;
    case PO_ERR_OVERFLOW:
      printf("Audio buffer overflow\n");
      break;
    default:
      break;
    }
  return 0;
}

