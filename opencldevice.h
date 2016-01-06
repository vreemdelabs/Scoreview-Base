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

class Copencldevice
{
 protected:
  cl_program build_progam(int numfiles, char **filenames, cl_device_id *pdevice, cl_context *pcontext);
  void print_device_info_string(cl_device_id* device, cl_device_info infID, char *printstring, int j);
  int  find_opencl_device(cl_device_type  device_type);
  void print_enqueNDrKernel_error(cl_int err);
  void print_enqueReadBuffer_error(cl_int err);

 protected:
  cl_device_id     m_device;
  cl_context       m_context;
};

