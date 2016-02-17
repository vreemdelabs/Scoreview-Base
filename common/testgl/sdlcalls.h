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

int init_SDL(SDL_Window **psdlwindow, SDL_GLContext *GLContext, int x, int y, int width, int height);
int close_SDL(SDL_Window *psdlwindow, SDL_GLContext GLContext);

int register_sdl_user_event();

void handle_mouse_down(int button);
void handle_mouse_up(int button);
bool handle_input_events(bool *pmustquit, int *pscr_w, int *pscr_h);

