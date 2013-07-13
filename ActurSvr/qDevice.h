//////////////////////////////////////////////////////////////////////////////
//This file is newly created and added by me.
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

#ifndef qDevice_h
#define qDevice_h

#include "qp_port.h"
#include <stdlib.h>

using namespace QP;

#define PROC_id        0
#define TOTAL_OF_DEV   2
#define cmdSIZE       17
#define stoSIZE      30

enum qdSignals {
	SI_EMGCY_SIG = QP::Q_USER_SIG,
	MAX_PUB_SIG,
	MAX_SIG
};

enum InternalSignals {
	TIMEOUT_SIG = MAX_SIG,
	HIGH_SIG,
	LOW_SIG,
	DONE_SIG,
	FREQ_SIG,
	SI_END_LINE_SIG,
	SI_CHK_ALIVE_SIG,
	SI_RETURN_SIG
};

//command-option
#define ENQUEUE  64
#define DEQUEUE  128
#define FLUSH    192

#define ONE_TOK  0
#define TWO_TOK  2
#define USE_NUM  3

#define WAVE_DRIVE   0x11
#define HALF_STEP    0X07
#define FULL_STEP    0x33

#define STAY         0x01
#define ALIVE        0x02
#define CHKECHO      0x04
#define EMGCY        0x80

#define START() start_time = micros()
#define STOP() passed_time = micros() - start_time
#define SEND_CMD(id, dblk)  (((QDevice*)dev_tbl[(id)])->clbkfunc((QDevice*)dev_tbl[(id)], (dblk)))
#define GET_DL() (Data_List*)m_sto.m_pool.get()
#define PUT_DL(p) m_sto.m_pool.put((p))

struct Cmd_Data {
	uint8_t context;
	char tok[2][6];
	int16_t num;
};

union Data_Block {
	char origin_str[cmdSIZE];
	Cmd_Data cmd_d;
};

struct Data_List {
	Data_List*     next;
	Data_Block d_blk;
};

class DL_Storage {
public:
QMPool m_pool;
DL_Storage();
private:
Data_List dListSto[stoSIZE];
};
//............................................................................
class QDevice : public QP::QActive {
public:
static QActive* dev_tbl[];
static DL_Storage m_sto;
typedef     bool (*QDcmdHandler)(QDevice*, Data_Block*);

private:
const uint8_t devID;

Data_List*  first;
Data_List*  last;
uint8_t List_cnt;

public:
const QDcmdHandler clbkfunc;

QDevice(
        uint8_t,
        QDcmdHandler,
        QP::QStateHandler
        );

uint8_t    getID();
uint8_t    ListCount();
void       EnqueueList(Data_List*);
bool       DequeueCmd();
void       FlushQueue();
};
//............................................................................

class CmdPump : public QDevice {

public:
CmdPump(uint8_t);
void       On_ISR();

bool       IsEmgcy()    {
	return stat_flg & EMGCY;
};

private:
QP::QTimeEvt m_keep_alive_timer;
volatile uint8_t stat_flg;

Data_List* lstp;
char* rp;
char c;

static bool CmdExecutor(CmdPump*, Data_Block*);
static QP::QState initial (CmdPump *me, QP::QEvent const *e);
static QP::QState Exchange (CmdPump *me, QP::QEvent const *e);
};

//............................................................................

class LEDgroup : public QDevice {
private:
QP::QTimeEvt m_timeEvt;

public:
LEDgroup(
        uint8_t,
        uint8_t,
        uint8_t,
        uint16_t itr = 1000
        );

private:
const uint8_t s_pin;
const uint8_t e_pin;

uint8_t cur_pin;
uint16_t itrvl;

static bool CmdExecutor(LEDgroup*, Data_Block*);

static QP::QState initial (LEDgroup *me, QP::QEvent const *e);
static QP::QState blinkForward(LEDgroup *me, QP::QEvent const *e);
static QP::QState blinkBackward(LEDgroup *me, QP::QEvent const *e);
};
//............................................................................

#endif

