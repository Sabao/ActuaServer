//////////////////////////////////////////////////////////////////////////////
//This file was created by modifying the file, 
//qp/examples/qp/qp_dpp_qk/bsp.h received from Quantum Leaps, LLC.
//Sabao Akutsu. Mar. 27, 2013
// 
//Product : ActuaServer (On Arduino, Real-time control some cheap actuators)
//
//Copyright (C) 2002-2012 Quantum Leaps, LLC. All rights reserved.
//Copyright (C) 2013  Sabao Akutsu.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#ifndef bsp_h
#define bsp_h

#include <avr/io.h>                                                 // AVR I/O

                                                 // Sys timer tick per seconds
#define BSP_TICKS_PER_SEC    5000

void BSP_init(void);

//////////////////////////////////////////////////////////////////////////////
// NOTE: The CPU clock frequency F_CPU is defined externally for each
// Arduino board

#endif                                                                // bsp_h


