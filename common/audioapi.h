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

typedef struct s_pa_device
{
  std::string  name;
  int          device_api_code;
  int          inputs;
  int          outputs;
}              t_pa_device;

typedef struct            s_portaudio_api
{
  std::string             name;
  std::list<t_pa_device>  device_list;
}                         t_portaudio_api;

typedef struct  s_channel_select_strings
{
  std::string   api_name;
  std::string   in_device_name;
  std::string   out_device_name;
}               t_channel_select_strings;

