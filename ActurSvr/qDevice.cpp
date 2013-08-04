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

#include "qDevice.h"
#include "Arduino.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

using namespace QP;

Q_DEFINE_THIS_FILE
//............................................................................
extern QActive* dev_tbl[];

extern unsigned long start_time;
extern unsigned long passed_time;
extern uint8_t reent;
extern bool trig1;
extern bool trig2;
extern bool update;
extern SI* p_ao0;

//BEGIN OF COMMAND TOKEN DEFINITIONS////////////////////////////////////////
#define QD_TOKEN(pref, suf, str) char pref ## _tok_ ## suf[] PROGMEM = #str;
//Store string in the program memory
QD_TOKEN(err, 0, E000)
QD_TOKEN(err, 1, E001)
QD_TOKEN(err, 2, E002)
QD_TOKEN(err, 3, E003)
QD_TOKEN(err, 4, E004)
QD_TOKEN(err, 5, E005)
QD_TOKEN(si,  6, hello)
QD_TOKEN(led, 7, itvl)

#define QD_TOKEN(pref, suf, str) pref ## _tok_ ## suf,
//Pointer array from here
PGM_P QDevice::tok_tbl[] PROGMEM = {
QD_TOKEN(err, 0, E000)
QD_TOKEN(err, 1, E001)
QD_TOKEN(err, 2, E002)
QD_TOKEN(err, 3, E003)
QD_TOKEN(err, 4, E004)
QD_TOKEN(err, 5, E005)
QD_TOKEN(si,  6, hello)
QD_TOKEN(led, 7, itvl)
};
//END OF COMMAND TOKEN DEFINITIONS/////////////////////////////////////////

DL_Storage::DL_Storage() {
	m_pool.init(dListSto, sizeof(dListSto), sizeof(dListSto[0]));
	blk_cnt = 0;
}

uint8_t DL_Storage::BlockCount() {
	return blk_cnt;
}

Data_List* DL_Storage::getDLBlock() {
	Data_List* dp;
	if (dp = ((Data_List*)m_pool.get())) {
		++blk_cnt;
	}
	return dp;
}

void DL_Storage::putDLBlock(Data_List* dp) {
	m_pool.put(dp);
	--blk_cnt;
}

QDevice::QDevice(uint8_t id, QDcmdHandler p, QStateHandler h, uint8_t ts, uint8_t te)
	:
	QActive(h),
	devID(id),
	clbkfunc(p),
	tok_start(ts),
	tok_end(te){
	dev_tbl[ id & 0x3F ] = this;
}

uint8_t QDevice::getID() {
	return devID & 0x3F;
}

uint8_t DL_Queue::ListCount() {
	return List_cnt;
}

void DL_Queue::EnqueueList(Data_List* lp) {
	QF_INT_DISABLE();
	if(last == static_cast<Data_List *>(0)) {
		last  = lp;
		first = last;
	}
	else {
		last->next = lp;
		last = last->next;
	}

	last->next = static_cast<Data_List *>(0);
	++List_cnt;
	QF_INT_ENABLE();
};

Data_List* DL_Queue::DequeueList() {
	QF_INT_DISABLE();
	if(first != static_cast<Data_List *>(0)) {

		Data_List *temp = first;

		first = static_cast<Data_List *>(0);

		if(temp->next != static_cast<Data_List *>(0)) {
			first = temp->next;
		}
		else {
			last = static_cast<Data_List *>(0);
		}

		--List_cnt;

		return temp;
	} else {
		return static_cast<Data_List *>(0);
	}
	QF_INT_ENABLE();
};

void DL_Queue::FlushQueue(DL_Storage* sto) {
	QF_INT_DISABLE();
	if(first == static_cast<Data_List *>(0)) {
		return;
	}

	Data_List* flush_list = first;
	Data_List* nextflush_list = static_cast<Data_List *>(0);

	do {

		nextflush_list = flush_list->next;
		sto->putDLBlock(flush_list);
		flush_list = nextflush_list;

	}
	while(flush_list != static_cast<Data_List *>(0));
	List_cnt = 0;
	QF_INT_ENABLE();
}

//............................................................................
SI::SI(uint8_t id)
	: QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial, 6, 6),
	m_keep_alive_timer(SI_CHK_ALIVE_SIG)
{
	stat_flg = 0x00;
	stat_flg |= STAY;
	stat_flg |= ALIVE;

	lstp  = m_sto.getDLBlock();
	rp  = lstp->d_blk.origin_str;
	ctrlcnt = 0;
	c = '\0';
}

void SI::writeStatus(uint8_t id, uint16_t val) {
	if (val > 1023) return;
	Data_List* ws  = m_sto.getDLBlock();
	(ws->d_blk).cmd_d.context = S_STATUS;
	(ws->d_blk).cmd_d.devid = id;
	(ws->d_blk).cmd_d.Val = val;
	out.EnqueueList(ws);
}

