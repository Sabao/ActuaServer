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
extern SI* p_si;

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

QDevice::QDevice(uint8_t id, QDcmdHandler p, QStateHandler h)
	:
	QActive(h),
	devID(id), clbkfunc(p) {
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
	if(last == NULL) {
		last  = lp;
		first = last;
	}
	else {
		last->next = lp;
		last = last->next;
	}

	last->next = NULL;
	++List_cnt;
	QF_INT_ENABLE();
};

Data_List* DL_Queue::DequeueList() {
	QF_INT_DISABLE();
	if(first != NULL) {

		Data_List *temp = first;

		first = NULL;

		if(temp->next != NULL) {
			first = temp->next;
		}
		else {
			last = NULL;
		}

		--List_cnt;

		return temp;
	} else {
		return NULL;
	}
	QF_INT_ENABLE();
};

void DL_Queue::FlushQueue(DL_Storage* sto) {
	QF_INT_DISABLE();
	if(first == NULL) {
		return;
	}

	Data_List* flush_list = first;
	Data_List* nextflush_list = NULL;

	do {

		nextflush_list = flush_list->next;
		sto->putDLBlock(flush_list);
		flush_list = nextflush_list;

	}
	while(flush_list != NULL);
	List_cnt = 0;
	QF_INT_ENABLE();
}

//............................................................................
SI::SI(uint8_t id)
	: QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial),
	m_keep_alive_timer(SI_CHK_ALIVE_SIG)
{
	stat_flg = 0x00;
	stat_flg |= STAY;
	stat_flg |= ALIVE;

	lstp  = m_sto.getDLBlock();
	rp  = lstp->d_blk.origin_str;
	c = '\0';
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
		if (stat_flg & EMGCY) {
			c = Serial.read();
			if (c == '^') {
				QEvent* pe = Q_NEW(QEvent, SI_RETURN_SIG);
				this->POST(pe, this);
			}
		} else {

			c = Serial.read();

			if(c == '\0') { return; }

			*rp++ = c;

			if (c == '\n' || ((rp - (lstp->d_blk.origin_str)) > (cmdSIZE - 2))) {
				switch (*(lstp->d_blk.origin_str)) {
				case '<':
				case '(':
				{
					*rp = '\0';
					work.EnqueueList(lstp);
					lstp  = m_sto.getDLBlock();
					rp    = lstp->d_blk.origin_str;
					break;
				}
				case '~':
				{
					stat_flg &= ~STAY;
					stat_flg |= ALIVE;
					break;
				}
				default:
				{
					rp    = lstp->d_blk.origin_str;
					break;
				}
				}
			}
		}
	}
}

void SI::Dispatch() {
	Data_List    *dl      = work.DequeueList();
	char         *tp      = (dl->d_blk).origin_str;
	bool err      = false;
	char         *endp    = NULL;
	char         *saveptr = NULL;

	uint8_t err_no;
	char err_code[6][5] = {
		"?00\n",
		"?01\n",
		"?02\n",
		"?03\n",
		"?04\n",
		"?05\n"
	};

	Cmd_Data cpy_d = {0};

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
	}else if(dev_tbl[id] == NULL) {
		err = true;
		err_no = 2;
	}else if(!(tp = strchr((dl->d_blk).origin_str, ','))) {
		err = true;
		err_no = 3;
	}

	if (!err) {
		tp = strtok_r(++tp, ",\n", &saveptr);
		uint8_t i;
		for(i = 0; tp != NULL; ++i) {
			if(i < 2) {
				strncpy(cpy_d.tok[i], tp, 5);
			}
			tp = strtok_r(NULL, ",\n", &saveptr);
		}

		cpy_d.Val = strtol(cpy_d.tok[1], &endp, 10);

		if (cpy_d.tok[1] != endp) {
			cpy_d.context += USE_VAL;
		} else if (i == 2) {
			cpy_d.context += TWO_TOK;
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
		cpy_d.context = ONE_ARR;
		strcpy(cpy_d.tok[0], err_code[err_no]);
	} else {
		cpy_d.context = ECHO_SUM;
	}

	(dl->d_blk).cmd_d = cpy_d;
	out.EnqueueList(dl);
}

void SI::Write() {

	Data_List *dl = out.DequeueList();

	uint8_t res[5] = { '\0', '\0', '\0', '\n', '\n' };

	switch((dl->d_blk).cmd_d.context) {
	case ONE_ARR:
	{
		Serial.write((dl->d_blk).cmd_d.tok[0]);
	}
	break;
	case TWO_ARR:
	{
		Serial.write((dl->d_blk).cmd_d.tok[0]);
		Serial.write((dl->d_blk).cmd_d.tok[1]);
	}
	break;
	case SENSOR:
	{
		res[0] = '(';
		res[1] = (dl->d_blk).cmd_d.devid;
		*((uint16_t*)(res + 2)) = (dl->d_blk).cmd_d.Val;
		Serial.write(res, 5);
	}
	break;
	case ECHO_SUM:
	{
		res[0] = '>';
		*((uint16_t*)(res + 1)) = (dl->d_blk).cmd_d.chksum;
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
		if(!strcmp(dat->tok[0], "cksum")) {
			me->stat_flg |= CHKSUM;
			return true;
		} else if(!strcmp(dat->tok[0], "nosum")) {
			me->stat_flg &= ~CHKSUM;
			return true;
		}
	}
	return false;
}
//............................................................................

LEDgroup::LEDgroup(uint8_t id, uint8_t s, uint8_t e, uint16_t itr)
	: QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial),
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
		if(!strcmp(dat->tok[0], "itrvl")) {
			if(dat->Val > 0) {
				me->itrvl = dat->Val;
				me->m_timeEvt.rearm(me->itrvl);
				return true;
			}
		}
	}
	return false;
}

//............................................................................
