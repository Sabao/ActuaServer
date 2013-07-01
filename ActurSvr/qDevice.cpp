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
extern CmdPump* p_si;

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

bool QDevice::CmdDivider(const char* cmd) {
  
  CmdInfo cmdinfo = {0};
  
  char* ch;
  char* p = strchr(cmd, ',');
    
  if (p == NULL) {
    return false;
  } else {
    ch = strtok(++p,",");
  }
  
  strcpy(cmdinfo.buffcpy, p);
  uint8_t i;
  
  for(i = 0; ch != NULL ; i++) {
    if(i < 2) {
      strncpy(&cmdinfo.cmdLetter[i], ch, 1);
      cmdinfo.cmdValue[i] = strtol(ch, NULL, 10);
    }
    ch = strtok(NULL,",");
  }
    
  if(i > 2) { return false; }    

  strcpy(cmdinfo.buffcpy, cmd);

  (*clbkfunc)(this, &cmdinfo); 

  return true;
};

char* QDevice::EnqueueCmd(const char *cmd) {

  void* new_list;
  if(new_list = malloc(sizeof(CmdList))) {

    if(last == NULL) {
      last = (CmdList*)new_list;
      first = last;
    }
    else {
      last->next = (CmdList*)new_list;
      last = last->next;
    }

    last->next = NULL;
    
    if (cmd != NULL) {
      strcpy(last->cmdString, cmd);
    }
    
    ++List_cnt;
    return ((CmdList*)new_list)->cmdString;
    
  } else {
    FlushQueue();
    return NULL;
  } 
};

void QDevice::EnqueueList(CmdList* lp) {

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

    CmdList* temp = first;

    first = NULL;
    CmdDivider(temp->cmdString);

    if(temp->next != NULL) {
      first = temp->next;
    }
    else {
      last = NULL;
    }
    
    --List_cnt;
    free(temp);
    return true;
  }
  return false;        
};

void QDevice::FlushQueue() {

  if(first == NULL) {
    return; 
  }

  CmdList* flush_list = first;
  CmdList* nextflush_list = NULL;

  do {

    nextflush_list = flush_list->next;
    free(flush_list);
    flush_list = nextflush_list;

  } 
  while(flush_list != NULL);
  List_cnt = 0;
}

void QDevice::InternalCmd(uint8_t id, int16_t val1, int16_t val2, char c = '<') {
  
  char temp[6] = {'\0'};
  char str[cmdSIZE] = {'@',};
  
  strcat(str, itoa(PROC_id, temp ,10));
  strcat(str, "|");
  memset(temp, '\0', 6);
  strcat(str, itoa(id, temp, 10));
  strcat(str, ",");
  memset(temp, '\0', 6);
  strcat(str, itoa(val1, temp, 10));
  strcat(str, ",");
  memset(temp, '\0', 6);
  strcat(str, itoa(val2, temp, 10));
  strcat(str, "\n");
  
  send_cmd(str, id, c);
}

void QDevice::InternalCmd(uint8_t id, int16_t val1, char val2, char c = '<') {
  
  char temp[6] = {'\0'};
  char str[cmdSIZE] = {'@',};
  
  strcat(str, itoa(PROC_id, temp ,10));
  strcat(str, "|");
  memset(temp, '\0', 6);
  strcat(str, itoa(id, temp, 10));
  strcat(str, ",");
  memset(temp, '\0', 6);
  strcat(str, itoa(val1, temp, 10));
  strcat(str, ",");
  memset(temp, '\0', 6);
  temp[0] = val2;
  strcat(str, temp);
  strcat(str, "\n");
  
  send_cmd(str, id, c);
}

void QDevice::InternalCmd(uint8_t id, char val1, int16_t val2, char c = '<') {
  
  char temp[6] = {'\0'};
  char str[cmdSIZE] = {'@',};

  strcat(str, itoa(PROC_id, temp ,10));
  strcat(str, "|");
  memset(temp, '\0', 6);
  strcat(str, itoa(id, temp, 10));
  strcat(str, ",");
  memset(temp, '\0', 6);
  temp[0] = val1;
  strcat(str, temp);
  strcat(str, ",");
  strcat(str, itoa(val2, temp, 10));
  strcat(str, "\n");
  
  send_cmd(str, id, c);  
}

void QDevice::InternalCmd(uint8_t id, char val1, char val2, char c = '<') {
  
  char temp[4] = {'\0'};
  char str[cmdSIZE] = {'@',};

  strcat(str, itoa(PROC_id, temp ,10));
  strcat(str, "|");
  memset(temp, '\0', 6);
  strcat(str, itoa(id, temp, 10));
  strcat(str, ",");
  memset(temp, '\0', 4);
  temp[0] = val1;
  strcat(str, temp);
  strcat(str, ",");
  temp[0] = val2;
  strcat(str, temp);
  strcat(str, "\n");

  send_cmd(str, id, c);  
}

void QDevice::send_cmd(const char*s, uint8_t oid, char c) {
  if (c == '<') {
    if( oid <= TOTAL_OF_DEV && dev_tbl[oid] != NULL ) {
      ((QDevice*)dev_tbl[oid])->EnqueueCmd(s);
    }
  }
}

//............................................................................
CmdPump::CmdPump(uint8_t id)
: QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial),
m_keep_alive_timer(SI_CHK_ALIVE_SIG)
{
  stat_flg = 0x00;
  stat_flg |= STAY;
  stat_flg |= ALIVE;

  lstp  = (CmdList*)malloc(sizeof(CmdList));
  rp  = lstp->cmdString;
  c = '\0';
}

