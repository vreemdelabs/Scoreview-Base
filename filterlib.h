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

void DSPCPPfilter_pass_band_Butterworth(float *psamples, int samplenum, int samplerate, float frequency, float bandwidth);
void DSPCPPfilter_low_pass_Butterworth(float *psamples, int samplenum, int samplerate, float cutfrequency);
void DSPCPPfilter_Bandpass_filter_poles(int samplerate, double frequency, double bandwidth, double *pcoef, int *stages);
void DSPCPPfilter_lowpass_filter_poles(int samplerate, double cutfrequency, double *pcoef, int *stages);

