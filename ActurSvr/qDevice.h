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
#include <Servo.h>

#define PROC_id        0
#define TOTAL_OF_DEV   2
#define cmdSIZE       20

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
  SI_RETURN_SIG,
  SI_DEQUE_SIG
};

//command-option
#define ENQUEUE  64
#define DEQUEUE  128
#define FLUSH    192

#define WAVE_DRIVE   0x11
#define HALF_STEP    0X07
#define FULL_STEP    0x33

#define STAY         0x01
#define ALIVE        0x02
#define CHKECHO      0x04
#define SHUT         0x08
#define EMGCY        0x80

struct CmdInfo {
  char           buffcpy[cmdSIZE];
  char           cmdLetter[2];
  int16_t        cmdValue[2];
};

struct CmdQueue {
  CmdQueue* next;
  char cmdString[cmdSIZE];
};

//............................................................................

class QDevice : public QP::QActive {
  public:
    typedef  void (*QDcmdHandler)(QDevice*, CmdInfo*);
    bool rsv();

  private:
    const    QDcmdHandler clbkfunc;
    const    uint8_t devID;
    void     send_cmd(const char*, uint8_t, char);
    
    CmdQueue*  first;
    CmdQueue*  last;

  public:
    QDevice(
      uint8_t,
      QDcmdHandler,
      QP::QStateHandler
    );

  uint8_t getID();
  
  bool CmdDivider(const char*);
  
  bool EnqueueCmd(const char*);
  bool DequeueCmd();
  void FlushQueue();
  
  void InternalCmd(uint8_t, int16_t, int16_t, char);
  void InternalCmd(uint8_t, int16_t, char ,char);
  void InternalCmd(uint8_t, char, int16_t, char);
  void InternalCmd(uint8_t, char, char, char);

};

//............................................................................

class SerialInterface : public QDevice {

  public:
    SerialInterface(uint8_t);
    void On_ISR();
    
    bool IsEmgcy()    { return stat_flg & EMGCY; };
    bool send_to_serial(int8_t, int8_t, int16_t, int16_t);
    bool send_to_serial(int8_t, int8_t, int16_t, char);
    bool send_to_serial(int8_t, int8_t, char, int16_t);
    bool send_to_serial(int8_t, int8_t, char, char);
    
  private:
    QP::QTimeEvt m_keep_alive_timer;
    volatile uint8_t   stat_flg;

    char  read_buf[cmdSIZE];
    char  check_buf[cmdSIZE];
    char* rp;
    char  c;

    void SI_prefix(char*, uint8_t, int8_t);
    
  static void CmdExecutor(SerialInterface*, CmdInfo*);

  static QP::QState initial (SerialInterface *me, QP::QEvent const *e);
  static QP::QState Exchange (SerialInterface *me, QP::QEvent const *e);
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

  static void CmdExecutor(LEDgroup*, CmdInfo*);

  static QP::QState initial (LEDgroup *me, QP::QEvent const *e);
  static QP::QState blinkForward(LEDgroup *me, QP::QEvent const *e);
  static QP::QState blinkBackward(LEDgroup *me, QP::QEvent const *e);
};

//............................................................................

#endif

