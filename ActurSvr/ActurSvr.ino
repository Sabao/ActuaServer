//////////////////////////////////////////////////////////////////////////////
//This file was created by modifying the file, 
//qp/examples/qp/qp_dpp_qk/qp_dpp_qk.ino received from Quantum Leaps, LLC.
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

#include "qp_port.h"
#include "bsp.h"
#include "qDevice.h"
#include <Servo.h>

using namespace QP;

QActive* dev_tbl[TOTAL_OF_DEV] = { NULL };

// Local-scope objects -------------------------------------------------------
static union SmallEvents {
  void      *e0;
  uint8_t   e1[sizeof(CommandEvt)];
} l_smlPoolSto[TOTAL_OF_DEV];

static QEvent const *l_DeviceQueueSto[TOTAL_OF_DEV][1];
static QSubscrList   l_subscrSto[MAX_PUB_SIG];

//............................................................................

void setup() {
  BSP_init();                                          // initialize the BSP
  QF::init();       // initialize the framework and the underlying RT kernel

  QF::poolInit(l_smlPoolSto, sizeof(l_smlPoolSto), sizeof(l_smlPoolSto[0]));
  QF::psInit(l_subscrSto, Q_DIM(l_subscrSto));     // init publish-subscribe

  for (uint8_t n = 0U; n < TOTAL_OF_DEV; ++n) {
    if(dev_tbl[n] != NULL) {
      dev_tbl[n]->start((uint8_t)(n + 1U),
      l_DeviceQueueSto[n], Q_DIM(l_DeviceQueueSto[n]),
      (void *)0, 0U);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// NOTE: Do not define the loop() function, because this function is
// already defined in the QP port to Arduino

