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

void Rotate8BitsShift(uint8_t* p, uint8_t cnt, bool r_direction) {
  uint8_t i;
  for(i=0; i < cnt; ++i) {

    if(r_direction) {
      if(*p & 128) {
        *p = *p << 1;
        *p = *p | 1;
      } 
      else {
        *p = *p << 1;
      }
    }
    else {
      if(*p & 1) {
        *p = *p >> 1;
        *p = *p | 128;
      } 
      else {
        *p = *p >> 1;
      }
    }
  }
}

//............................................................................

extern QActive* dev_tbl[];
extern SerialInterface* p_si;

QDevice::QDevice(uint8_t id, QDcmdHandler p, QStateHandler h)
: 
QActive(h),
devID(id), clbkfunc(p) {
  first = NULL; 
  last  = NULL;
#ifdef TOTAL_OF_DEV
  dev_tbl[ id & 0x3F ] = this;
#endif
}

uint8_t QDevice::getID() {
  return devID & 0x3F;
}

bool QDevice::rsv() {
  if (first != NULL) {
    return true;
  } else {
    return false;
  }
}

int8_t QDevice::check_id(const char* cmd) {
  
  char *p = strchr(cmd, '|');
  
  if (p == NULL) {
    return -1;
  } else {
    ++p;
  }
  
  uint8_t id = atoi(p);
  if(((id & 0x3F) != (devID & 0x3F)) && (!(devID & NO_RETURN))) { 
    return -1;
  }  

  if(id & FLUSH_QUEUE) {
    FlushQueue();
  }
  
  return id;  
}

bool QDevice::CmdDivider(const char* cmd) {
  
  int8_t id = check_id(cmd);
  if(id < 0) { return false; }

  if((first != NULL) && (!(id & INTERRUPT))) {
    EnqueueCmd(cmd);
    return false;
  }
  
  CmdInfo cmdinfo = {0};
  
  if (!(devID & NO_ARRAY)) {
    
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
        cmdinfo.cmdValue[i] = atoi(ch);
      }
      ch = strtok(NULL,",");
    }
    
    if(i > 2) { return false; }    
  }
  
  if (this != p_si && (*cmd == '@')) {
    p_si->EnqueueCmd(cmd);
  }
  
  strcpy(cmdinfo.buffcpy, cmd);

  (*clbkfunc)(this, &cmdinfo); 

  return true;
};

bool QDevice::EnqueueCmd(const char *cmd) {

  if(check_id(cmd) >= 0) {
    void* new_que;
    if(new_que = malloc(sizeof(CmdQueue))) {

      if(last == NULL) {
        last = (CmdQueue*)new_que;
        first = last;
      }
      else {
        last->next = (CmdQueue*)new_que;
        last = last->next;
      }

      last->next = NULL;    
      strcpy(last->cmdString, cmd);
      return true;
    } else {
      FlushQueue();
      return false;
    }
  } else {
    return false;
  }
};

bool QDevice::DequeueCmd() {

  if(first != NULL) {

    CmdQueue* temp = first;

    first = NULL;
    CmdDivider(temp->cmdString);

    if(temp->next != NULL) {
      first = temp->next;
    }
    else {
      last = NULL;
    }

    free(temp);
    return true;
  }
  return false;        
};

void QDevice::FlushQueue() {
  if(first == NULL) { 
    return; 
  }

  CmdQueue* flush_que = first;
  CmdQueue* nextflush_que = NULL;

  do {

    nextflush_que = flush_que->next;
    free(flush_que);
    flush_que = nextflush_que;

  } 
  while(flush_que != NULL);

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
      CommandEvt* pe = Q_NEW(CommandEvt, BROAD_COMM_SIG);
      strcpy(pe->CommStr, s);
      dev_tbl[oid]->POST(pe, this);
    }
  } else {
    CommandEvt* pe = Q_NEW(CommandEvt, BROAD_COMM_SIG);
    strcpy(pe->CommStr, s);
    QF::publish(pe);
  }
}

//............................................................................
SerialInterface::SerialInterface(uint8_t id)
: QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial),
m_keep_alive_timer(SI_CHK_ALIVE_SIG)
{
  stat_flg = 0x00;
  stat_flg |= STAY;
  stat_flg |= ALIVE;
  memset(read_buf, 0, sizeof(read_buf));
  memset(check_buf, 0, sizeof(check_buf));
  rp = read_buf;
  c = '\n';
}

