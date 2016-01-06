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

class CpictureList
{
 public:
  CpictureList(CGL2Dprimitives *primitives);
  ~CpictureList();

  void add_whole_image(std::string imagefile, std::string name);

  void open_interface_imgs(std::string imagefile);
  void open_practice_drawings(std::string imagefile);
  void open_instrument_tabs_drawings(std::string imagefile);
  void open_skin(std::string imagefile, std::string skin_name);

 private:
  void add_picture_from_area(t_coord  dim, SDL_Rect srcrect, SDL_Surface *img, std::string name);
  void add_picture(SDL_Surface *img, std::string name);
  SDL_Surface* img_load_ARGB(std::string imagefile);

 private:
  CGL2Dprimitives *m_gfxprimitives;
};

