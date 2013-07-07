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
extern unsigned long max_time;

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

bool QDevice::CmdDivider(Cmd_Data* dist, char* src) { 
  
  char* p = strchr(src, ',');  
  if (p == NULL) { return false; }
  
  char* ch         = NULL;
  char* saveptr    = NULL;
  char* endp       = NULL;
  
  uint8_t i;
  
  ch = strtok_r(++p, ",\n", &saveptr);  
  
  for(i = 0; ch != NULL ; i++) {
    if(i < 2) {
      
      strncpy(dist->Str[i], ch, 5);
      dist->Param = strtol(dist->Str[i], &endp, 10);
      if (dist->Str[i] != endp) {
        //dist->context += i + 1;
      }
      
    }    
    ch = strtok_r(NULL, ",\n", &saveptr);
  }
  
  //if(i == 1) { dist->context += 128; }
  dist->context = QD_I;
  if(i > 2) { return false; }
  
  return true;
};

bool  QDevice::CmdExecutor(QDevice* Me, Data_Block* dbp) {
  return (*clbkfunc)(Me, dbp);
};

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
    
    Data_List* temp = first;

    first = NULL;
    
    ans = (*clbkfunc)(this, &(temp->d_blk));

    if(temp->next != NULL) {
      first = temp->next;
    }
    else {
      last = NULL;
    }
    
    --List_cnt;
    free(temp);
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
/*
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
*/
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
/*
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
*/
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
      
      *rp = c;
      
       ++rp;
       
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
  
  char*       p;
  char*       endp;
  Data_List*  pList;
  uint8_t  i;  
  
  p = dblk->origin_str + 1;
  i = strtol(p, &endp, 10);
  
  if (p == endp) { return false; }
  
  uint8_t op = i & 0xC0;
  i = i & 0x3F;

  Data_Block newblk;
  
  if (me->CmdDivider(&(newblk.cmd_d), p) == false) {
    return false;
  }
  
  if( (i <= TOTAL_OF_DEV) && (dev_tbl[i] != NULL)) {
    
    bool ans = false;
    
    if (i == me->getID()) {

    } else {
      switch (op) {
        
        case ENQUEUE:
        {  
          if (pList = (Data_List*)malloc(sizeof(Data_List))) {
            pList->d_blk.cmd_d = newblk.cmd_d;
          } else {
            return false;
          }
          ((QDevice*)dev_tbl[i])->EnqueueList(pList);
          ans = true;
          break;
        }
        case DEQUEUE:
        {
          ans = ((QDevice*)dev_tbl[i])->DequeueCmd();
          break;
        }
        case FLUSH:
        {
          ((QDevice*)dev_tbl[i])->FlushQueue();
          ans = true;
        }
        default:
        {          
          ans = ((QDevice*)dev_tbl[i])->CmdExecutor((QDevice*)dev_tbl[i], &newblk);          
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

  if (p->cmd_d.context == QD_I) {
    if(p->cmd_d.Param > 0) {
      me->itrvl = p->cmd_d.Param;
      return true;
    }
  }  
  return false;
}

//............................................................................