void SerialInterface::SI_prefix(char* s, uint8_t ch, int8_t id) {
  
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

bool SerialInterface::send_to_serial(int8_t ch, int8_t id, int16_t val1, int16_t val2){
  
  if(!(stat_flg & EMGCY)) {
    
    char temp[6] = {'\0'};
    char str[cmdSIZE] = {'\0'};
    
    SI_prefix(str, ch, id);
    
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

bool SerialInterface::send_to_serial(int8_t ch, int8_t id, int16_t val1, char val2){
  
  if(!(stat_flg & EMGCY)) {
    
    char temp[6] = {'\0'};
    char str[cmdSIZE] = {'\0'};
    
    SI_prefix(str, ch, id);
    
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

bool SerialInterface::send_to_serial(int8_t ch, int8_t id, char val1, int16_t val2){
  
  if(!(stat_flg & EMGCY)) {
    
    char temp[6] = {'\0'};
    char str[cmdSIZE] = {'\0'};
    
    SI_prefix(str, ch, id);
    
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

bool SerialInterface::send_to_serial(int8_t ch, int8_t id, char val1, char val2){
  
  if(!(stat_flg & EMGCY)) {
    
    char temp[2] = {'\0'};
    char str[cmdSIZE] = {'\0'};
    
    SI_prefix(str, ch, id);
    
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

void SerialInterface::On_ISR() {
  if (stat_flg & EMGCY) {
    if (Serial.available() > 0) {
      c = Serial.read();
      if (c == '^') {
        MsgEvt* pe = Q_NEW(MsgEvt, SI_RETURN_SIG);
        this->POST(pe, this);
      }
    }
  } else {
    if (Serial.available() > 0) {
      if (stat_flg & SHUT) { return; }
      
      c = Serial.read();
      if(c == '\0') { return; }
      
      Serial.print(c);
      *rp++ = c;
      
      if (c == '\n' || (rp - read_buf) > cmdSIZE - 2) {
        switch (*read_buf) {
          case '<':
          case '[':
          case ')':
          case '~':
          {
            stat_flg |= SHUT;
            MsgEvt* pe = Q_NEW(MsgEvt, SI_END_LINE_SIG);
            this->POST(pe, this);
            rp = read_buf;
            break;
          }
          default:
          {
            memset(read_buf, 0, sizeof(read_buf));
            rp = read_buf;
            break;
          }
        }
      }
    } else if (c == '\n') {
      if (rsv()) {
        stat_flg |= SHUT;
        MsgEvt* pe = Q_NEW(MsgEvt, SI_DEQUE_SIG);
        this->POST(pe, this); 
      }
    }
  }
}

QState SerialInterface::initial(SerialInterface *me, QEvent const *) {
  me->m_keep_alive_timer.postIn(me, 250);
  me->subscribe(SI_EMGCY_SIG);
  return Q_TRAN(&SerialInterface::Exchange);
}

QState SerialInterface::Exchange(SerialInterface *me, QEvent const *e) {
  switch (e->sig) {
    case Q_ENTRY_SIG:
    {
      return Q_HANDLED();
    }
    case SI_END_LINE_SIG:
    {
      switch (*(me->read_buf)) {
        case '<':
        {
          if (atoi(me->read_buf + 1) == PROC_id) {
            char *p = strchr(me->read_buf, '|');
            if (p != NULL) {
              p++;
              uint8_t ai = atoi(p);
              if( (ai <= TOTAL_OF_DEV) && (dev_tbl[ai] != NULL) ) {
                CommandEvt* pe = Q_NEW(CommandEvt, BROAD_COMM_SIG);
                *(me->read_buf) = '>';
                Serial.print(me->read_buf);
                strcpy(pe->CommStr, me->read_buf);
                dev_tbl[ai]->POST(pe, me);
              }
            }
          }
          break;
        }
        case '[':
        {
          if (atoi(me->read_buf + 1) == PROC_id) {
            CommandEvt* pe = Q_NEW(CommandEvt, BROAD_COMM_SIG);
            *(me->read_buf) = ']';
            Serial.print(me->read_buf);
            strcpy(pe->CommStr, me->read_buf);
            QF::publish(pe);
          }
          break;
        }
        case ')':
        {
          char *p = strchr(me->read_buf, '*') + 1;
          if (atoi(p) == PROC_id) {
            if (strcmp((me->read_buf + 1) , (me->check_buf + 1)) != 0) {
              me->stat_flg |= EMGCY;
              me->stat_flg &= ~STAY;
              me->stat_flg &= ~ALIVE;
            } else {
              me->stat_flg &= ~CHKECHO;
            }
          }
          break;
        }
        case '~':
        {
          me->stat_flg &= ~STAY;
          me->stat_flg |= ALIVE;
          break;
        }
      }
      memset(me->read_buf, 0, sizeof(me->read_buf));
      me->stat_flg &= ~SHUT;
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
          MsgEvt* pe = Q_NEW(MsgEvt, SI_EMGCY_SIG);
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
      memset(me->read_buf, 0, sizeof(me->read_buf));
      memset(me->check_buf, 0, sizeof(me->check_buf));
      me->rp = me->read_buf;
      me->c = '\0';
      MsgEvt* pe = Q_NEW(MsgEvt, SI_CHK_ALIVE_SIG);
      me->POST(pe, me);    
      return Q_HANDLED();
    }
    case SI_DEQUE_SIG:
    {
      me->DequeueCmd();
      me->stat_flg &= ~SHUT;
      return Q_HANDLED();
    }
  }
  return Q_SUPER(&QHsm::top);
}

void SerialInterface::CmdExecutor(SerialInterface* me, CmdInfo* p) {
     if(*(p->buffcpy) == '(') {
       if(me->stat_flg & CHKECHO) {
         me->EnqueueCmd(p->buffcpy);
         return;
       } else {
         me->stat_flg |= CHKECHO;         
         strcpy(me->check_buf, p->buffcpy);
       }
     }
     Serial.print(p->buffcpy);
}
//............................................................................

LEDgroup::LEDgroup(uint8_t id, uint8_t s, uint8_t e, uint16_t itr)
: QDevice(id, (QDcmdHandler)&CmdExecutor, (QStateHandler)initial),
m_timeEvt(TIMEOUT_SIG), s_pin(s), e_pin(e) {       
  cur_pin = s;    
  itrvl = itr;

  uint8_t i;
  for(i = s_pin; i <= e_pin; ++i) {
    pinMode(i, OUTPUT);
  }    
}

QState LEDgroup::initial(LEDgroup *me, QEvent const *) {
  me->subscribe(BROAD_COMM_SIG);
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
  case BROAD_COMM_SIG: 
    {
      me->CmdDivider(((CommandEvt *)e)->CommStr);
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
  case BROAD_COMM_SIG: 
    {
      me->CmdDivider(((CommandEvt *)e)->CommStr);
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

QDevStepper::QDevStepper(
  uint8_t  id,                  
  uint8_t  x,
  uint8_t  y,
  uint8_t  _x,
  uint8_t  _y,
  uint8_t  stepMode,
  uint16_t itr,
  uint16_t cnt,
  bool     dir
  ) :
QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial),
m_timeEvt(TIMEOUT_SIG) {
  
  excPattern = stepMode;

  itrvl = itr;
  stepcnt = cnt;
  r_direction = dir;
  cntdown = true;

  MaskTable[0][0] = x;
  MaskTable[1][0] = y;
  MaskTable[2][0] = _x;
  MaskTable[3][0] = _y;

  uint8_t i, shiftcnt, mask = 0x80;

  switch(stepMode) {
  case WAVE_DRIVE:
  case FULL_STEP: 
    {
      shiftcnt = 1;
      break;
    }
  case HALF_STEP: 
    {
      shiftcnt = 2;
      break;
    }
  default : 
    {
      shiftcnt = 1;
      break;
    }
  }

  for(i=0; i < 4; ++i) {
    pinMode(MaskTable[i][0], OUTPUT);
    Rotate8BitsShift(&mask, shiftcnt, 0);
    MaskTable[i][1] = mask;
  }
}

QState QDevStepper::initial (QDevStepper *me, QEvent const *e) {
  me->subscribe(BROAD_COMM_SIG);
  return Q_TRAN(&QDevStepper::onIdle);
}

QState QDevStepper::Stepping(QDevStepper *me, QEvent const *e) {
  switch (e->sig) {
  case Q_ENTRY_SIG: 
    {
      me->m_timeEvt.postIn(me, 1);
      return Q_HANDLED();
    }
  case BROAD_COMM_SIG: 
    {             
      me->EnqueueCmd(((CommandEvt *)e)->CommStr);            
      return Q_HANDLED();
    }
  case TIMEOUT_SIG: 
    {          
      if(me->stepcnt) {
        uint8_t i;
        for(i = 0; i < 4; ++i) {
          digitalWrite(me->MaskTable[i][0], (me->excPattern) & (me->MaskTable[i][1]));
        }                 
        Rotate8BitsShift(&(me->excPattern), 1, me->r_direction);
        me->m_timeEvt.postIn(me, me->itrvl);
        if(me->cntdown) { 
          me->stepcnt--; 
        }
        return Q_HANDLED();
      }
      else if(me->DequeueCmd()) {
        me->m_timeEvt.postIn(me, 1);
        return Q_HANDLED();
      }
      else {
        return Q_TRAN(&QDevStepper::onIdle);
      }               
    }
  }
  return Q_SUPER(&QHsm::top);
}

QState QDevStepper::onIdle(QDevStepper *me, QEvent const *e) {
  switch (e->sig) {
  case Q_ENTRY_SIG: 
    {
      return Q_HANDLED();
    }
  case BROAD_COMM_SIG: 
    {               
      me->CmdDivider(((CommandEvt *)e)->CommStr);               
      return Q_TRAN(&QDevStepper::Stepping);
    }
  }
  return Q_SUPER(&QHsm::top);
}

void QDevStepper::CmdExecutor(QDevStepper* me, CmdInfo* p) {

  switch (p->cmdLetter[0]) {
  case 'C':
  case 'c': 
    {
      me->cntdown = !(me->cntdown);
      break;
    }
  case 'D':
  case 'd': 
    {
      me->r_direction = !(me->r_direction);
      break;
    }
  case 'I':
  case 'i': 
    {
      me->itrvl = p->cmdValue[1];
      break;
    }
  case 'M':
  case 'm': 
    {
      uint8_t i, shiftcnt, mask = 0x80;

      for(i = 0; i < 4; ++i) { 
        digitalWrite(me->MaskTable[i][0], LOW); 
      }
      switch(p->cmdLetter[1]) {
      case 'W': 
        {
          me->excPattern = WAVE_DRIVE;
          shiftcnt = 1;
          break;
        }
      case 'F': 
        {
          me->excPattern = FULL_STEP;
          shiftcnt = 1;
          break;
        }
      case 'H': 
        {
          me->excPattern = HALF_STEP;
          shiftcnt = 2;
          break;
        }
      default : 
        {
          me->excPattern = WAVE_DRIVE;
          shiftcnt = 1;
          break;
        }
      }  
      for(i=0; i < 4; ++i) {
        Rotate8BitsShift(&mask, shiftcnt, 0);
        me->MaskTable[i][1] = mask;
      }
      break;
    }
  case 'S':
  case 's': 
    {
      me->stepcnt = p->cmdValue[1];
      break;
    }
  default : 
    { 
      break; 
    }
  }
}

//............................................................................

ServoTact::ServoTact(uint8_t id)
    : QDevice(id, (QDcmdHandler)NULL, (QStateHandler)initial),
      m_HIGHtimingEvt(HIGH_SIG)
{  
  uint8_t i;
  for(i = 0; i < 8; ++i) {
    svo[i] = NULL;
  } 
  curr_svo = 0;
}

bool ServoTact::Attach(QDevServo *p) {
  uint8_t i;
  
  for(i = 0; i < 8; ++i) {
    if(svo[i] == NULL) {
      svo[i] = p;
      return true;
    }
  }
  return false;
}

void ServoTact::Detach(QDevServo *p) {
  uint8_t i;
  
  for(i = 0; i < 8; ++i) {
    if(svo[i] == p) {
      svo[i] = NULL;
      break;
    }
  }
}

QState ServoTact::initial(ServoTact *me, QEvent const *) {    
    return Q_TRAN(&ServoTact::conducting);
}

QState ServoTact::conducting(ServoTact *me, QEvent const *e) {
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            me->m_HIGHtimingEvt.postEvery(me, 13);
            return Q_HANDLED();
        }
        case HIGH_SIG: {
            if(me->curr_svo > 7) { me->curr_svo = 0; }            
            QDevServo* p = me->svo[me->curr_svo];
            
            if(p != NULL) {
               p->SetHigh();
            }
            ++me->curr_svo;            
            
            return Q_HANDLED();
        }
    }
    return Q_SUPER(&QHsm::top);
}

//............................................................................

extern ServoTact* p_tact;

QDevServo::QDevServo(
  uint8_t id,
  uint8_t   ctrl,
  uint16_t  bWidth,
  uint16_t  rWidth,
  uint8_t   spd
)
: QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial),
m_LOWtimingEvt(LOW_SIG), ctrlpin(ctrl) {
  
  pinMode(ctrlpin, OUTPUT);

  baseWidth  = bWidth;
  rangeWidth = rWidth;
  Speed      = spd;
  go_Idle    = true;
  sleepcnt    = 0;
  
  SetDegree(0);
}

void QDevServo::SetDegree(uint8_t deg) {

  if((deg >= 0) && (deg <= 180)) {

    pre_deg = curr_deg;
    curr_deg = deg;

    appendedWidth = (((rangeWidth / 180) * deg) + 0.5);
    tickcnt = (baseWidth + appendedWidth) / 200;
    uint8_t rest_time = (baseWidth + appendedWidth) % 200;

    if(rest_time < 100) {
      waitcnt = 3;
      if(rest_time < 15) {
        waitcnt += 15;
      }
    } 
    else {
      waitcnt = 15 + (200 - rest_time);
      ++tickcnt;
    }
    ch_deg = true;
  } 
}

void QDevServo::SetHigh() {          
  delayMicroseconds(waitcnt);
  startTime = micros();
  digitalWrite(ctrlpin, HIGH);

  if(ch_deg) {
    finishTime = startTime + (((Speed / 60) + 0.5) * abs(pre_deg - curr_deg)) * 1000;
    ch_deg = false;                  
  }

  m_LOWtimingEvt.postIn(this, tickcnt);

}

QState QDevServo::initial (QDevServo *me, QEvent const *e) {
  me->subscribe(BROAD_COMM_SIG);
  return Q_TRAN(&QDevServo::onIdle);
}

QState QDevServo::pulsing(QDevServo *me, QEvent const *e) {
  switch (e->sig) {
  case Q_ENTRY_SIG: 
    {
      p_tact->Attach(me);
      return Q_HANDLED();
    }
  case BROAD_COMM_SIG: 
    {             
      me->EnqueueCmd(((CommandEvt *)e)->CommStr);            
      return Q_HANDLED();
    }
  case LOW_SIG: 
    {    
      int32_t wait_time = (me->baseWidth + me->appendedWidth)-(micros() - me->startTime);

      if((wait_time > 2) && (wait_time < 200)) {
        delayMicroseconds(wait_time);
      }
      digitalWrite(me->ctrlpin, LOW);
      
      uint8_t maxcnt = (me->Speed * 3) / 20.8;
      
      if(micros() > me->finishTime || me->sleepcnt > maxcnt) {
        me->DequeueCmd();
        me->sleepcnt = 0;
      } else {        
        ++me->sleepcnt;
      }
      
      if(me->go_Idle) {
        p_tact->Detach(me);
        me->FlushQueue();
        return Q_TRAN(&QDevServo::onIdle);
      }         
      return Q_HANDLED();
    }
  }
  return Q_SUPER(&QHsm::top);
}

QState QDevServo::onIdle(QDevServo *me, QEvent const *e) {
  switch (e->sig) {
  case Q_ENTRY_SIG: 
    {     
      return Q_HANDLED();
    }
  case BROAD_COMM_SIG: 
    {               
      me->CmdDivider(((CommandEvt *)e)->CommStr);
      if(!me->go_Idle) { 
        return Q_TRAN(&QDevServo::pulsing); 
      }
      return Q_HANDLED();               
    }
  }
  return Q_SUPER(&QHsm::top);
}

void QDevServo::CmdExecutor(QDevServo* me, CmdInfo* p) {

  switch (p->cmdLetter[0]) {
  case 'D':
  case 'd': 
    {
      uint8_t deg = p->cmdValue[1];
      me->SetDegree(deg);
      break;     
    }

  case 'B':
  case 'b': 
    {    
      uint16_t bw = p->cmdValue[1];
      if((bw + me->rangeWidth) <= 2499) {
        me->baseWidth = bw;
        me->SetDegree(90);
      }
      break;     
    }    
  case 'R':
  case 'r': 
    {
      uint16_t rw = p->cmdValue[1];
      if((me->baseWidth + rw) <= 2499) {
        me->rangeWidth = rw;
        me->SetDegree(90);
      }
      break;      
    }    
  case 'I':
  case 'i': 
    {
      me->go_Idle = true;
      break;
    }    
  case 'P':
  case 'p': 
    {
      me->go_Idle = false;
      break;
    }
  case 'S':
  case 's':
    {
      me->Speed = p->cmdValue[1];
    }     
  default : 
    { 
      break; 
    }
  }
}

//............................................................................

ArduServo::ArduServo(
  uint8_t   id,
  uint8_t   ctrl,
  uint16_t  bWidth,
  uint16_t  rWidth,
  uint16_t  spd
) : QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial),
  m_DONEtimingEvt(DONE_SIG), ctrlpin(ctrl) {
  baseWidth  = bWidth;
  rangeWidth = rWidth;
  Speed      = spd;
  
  moving     = false;
  go_Idle    = true;
}

QState ArduServo::initial (ArduServo *me, QEvent const *e) {
  me->subscribe(BROAD_COMM_SIG);    
  return Q_TRAN(&ArduServo::onIdle);
}

QState ArduServo::pulsing(ArduServo *me, QEvent const *e) {
  switch (e->sig) {
  case Q_ENTRY_SIG:
    {
      me->m_DONEtimingEvt.postIn(me, 1);
      return Q_HANDLED();
    }
  case BROAD_COMM_SIG: 
    {
      me->EnqueueCmd(((CommandEvt *)e)->CommStr);
      return Q_HANDLED();
    }
  case DONE_SIG:
    {
      me->moving = false;
      me->DequeueCmd();      
      
      if(me->go_Idle) { 
        return Q_TRAN(&ArduServo::onIdle); 
      }
      
      if(!me->moving) { me->m_DONEtimingEvt.postIn(me, 50); }
      return Q_HANDLED();
    }
  }
  return Q_SUPER(&QHsm::top);
}

QState ArduServo::onIdle(ArduServo *me, QEvent const *e) {
  switch (e->sig) {
  case Q_ENTRY_SIG: 
    {     
      return Q_HANDLED();
    }
  case BROAD_COMM_SIG: 
    {               
      me->CmdDivider(((CommandEvt *)e)->CommStr);
      if(!me->go_Idle) { 
        return Q_TRAN(&ArduServo::pulsing); 
      }
      return Q_HANDLED();               
    }
  }
  return Q_SUPER(&QHsm::top);
}

void ArduServo::CmdExecutor(ArduServo* me, CmdInfo* p) {

  switch (p->cmdLetter[0]) {
  case 'D':
  case 'd': 
    {
      uint8_t deg = p->cmdValue[1];
      
      if(deg >= 0 && deg <= 180) {
        uint8_t predeg = me->ardusvo.read();
        uint16_t sleepcnt = (((me->Speed / 60) + 0.5) * abs(predeg - deg)) * 5 + 1;
        me->ardusvo.write(deg);
        me->m_DONEtimingEvt.postIn(me, sleepcnt);
        
        me->moving = true;
      }
      break;     
    }

  case 'B':
  case 'b':
    {    
      uint16_t bw = p->cmdValue[1];
      me->baseWidth = bw;
      me->go_Idle = true;
      break;     
    }    
  case 'R':
  case 'r': 
    {
      uint16_t rw = p->cmdValue[1];
      me->rangeWidth = rw;
      me->go_Idle = true;
      break;      
    }    
  case 'I':
  case 'i': 
    {
      me->ardusvo.detach();
      me->FlushQueue();
      me->go_Idle = true;
      break;
    }    
  case 'P':
  case 'p': 
    {
      me->ardusvo.attach(me->ctrlpin, me->baseWidth, me->rangeWidth);
      me->go_Idle = false;
      break;
    }
  default : 
    { 
      break; 
    }
  }
}

//............................................................................

QDevMortor::QDevMortor(
  const uint8_t  id,
  const uint8_t  in1,
  const uint8_t  in2
) : QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial),
  m_FREQtimingEvt(FREQ_SIG), m_LOWtimingEvt(LOW_SIG),
  In1_pin(in1),
  In2_pin(in2) {
    
  pinMode(In1_pin, OUTPUT);
  pinMode(In2_pin, OUTPUT);
  
  pwm_pin   = In1_pin;
  
  go_Idle   = true;  
  curr_duty = 0;

  SetPwm(0);  
}

void QDevMortor::SetPwm(int8_t d) {
  
  digitalWrite(In1_pin, LOW);
  digitalWrite(In2_pin, LOW);
  
  uint8_t pre_duty = curr_duty;
  curr_duty  = d;  
  
  if((d > 0) && (d < 100)) {

    uint32_t wait_time = 20 * d;
    
    wait_tick = wait_time / 200;
    remcnt    = wait_time % 200;
    
    if(pre_duty < 1 || pre_duty > 99) { m_FREQtimingEvt.postIn(this, 1); }
    
  } else {
    
    wait_tick  = 0;
    remcnt     = 0;

    if(d < 1) {
      digitalWrite(pwm_pin, LOW);
    } else if(d > 99) {
      digitalWrite(pwm_pin, HIGH);
    }
  }
}

QState QDevMortor::initial (QDevMortor *me, QEvent const *e) {
  me->subscribe(BROAD_COMM_SIG);
  return Q_TRAN(&QDevMortor::onIdle);
}

QState QDevMortor::pulsing(QDevMortor *me, QEvent const *e) {
  switch (e->sig) {
  case Q_ENTRY_SIG:
    {
      return Q_HANDLED();
    }
  case BROAD_COMM_SIG: 
    {
      me->CmdDivider(((CommandEvt *)e)->CommStr);
      if(me->go_Idle) {
        me->SetPwm(0);
        return Q_TRAN(&QDevMortor::onIdle); 
      }
      return Q_HANDLED();
    }
  case FREQ_SIG:
    {
      if(me->curr_duty < 1 || me->curr_duty > 99) { return Q_HANDLED(); }
      
      digitalWrite(me->pwm_pin, LOW);
      
      if(me->remcnt < 100) {
        digitalWrite(me->pwm_pin, HIGH);
        
        if(me->wait_tick > 0) {
          me->m_LOWtimingEvt.postIn(me, me->wait_tick);
        } else {
          delayMicroseconds(me->remcnt);
          digitalWrite(me->pwm_pin, LOW);
        }
      } else {        
        if((200 - me->remcnt) > 2) { delayMicroseconds(200 - me->remcnt); }
        digitalWrite(me->pwm_pin, HIGH);
        
        if((me->wait_tick + 1) < 10) {
          me->m_LOWtimingEvt.postIn(me, (me->wait_tick + 1));
        }
      }      
      me->m_FREQtimingEvt.postIn(me, 10);
      return Q_HANDLED();
    }
  case LOW_SIG:
    {
      if(me->curr_duty < 1 || me->curr_duty > 99) { return Q_HANDLED(); }
      
      if(me->remcnt > 2 && me->remcnt < 100) { delayMicroseconds(me->remcnt); }
      digitalWrite(me->pwm_pin, LOW);
      
      return Q_HANDLED();
    }
  }
  return Q_SUPER(&QHsm::top);
}

QState QDevMortor::onIdle(QDevMortor *me, QEvent const *e) {
  switch (e->sig) {
  case Q_ENTRY_SIG: 
    {     
      return Q_HANDLED();
    }
  case BROAD_COMM_SIG: 
    {               
      me->CmdDivider(((CommandEvt *)e)->CommStr);
      if(!me->go_Idle) {
        return Q_TRAN(&QDevMortor::pulsing); 
      }
      return Q_HANDLED();               
    }
  }
  return Q_SUPER(&QHsm::top);
}

void QDevMortor::CmdExecutor(QDevMortor* me, CmdInfo* p) {

  switch (p->cmdLetter[0]) {
  case 'B':
  case 'b': 
    {
      me->SetPwm(0);
      digitalWrite(me->In1_pin, HIGH);
      digitalWrite(me->In2_pin, HIGH);
      break;      
    }
  case 'C':
  case 'c': 
    {
      switch (p->cmdLetter[1]) {
        case 'C':
        case 'c':
        {
          me->SetPwm(0);
          me->pwm_pin = me->In2_pin;
          digitalWrite(me->In1_pin, LOW);
          break;
        }
        case 'W':
        case 'w':
        {
          me->SetPwm(0);
          me->pwm_pin = me->In1_pin;
          digitalWrite(me->In2_pin, LOW);
          break;
        }      
      }
      break;
    }    
  case 'D':
  case 'd': 
    {
      me->SetPwm(p->cmdValue[1]);
      break;     
    }
  case 'I':
  case 'i': 
    {
      me->go_Idle = true;
      break;
    }       
  case 'P':
  case 'p':
    {
      me->go_Idle = false;
      break;
    }
  case 'X':
  case 'x': 
    {
      me->SetPwm(0);
      digitalWrite(me->In1_pin, LOW);
      digitalWrite(me->In2_pin, LOW);
      break;      
    }      
  default : 
    { 
      break; 
    }
  }
}

//............................................................................

QDevMortorVref::QDevMortorVref(
  const uint8_t  id,
  const uint8_t  in1,
  const uint8_t  in2,
  const uint8_t  vrf
) : QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial),
  m_FREQtimingEvt(FREQ_SIG), m_LOWtimingEvt(LOW_SIG),
  In1_pin(in1),
  In2_pin(in2),
  vref(vrf) {
    
  pinMode(In1_pin, OUTPUT);
  pinMode(In2_pin, OUTPUT);
  pinMode(vref,    OUTPUT);

  go_Idle   = true;  
  curr_val = 0;

  SetPwm(0);  
}

void QDevMortorVref::SetPwm(uint8_t v) {
  
  uint8_t pre_val = curr_val;
  curr_val  = v;  
  
  if((v > 0) && (v < 255)) {

    uint32_t wait_time = 7.84 * v;
    
    wait_tick = wait_time / 200;
    remcnt    = wait_time % 200;
    
    if(pre_val < 1 || pre_val > 254) { m_FREQtimingEvt.postIn(this, 1); }
    
  } else {
    
    wait_tick  = 0;
    remcnt     = 0;

    if(v == 0) {
      digitalWrite(vref, LOW);
    } else if(v == 255) {
      digitalWrite(vref, HIGH);
    }
  }
}

QState QDevMortorVref::initial (QDevMortorVref *me, QEvent const *e) {
  me->subscribe(BROAD_COMM_SIG);
  return Q_TRAN(&QDevMortorVref::onIdle);
}

QState QDevMortorVref::pulsing(QDevMortorVref *me, QEvent const *e) {
  switch (e->sig) {
  case Q_ENTRY_SIG:
    {
      return Q_HANDLED();
    }
  case BROAD_COMM_SIG: 
    {
      me->CmdDivider(((CommandEvt *)e)->CommStr);
      if(me->go_Idle) {
        me->SetPwm(0);
        return Q_TRAN(&QDevMortorVref::onIdle); 
      }
      return Q_HANDLED();
    }
  case FREQ_SIG:
    {
      if(me->curr_val == 0 || me->curr_val == 255) { return Q_HANDLED(); }
      
      digitalWrite(me->vref, LOW);
      
      if(me->remcnt < 100) {
        digitalWrite(me->vref, HIGH);
        
        if(me->wait_tick > 0) {
          me->m_LOWtimingEvt.postIn(me, me->wait_tick);
        } else {
          delayMicroseconds(me->remcnt);
          digitalWrite(me->vref, LOW);
        }
      } else {        
        if((200 - me->remcnt) > 2) { delayMicroseconds(200 - me->remcnt); }
        digitalWrite(me->vref, HIGH);
        
        if((me->wait_tick + 1) < 10) {
          me->m_LOWtimingEvt.postIn(me, (me->wait_tick + 1));
        }
      }      
      me->m_FREQtimingEvt.postIn(me, 10);
      return Q_HANDLED();
    }
  case LOW_SIG:
    {
      if(me->curr_val == 0 || me->curr_val == 255) { return Q_HANDLED(); }
      
      if(me->remcnt > 2 && me->remcnt < 100) { delayMicroseconds(me->remcnt); }
      digitalWrite(me->vref, LOW);
      
      return Q_HANDLED();
    }
  }
  return Q_SUPER(&QHsm::top);
}

QState QDevMortorVref::onIdle(QDevMortorVref *me, QEvent const *e) {
  switch (e->sig) {
  case Q_ENTRY_SIG: 
    {     
      return Q_HANDLED();
    }
  case BROAD_COMM_SIG: 
    {               
      me->CmdDivider(((CommandEvt *)e)->CommStr);
      if(!me->go_Idle) {
        return Q_TRAN(&QDevMortorVref::pulsing); 
      }
      return Q_HANDLED();               
    }
  }
  return Q_SUPER(&QHsm::top);
}

void QDevMortorVref::CmdExecutor(QDevMortorVref* me, CmdInfo* p) {

  switch (p->cmdLetter[0]) {
  case 'B':
  case 'b': 
    {
      me->SetPwm(0);
      digitalWrite(me->In1_pin, HIGH);
      digitalWrite(me->In2_pin, HIGH);
      break;      
    }
  case 'C':
  case 'c': 
    {
      switch (p->cmdLetter[1]) {
        case 'C':
        case 'c':
        {
          me->SetPwm(0);
          digitalWrite(me->In2_pin, HIGH);
          digitalWrite(me->In1_pin, LOW);
          break;
        }
        case 'W':
        case 'w':
        {
          me->SetPwm(0);
          digitalWrite(me->In1_pin, HIGH);
          digitalWrite(me->In2_pin, LOW);
          break;
        }      
      }
      break;
    }    
  case 'D':
  case 'd': 
    {
      me->SetPwm(p->cmdValue[1]);
      break;     
    }
  case 'I':
  case 'i': 
    {
      me->go_Idle = true;
      break;
    }       
  case 'P':
  case 'p':
    {
      me->go_Idle = false;
      break;
    }
  case 'X':
  case 'x': 
    {
      me->SetPwm(0);
      digitalWrite(me->In1_pin, LOW);
      digitalWrite(me->In2_pin, LOW);
      break;      
    }      
  default : 
    { 
      break; 
    }
  }
}
//............................................................................

trickLEDgroup::trickLEDgroup(uint8_t id, uint8_t s, uint8_t e, uint16_t itr)
: QDevice(id, (QDcmdHandler)CmdExecutor, (QStateHandler)initial),
m_timeEvt(TIMEOUT_SIG), s_pin(s), e_pin(e) {       
  cur_pin = s;    
  itrvl = itr;

  uint8_t i;
  for(i = s_pin; i <= e_pin; ++i) {
    pinMode(i, OUTPUT);
  }    
}

QState trickLEDgroup::initial(trickLEDgroup *me, QEvent const *) {
  me->subscribe(BROAD_COMM_SIG);
  digitalWrite(me->cur_pin, HIGH);
  me->m_timeEvt.postIn(me, me->itrvl);
  return Q_TRAN(&trickLEDgroup::blinkForward);
}

QState trickLEDgroup::blinkForward(trickLEDgroup *me, QEvent const *e) {
  switch (e->sig) {
  case Q_ENTRY_SIG: 
    {
      return Q_HANDLED();
    }
  case BROAD_COMM_SIG: 
    {
      me->CmdDivider(((CommandEvt *)e)->CommStr);
      return Q_HANDLED();
    }
  case TIMEOUT_SIG: 
    {
      digitalWrite(me->cur_pin, LOW); 
      ++me->cur_pin;
      digitalWrite(me->cur_pin, HIGH);

      me->m_timeEvt.postIn(me, me->itrvl);              
      if(me->cur_pin == me->e_pin) return Q_TRAN(&trickLEDgroup::blinkBackward);

      return Q_HANDLED();
    }
  }
  return Q_SUPER(&QHsm::top);
}

QState trickLEDgroup::blinkBackward(trickLEDgroup *me, QEvent const *e) {
  switch (e->sig) {
  case Q_ENTRY_SIG: 
    {
      return Q_HANDLED();
    }
  case BROAD_COMM_SIG: 
    {
      me->CmdDivider(((CommandEvt *)e)->CommStr);
      return Q_HANDLED();
    }
  case TIMEOUT_SIG: 
    {
      digitalWrite(me->cur_pin, LOW); 
      --me->cur_pin;
      digitalWrite(me->cur_pin, HIGH);

      me->m_timeEvt.postIn(me, me->itrvl);              
      if(me->cur_pin == me->s_pin) return Q_TRAN(&trickLEDgroup::blinkForward);

      return Q_HANDLED();
    }
  }
  return Q_SUPER(&QHsm::top);
}

void trickLEDgroup::CmdExecutor(trickLEDgroup* me, CmdInfo* p) {
  Serial.println("trick!");
  for(;;);
}
