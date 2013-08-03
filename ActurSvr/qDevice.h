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
#include <avr/pgmspace.h>

using namespace QP;

#define TOTAL_OF_DEV  2
#define cmdSIZE       18
#define stoSIZE       30

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
	SI_CHK_ALIVE_SIG,
	SI_RETURN_SIG
};


//context
#define ONE_TOK    0
#define TWO_TOK    2
#define USE_VAL    3
#define S_STATUS   4
#define ERR        8
#define ECHO_SUM  16

#define WAVE_DRIVE   0x11
#define HALF_STEP    0X07
#define FULL_STEP    0x33

#define STAY         0x01
#define ALIVE        0x02
#define CHKECHO      0x04
#define EMGCY        0x80

#define START() do { \
		if (!reent) \
			start_time = micros(); \
} while(0)

#define STOP()  do { \
		if ((trig1 || trig2) && (reent == 0)) { \
			QF_INT_DISABLE(); \
			passed_time = micros() - start_time; \
			update = true; \
			trig1 = false; \
			trig2 = false; \
			QF_INT_ENABLE(); \
		} \
} while(0)

#define RESULT() do { \
		if (update) { \
			update = false; \
			Serial.print(passed_time); \
			Serial.write("us\n"); \
		} \
} while(0)

#define SEND_CMD(id, dblk)  (((QDevice*)dev_tbl[(id)])->clbkfunc((QDevice*)dev_tbl[(id)], (dblk)))
#define TOKEN1 tok[0][5]
#define TOKEN2 tok[1][5]

#define QD_TOKEN(pref, suf, str) str,

struct Cmd_Data {
	uint16_t chksum;
	uint8_t devid;
	uint8_t context;
	char tok[2][6];
	int16_t Val;
};

union Data_Block {
	char origin_str[cmdSIZE];
	Cmd_Data cmd_d;
};

struct Data_List {
	Data_List*  next;
	Data_Block d_blk;
};

class DL_Storage {
private:
uint8_t blk_cnt;
QMPool m_pool;
Data_List dListSto[stoSIZE];

public:
DL_Storage();
uint8_t    BlockCount();
Data_List* getDLBlock();
void       putDLBlock(Data_List*);
};

class DL_Queue {
private:
uint8_t List_cnt;
Data_List*  first;
Data_List*  last;

public:
uint8_t      ListCount();
void         EnqueueList(Data_List*);
Data_List*   DequeueList();
void         FlushQueue(DL_Storage*);
};
//............................................................................
class QDevice : public QP::QActive {
public:
typedef    bool (*QDcmdHandler)(QDevice*, Cmd_Data*);
static QActive* dev_tbl[];
static PGM_P tok_tbl[];
static DL_Storage m_sto;

QDcmdHandler const clbkfunc;
uint8_t const tok_start;
uint8_t const tok_end;

private:
uint8_t const devID;

public:
QDevice(
        uint8_t,
        QDcmdHandler,
        QP::QStateHandler,
        uint8_t = 0,
        uint8_t = 0
        );

uint8_t    getID();
};
//............................................................................

class SI : public QDevice {

friend class QP::QK;

public:
SI(uint8_t);

bool       IsEmgcy()    {
	return stat_flg & EMGCY;
};

void writeStatus(uint8_t, uint16_t);

private:

enum {
	QD_TOKEN(si, 0, hello)
};

static PGM_P err_code[];

QP::QTimeEvt m_keep_alive_timer;
volatile uint8_t stat_flg;

DL_Queue work;
DL_Queue out;

Data_List* lstp;
int8_t ctrlcnt;
char* rp;
char c;

void    Execute();
void Read(uint8_t);
void    Dispatch();
void    Write();
int8_t Seek(const char*, const uint8_t, const uint8_t);

static bool CmdExecutor(SI*, Cmd_Data*);
static QP::QState initial (SI *me, QP::QEvent const *e);
static QP::QState Exchange (SI *me, QP::QEvent const *e);
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
        uint16_t itr = 2500
        );

private:

enum {
	QD_TOKEN(led, 1, itvl)
};

const uint8_t s_pin;
const uint8_t e_pin;

uint8_t cur_pin;
uint16_t itrvl;

static bool CmdExecutor(LEDgroup*, Cmd_Data*);

static QP::QState initial (LEDgroup *me, QP::QEvent const *e);
static QP::QState blinkForward(LEDgroup *me, QP::QEvent const *e);
static QP::QState blinkBackward(LEDgroup *me, QP::QEvent const *e);
};
//............................................................................
#endif

