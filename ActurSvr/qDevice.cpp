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

//............................................................................

extern QActive* dev_tbl[];
extern CmdPump* p_cp;

extern unsigned long start_time;
extern unsigned long passed_time;
extern uint16_t max_time;

QDevice::QDevice(uint8_t id, QDcmdHandler p, QStateHandler h)
	:
	QActive(h),
	devID(id), clbkfunc(p) {
	first     = NULL;
	last      = NULL;
	List_cnt  = 0;
	dev_tbl[ id & 0x3F ] = this;
}

uint8_t QDevice::getID() {
	return devID & 0x3F;
}

uint8_t QDevice::ListCount() {
	return List_cnt;
}

void QDevice::EnqueueList(Data_List* lp) {

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
};

bool QDevice::DequeueCmd() {

	if(first != NULL) {

		bool ans = false;

		Data_List temp = *first;
		free(first);

		first = NULL;

		ans = (*clbkfunc)(this, &(temp.d_blk));

		if(temp.next != NULL) {
			first = temp.next;
		}
		else {
			last = NULL;
		}

		--List_cnt;

		return ans;
	}
	return false;
};

void QDevice::FlushQueue() {

	if(first == NULL) {
		return;
	}

	Data_List* flush_list = first;
	Data_List* nextflush_list = NULL;

	do {

		nextflush_list = flush_list->next;
		free(flush_list);
		flush_list = nextflush_list;

	}
	while(flush_list != NULL);
	List_cnt = 0;
}
//............................................................................
CmdPump::CmdPump(uint8_t id)
	: QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial),
	m_keep_alive_timer(SI_CHK_ALIVE_SIG)
{
	stat_flg = 0x00;
	stat_flg |= STAY;
	stat_flg |= ALIVE;

	lstp  = (Data_List*)malloc(sizeof(Data_List));
	rp  = lstp->d_blk.origin_str;
	c = '\0';
}

void CmdPump::On_ISR() {
	if (stat_flg & EMGCY) {
		if (Serial.available() > 0) {
			c = Serial.read();
			if (c == '^') {
				QEvent* pe = Q_NEW(QEvent, SI_RETURN_SIG);
				this->POST(pe, this);
			}
		}
	} else {
		if (Serial.available() > 0) {

			c = Serial.read();

			if(c == '\0') { return; }

			*rp++ = c;

			if (c == '\n' || ((rp - (lstp->d_blk.origin_str)) > (cmdSIZE - 2))) {
				switch (*(lstp->d_blk.origin_str)) {
				case '<':
				case '(':
				{
					*rp = '\0';
					EnqueueList(lstp);
					lstp  = (Data_List*)malloc(sizeof(Data_List));
					rp    = lstp->d_blk.origin_str;
					QEvent* pe = Q_NEW(QEvent, SI_END_LINE_SIG);
					this->POST(pe, this);
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
		} else if (ListCount()) {
			QEvent* pe = Q_NEW(QEvent, SI_END_LINE_SIG);
			this->POST(pe, this);
		}
	}
}

QState CmdPump::initial(CmdPump *me, QEvent const *e) {
	me->m_keep_alive_timer.postIn(me, 250);
	me->subscribe(SI_EMGCY_SIG);
	return Q_TRAN(&CmdPump::Exchange);
}

QState CmdPump::Exchange(CmdPump *me, QEvent const *e) {
	switch (e->sig) {
	case Q_ENTRY_SIG:
	{
		return Q_HANDLED();
	}
	case SI_END_LINE_SIG:
	{
		me->DequeueCmd();
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

bool CmdPump::CmdExecutor(CmdPump* me, Data_Block* dblk) {
	STOP();
	char *tp = dblk->origin_str + 1;
	char *endp;
	uint8_t id = strtol(tp, &endp, 10);

	if (tp == endp) { return false; }

	uint8_t op = id & 0xC0;
	id = id & 0x3F;

	Data_Block newblk = {0};

	tp = strchr(dblk->origin_str, ',');

	if (tp != NULL) {
		char *saveptr = NULL;
		tp = strtok_r(++tp, ",\n", &saveptr);
		uint8_t i;
		for(i = 0; tp != NULL; i++) {
			if(i < 2) {
				strncpy(newblk.cmd_d.tok[i], tp, 5);
			}
			tp = strtok_r(NULL, ",\n", &saveptr);
		}

		newblk.cmd_d.num = strtol(newblk.cmd_d.tok[1], &endp, 10);

		if (newblk.cmd_d.tok[1] != endp) {
			newblk.cmd_d.context += USE_NUM;
		} else if (i == 2) {
			newblk.cmd_d.context += TWO_TOK;
		} else if (i > 2) {
			return false;
		}
	} else {
		return false;
	}

	if( (id <= TOTAL_OF_DEV) && (dev_tbl[id] != NULL)) {

		bool ans = false;

		if (id == me->getID()) {

		} else {
			switch (op) {

			case ENQUEUE:
			{
				Data_List*  pList;
				if (pList = (Data_List*)malloc(sizeof(Data_List))) {
					pList->d_blk.cmd_d = newblk.cmd_d;
				} else {
					return false;
				}
				((QDevice*)dev_tbl[id])->EnqueueList(pList);
				ans = true;
				break;
			}
			case DEQUEUE:
			{
				ans = ((QDevice*)dev_tbl[id])->DequeueCmd();
				break;
			}
			case FLUSH:
			{
				((QDevice*)dev_tbl[id])->FlushQueue();
				ans = true;
			}
			default:
			{
				ans = SEND_CMD(id, &newblk);
				break;
			}
			}
		}
		return ans;
	} else {
		return false;
	}
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

bool LEDgroup::CmdExecutor(LEDgroup* me, Data_Block* p) {

	if (p->cmd_d.context == USE_NUM) {
		if(!strcmp(p->cmd_d.tok[0], "itrvl")) {
			if(p->cmd_d.num > 0) {
				me->itrvl = p->cmd_d.num;
				me->m_timeEvt.rearm(me->itrvl);
				return true;
			}
		}
	}
	return false;
}

//............................................................................
