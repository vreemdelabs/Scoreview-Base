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
#include <list>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/gl.h>

#include <tinyxml.h>

#include "env.h"
#include "audiodata.h"
#include "audioapi.h"
#include "cfgfile.h"
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
#include "scoreedit_piano.h"
#include "scorerenderer.h"
#include "card.h"
#include "messages.h"
#include "tcp_ip.h"
#include "messages_network.h"
#include "app.h"
#include "sdlcalls.h"

// FIXME should handle starts and stops and buttons
void Cappdata::timed_events()
{
  m_pserver->Send_to_dialogs_delayed_messages(m_last_time);
}