void SI::Execute() {
	uint8_t n;
	if (out.ListCount()) {
		SI::Write();
		return;
	} else if (work.ListCount()) {
		SI::Dispatch();
		return;
	} else if (n = Serial.available()) {
		SI::Read(n);
		return;
	}
}

void SI::Read(uint8_t n) {
	for(uint8_t i = 0; i < n; ++i) {

		c = Serial.read();

		if (IsEmgcy()) {

			if (c == '^') {
				QEvent* pe = Q_NEW(QEvent, SI_RETURN_SIG);
				this->POST(pe, this);
			}
		} else {
			if (ctrlcnt == -1) {
				*rp++ = c;
				if (c == '\n') {
					*rp = '\0';
					ctrlcnt = -2;
				} else if ((rp - (lstp->d_blk.origin_str)) > (cmdSIZE - 2)) {
					rp    = lstp->d_blk.origin_str;
					ctrlcnt = 0;
				}
			} else if (ctrlcnt == 0) {
				switch (c) {
				case '~':
					stat_flg &= ~STAY;
					stat_flg |= ALIVE;
					break;
				case '<':
					ctrlcnt = -1;
					*rp++ = c;
					break;
				case '*':
					ctrlcnt = 6;
					*rp++ = c;
					break;
				default:
					break;
				}
			} else if (ctrlcnt > 0) {
				*rp++ = c;
				if ((--ctrlcnt) == 0) ctrlcnt = -2;
			}

			if (ctrlcnt == -2) {
				work.EnqueueList(lstp);
				lstp  = m_sto.getDLBlock();
				rp    = lstp->d_blk.origin_str;
				ctrlcnt = 0;
			}
		}
	}
}

void SI::Dispatch() {
	Data_List    *dl      = work.DequeueList();
	char         *tp      = (dl->d_blk).origin_str;
	bool err      = false;
	char         *endp    = static_cast<char *>(0);
	char         *saveptr = static_cast<char *>(0);

	uint8_t err_no;

	Cmd_Data cpy_d = {0};

	switch (*tp) {
	case '<':
	{
		while(*tp != '\0') {
			cpy_d.chksum += *tp;
			++tp;
		}

		tp = (dl->d_blk).origin_str + 1;

		cpy_d.devid = strtol(tp, &endp, 10);
		uint8_t id = cpy_d.devid & 0x3F;

		if (tp == endp) {
			err = true;
			err_no = 0;
		}else if(id >= TOTAL_OF_DEV) {
			err = true;
			err_no = 1;
		}else if(dev_tbl[id] == static_cast<QActive *>(0)) {
			err = true;
			err_no = 2;
		}else if(!(tp = strchr((dl->d_blk).origin_str, ','))) {
			err = true;
			err_no = 3;
		}

		if (!err) {
			tp = strtok_r(++tp, ",\n", &saveptr);
			uint8_t i;
			for(i = 0; tp != static_cast<char *>(0); ++i) {
				if(i < 2) {
					strlcpy(cpy_d.tok[i], tp, 6);
				}
				tp = strtok_r(static_cast<char *>(0), ",\n", &saveptr);
			}

			cpy_d.Val = strtol(cpy_d.tok[1], &endp, 10);

			cpy_d.tok[0][5] = Seek(cpy_d.tok[0],
			                       ((QDevice*)dev_tbl[id])->tok_start,
			                       ((QDevice*)dev_tbl[id])->tok_end);

			if (cpy_d.tok[1] != endp) {
				cpy_d.context += USE_VAL;
			} else if (i == 2) {
				cpy_d.context += TWO_TOK;
				cpy_d.tok[1][5] = Seek(cpy_d.tok[0],
				                       ((QDevice*)dev_tbl[id])->tok_start,
				                       ((QDevice*)dev_tbl[id])->tok_end);
			}

			if (i > 2) {
				err = true;
				err_no = 4;
			} else if (!(SEND_CMD(id, &cpy_d))) {
				err = true;
				err_no = 5;
			}
		}

		if (err) {
			cpy_d.context = ERR;
			strlcpy_P(cpy_d.tok[0], (PGM_P)pgm_read_word(&(tok_tbl[err_no])), 5);
		} else {
			cpy_d.context = ECHO_SUM;
		}

		(dl->d_blk).cmd_d = cpy_d;
		out.EnqueueList(dl);
	}
	break;
	case '*':
	{
	}
	}
}

void SI::Write() {

	Data_List *dl = out.DequeueList();

	uint8_t res[4] = { 0 };

	switch((dl->d_blk).cmd_d.context) {
	case ERR:
	{
		Serial.write(((uint8_t*)(dl->d_blk).cmd_d.tok[0]), 4);
	}
	break;
	case S_STATUS:
	{
		res[0] = '@';
		*((uint16_t*)(res + 1)) = (dl->d_blk).cmd_d.Val;
		res[1] |= (((dl->d_blk).cmd_d.devid) << 2);
		res[3] = res[1] + res[2];
		Serial.write(res, 4);
	}
	break;
	case ECHO_SUM:
	{
		res[0] = '>';
		*((uint16_t*)(res + 1)) = (dl->d_blk).cmd_d.chksum;
		res[3] = res[1] + res[2];
		Serial.write(res, 4);
	}
	break;
	default:
	{
	}
	break;
	}

	m_sto.putDLBlock(dl);
}