void CmdPump::CmdPump_prefix(char* s, uint8_t ch, int8_t id) {
  
    char temp[4] = {'\0'};
    uint8_t proc_id = PROC_id;
    
    *s = '(';
    strcat(s, itoa(ch, temp, 10));
    strcat(s, "*");
    memset(temp, '\0', 4);
    strcat(s, itoa(proc_id, temp, 10));
    strcat(s, "|");
    memset(temp, '\0', 4);
    strcat(s, itoa(id, temp, 10));
    strcat(s, ",");
    
}

bool CmdPump::send_to_serial(int8_t ch, int8_t id, int16_t val1, int16_t val2){
  
  if(!(stat_flg & EMGCY)) {
    
    char temp[6] = {'\0'};
    char str[cmdSIZE] = {'\0'};
    
    CmdPump_prefix(str, ch, id);
    
    strcat(str, itoa(val1, temp, 10));
    strcat(str, ",");
    memset(temp, '\0', 6);
    strcat(str, itoa(val2, temp, 10));
    strcat(str, "\n");   

    EnqueueCmd(str);
    
    return true;
    
  } else {
    return false;
  }  
}

bool CmdPump::send_to_serial(int8_t ch, int8_t id, int16_t val1, char val2){
  
  if(!(stat_flg & EMGCY)) {
    
    char temp[6] = {'\0'};
    char str[cmdSIZE] = {'\0'};
    
    CmdPump_prefix(str, ch, id);
    
    strcat(str, itoa(val1, temp, 10));
    strcat(str, ",");
    memset(temp, '\0', 6);
    temp[0] = val2;
    strcat(str, temp);
    strcat(str, "\n");
    
    EnqueueCmd(str);
    
    return true;
    
  } else {
    return false;
  }  
}

bool CmdPump::send_to_serial(int8_t ch, int8_t id, char val1, int16_t val2){
  
  if(!(stat_flg & EMGCY)) {
    
    char temp[6] = {'\0'};
    char str[cmdSIZE] = {'\0'};
    
    CmdPump_prefix(str, ch, id);
    
    temp[0] = val1;
    strcat(str, temp);
    strcat(str, ",");
    strcat(str, itoa(val2, temp, 10));
    strcat(str, "\n");
    
    EnqueueCmd(str);
    
    return true;
    
  } else {
    return false;
  }  
}

bool CmdPump::send_to_serial(int8_t ch, int8_t id, char val1, char val2){
  
  if(!(stat_flg & EMGCY)) {
    
    char temp[2] = {'\0'};
    char str[cmdSIZE] = {'\0'};
    
    CmdPump_prefix(str, ch, id);
    
    temp[0] = val1;
    strcat(str, temp);
    strcat(str, ",");
    temp[0] = val2;
    strcat(str, temp);
    strcat(str, "\n");
    
    EnqueueCmd(str);
    
    return true;
    
  } else {
    return false;
  }  
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
      
      if (c == '\n' || (rp - lstp->cmdString > cmdSIZE - 2)) {
        switch (*(lstp->cmdString)) {
          case '<':
          case '(':
          {
            *rp = '\0';
            EnqueueList(lstp);
            lstp  = (CmdList*)malloc(sizeof(CmdList));
            rp    = lstp->cmdString;
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
            rp    = lstp->cmdString;
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
      me->InternalCmd(1, 25, 0);
      return Q_HANDLED();
    }
    case SI_RETURN_SIG:
    {
      Serial.print(me->c);
      
      me->stat_flg  = 0x00;
      me->stat_flg |= STAY;
      me->stat_flg |= ALIVE;
      me->rp        = me->lstp->cmdString;
      me->c = '\0';
      QEvent* pe = Q_NEW(QEvent, SI_CHK_ALIVE_SIG);
      me->POST(pe, me);    
      return Q_HANDLED();
    }
  }
  return Q_SUPER(&QHsm::top);
}

void CmdPump::CmdExecutor(CmdPump* me, CmdInfo* data) {
  
  char*    p;
  char*    endp;
  uint8_t  i;
  
  p = data->buffcpy + 1;
  i = strtol(p, &endp, 10);
  
  if (p == endp) { return; }
  
  uint8_t op = i & 0xC0;
  i = i & 0x3F;
  
  if( (i <= TOTAL_OF_DEV) && (dev_tbl[i] != NULL)) {
    Serial.print(data->buffcpy);
    
    if (i == me->getID()) {

    } else {
      switch (op) {
        
        case ENQUEUE:
        {
          ((QDevice*)dev_tbl[i])->EnqueueCmd(data->buffcpy);
          break;
        }
        case DEQUEUE:
        {
          ((QDevice*)dev_tbl[i])->DequeueCmd();
          break;
        }
        case FLUSH:
        {
          ((QDevice*)dev_tbl[i])->FlushQueue();
        }
        default:
        {
          ((QDevice*)dev_tbl[i])->CmdDivider(data->buffcpy);
          break;
        }
      }
    }
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

void LEDgroup::CmdExecutor(LEDgroup* me, CmdInfo* p) {

  //Write your class-depended code under here.
  if(p->cmdValue[0] != 0) {    
    me->itrvl = p->cmdValue[0];
  }  
}

//............................................................................
