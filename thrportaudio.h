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

#define PO_ERR_INIT      1
#define PO_ERR_NOINPUT   2
#define PO_ERR_OPENS     3
#define PO_ERR_OVERFLOW  4
#define PO_ERR_NOOUTPUT  5
#define PO_ERR_UNDERFLOW 6

// On 2015 linux mageia and ubuntu distributions, ALSA hangs at the first underrun, and it underruns on common peak loads (it was ok in 2014).
#ifdef __LINUX
#define FRAMES_PER_BUFFER (2048)
#else
#define FRAMES_PER_BUFFER (1024)
#endif

void* thr_portaudio(void *p_data);