int8_t SI::Seek(const char* p, uint8_t offset, uint8_t te) {
	uint8_t total = te - offset + 1;
	if (total) {
		for (uint8_t i = 0; i < total; ++i) {
			if(!strcasecmp_P(p, (PGM_P)pgm_read_word(&(tok_tbl[i + offset])))) return i;
		}
	}
	return -1;
}

QState SI::initial(SI *me, QEvent const *e) {
	me->m_keep_alive_timer.postIn(me, 250);
	me->subscribe(SI_EMGCY_SIG);
	return Q_TRAN(&SI::Exchange);
}

QState SI::Exchange(SI *me, QEvent const *e) {
	switch (e->sig) {
	case Q_ENTRY_SIG:
	{
		return Q_HANDLED();
	}
	case SI_CHK_ALIVE_SIG:
	{
		if (!(me->stat_flg & STAY)) {
			if (me->stat_flg & ALIVE) {
				me->m_keep_alive_timer.postIn(me, 250);
				me->stat_flg &= ~ALIVE;
			} else {
				me->stat_flg |= EMGCY;
				QEvent* pe = Q_NEW(QEvent, SI_EMGCY_SIG);
				QF::publish(pe);
			}
		} else {
			me->m_keep_alive_timer.postIn(me, 250);
		}
		return Q_HANDLED();
	}
	case SI_EMGCY_SIG:
	{
		return Q_HANDLED();
	}
	case SI_RETURN_SIG:
	{
		Serial.print(me->c);

		me->stat_flg  = 0x00;
		me->stat_flg |= STAY;
		me->stat_flg |= ALIVE;
		me->rp        = me->lstp->d_blk.origin_str;
		me->c = '\0';
		QEvent* pe = Q_NEW(QEvent, SI_CHK_ALIVE_SIG);
		me->POST(pe, me);
		return Q_HANDLED();
	}
	}
	return Q_SUPER(&QHsm::top);
}

bool SI::CmdExecutor(SI* me, Cmd_Data* dat) {
	if (dat->context == ONE_TOK) {
		switch(dat->TOKEN1) {
		case hello:
			Serial.println("Hello, world!");
			return true;
		default:
			break;
		}
	}
	return false;
}
//............................................................................

LEDgroup::LEDgroup(uint8_t id, uint8_t s, uint8_t e, uint16_t itr)
	: QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial, 7, 7),
	m_timeEvt(TIMEOUT_SIG), s_pin(s), e_pin(e) {
	cur_pin = s;
	itrvl = itr;

	uint8_t i;
	for(i = s_pin; i <= e_pin; ++i) {
		pinMode(i, OUTPUT);
	}
}

QState LEDgroup::initial(LEDgroup *me, QEvent const *) {
	digitalWrite(me->cur_pin, HIGH);
	me->m_timeEvt.postIn(me, me->itrvl);
	return Q_TRAN(&LEDgroup::blinkForward);
}

QState LEDgroup::blinkForward(LEDgroup *me, QEvent const *e) {
	switch (e->sig) {
	case Q_ENTRY_SIG:
	{
		return Q_HANDLED();
	}
	case TIMEOUT_SIG:
	{
		trig1 = true;
		digitalWrite(me->cur_pin, LOW);
		++me->cur_pin;
		digitalWrite(me->cur_pin, HIGH);

		me->m_timeEvt.postIn(me, me->itrvl);

		if(me->cur_pin == me->e_pin) return Q_TRAN(&LEDgroup::blinkBackward);
		return Q_HANDLED();
	}
	}
	return Q_SUPER(&QHsm::top);
}

QState LEDgroup::blinkBackward(LEDgroup *me, QEvent const *e) {
	switch (e->sig) {
	case Q_ENTRY_SIG:
	{
		return Q_HANDLED();
	}
	case TIMEOUT_SIG:
	{
		delayMicroseconds(200);
		trig1 = true;
		digitalWrite(me->cur_pin, LOW);
		--me->cur_pin;
		digitalWrite(me->cur_pin, HIGH);

		me->m_timeEvt.postIn(me, me->itrvl);

		if(me->cur_pin == me->s_pin) return Q_TRAN(&LEDgroup::blinkForward);
		return Q_HANDLED();
	}
	}
	return Q_SUPER(&QHsm::top);
}

bool LEDgroup::CmdExecutor(LEDgroup* me, Cmd_Data* dat) {

	if (dat->context == USE_VAL) {
		switch (dat->TOKEN1) {
		case itvl:
			if(dat->Val > 0) {
				me->itrvl = dat->Val;
				me->m_timeEvt.rearm(me->itrvl);
				return true;
			}
		default:
			break;
		}
	}
	return false;
}



//............................................................................
