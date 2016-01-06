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

#include "keypress.h"

Ckeypress::Ckeypress()
{
  int i;

  // Keypressed events
  for (i = 0; i < (int)(sizeof(m_bkeypress) / sizeof(bool)); i++)
    {
      m_bkeypress[i] = false;
    }
}

void Ckeypress::key_on(int code)
{	
  m_bkeypress[code % 256] = true;
  //printf("%c on\n", code);
}

void Ckeypress::key_off(int code)
{
  m_bkeypress[code % 256] = false;
  //printf("%c off\n", code);
}

bool Ckeypress::is_pressed(int code)
{
  return (m_bkeypress[code % 256]);
}
