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

//#include <SDL2/SDL.h>
//#include <SDL2/SDL_ttf.h>

//#include <GL/gl.h>

#include "env.h"
#include "gfxareas.h"
#include "mesh.h"
/*
#include "gl2Dprimitives.h"
#include "score.h"
#include "f2n.h"
#include "pictures.h"
#include "keypress.h"
#include "instrumenthand.h"
#include "scoreplacement.h"
#include "scoreedit.h"
#include "scorerenderer.h"
*/
#include "./meshloader/meshloader.h"
#include "meshlist.h"

CMeshList::CMeshList()
{
}

CMeshList::~CMeshList()
{
  std::list<CMesh*>::iterator iter;

  iter = m_mesh_list.begin();
  while (iter != m_mesh_list.end())
    {
      delete (*iter);
      iter = m_mesh_list.erase(iter);
    }
}

bool CMeshList::Load_meshes_in_list(char *file_name)
{
  bool   ret;

  ret = load_meshes((char*)DATAPATH, file_name, &m_mesh_list);
  return ret;
}

CMesh* CMeshList::get_mesh(std::string name)
{
  std::list<CMesh*>::iterator iter;

  iter = m_mesh_list.begin();
  while (iter != m_mesh_list.end())
    {
      if ((*iter)->get_name() == name)
	return (*iter);
      iter++;
    }
  return NULL;
}

