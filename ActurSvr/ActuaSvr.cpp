//////////////////////////////////////////////////////////////////////////////
//This file was created by modifying the file,
//qp/examples/qp/qp_dpp_qk/philo.cpp received from Quantum Leaps, LLC.
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

#include "qDevice.h"

Q_DEFINE_THIS_FILE

//QDevice objects -------------------------------------------------------------
/*
   Add all devices here.
   Count number of devices, and MODIFY macro 'TOTAL_OF_DEV' definition in qDevice.h.
 */

static CmdPump l_cp(0); //CmdPump objects(Do not delete!)
static LEDgroup ledgroup(1,9,11);

//Pointer that this application depends ---------------------------------------
CmdPump* p_cp = &l_cp; //(Do not delete!)
//ServoTact* p_tact = &;

//////////////////////////////////////////////////////////////////////////////
