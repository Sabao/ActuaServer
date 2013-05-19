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

#define PROC_id        1
#define TOTAL_OF_DEV   3
#define cmdSIZE       20

enum qdSignals {
   BROAD_COMM_SIG = QP::Q_USER_SIG,
   SI_EMGCY_SIG,
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

struct MsgEvt : public QP::QEvent {};

struct CommandEvt : public QP::QEvent {
    char CommStr[cmdSIZE];
};

//command
#define INTERRUPT    128
#define FLUSH_QUEUE  64

//devID
#define NO_RETURN    128
#define NO_ARRAY     64

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
    int8_t  check_id(const char*);
    void    send_cmd(const char*, uint8_t, char);
    
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

class QDevStepper : public QDevice {
  private:  
    QP::QTimeEvt m_timeEvt;

    uint8_t  excPattern;

    uint16_t itrvl;    
    uint16_t stepcnt;

    bool     r_direction;
    bool     cntdown;

    uint8_t  MaskTable[4][2];  //[pinNumber][pinMask]

    static void CmdExecutor(QDevStepper*, CmdInfo*);

    static QP::QState initial (QDevStepper *me, QP::QEvent const *e);
    static QP::QState Stepping (QDevStepper *me, QP::QEvent const *e);
    static QP::QState onIdle (QDevStepper *me, QP::QEvent const *e);

  public:
    QDevStepper(uint8_t  id,                  
      uint8_t  x,
      uint8_t  y,
      uint8_t  x_,
      uint8_t  y_,
      uint8_t  stepMode = WAVE_DRIVE,
      uint16_t itr = 100,
      uint16_t cnt = 0,
      bool     dir = false); 
};

//............................................................................

class QDevServo;

class ServoTact : public QDevice {

  friend class QDevServo;
  
  private:
    QP::QTimeEvt      m_HIGHtimingEvt;
    QDevServo*  svo[8];
    uint8_t       curr_svo;

  public:
    ServoTact(uint8_t);

  private:
    bool   Attach(QDevServo *p);
    void   Detach(QDevServo *p);
    static QP::QState initial (ServoTact *me, QP::QEvent const *e);
    static QP::QState conducting(ServoTact *me, QP::QEvent const *e);
};

//............................................................................

class QDevServo : public QDevice {
  
  friend class ServoTact;

  private:
    QP::QTimeEvt   m_LOWtimingEvt;
  
  public:
    QDevServo(
      const uint8_t     id,
      const uint8_t     ctrl,
      uint16_t bWidth = 1000,
      uint16_t rWidth = 1000,
      uint8_t  Speed  = 120
    );
    
  private:
    const    uint8_t ctrlpin;

    bool     ch_deg;
    bool     go_Idle;

    uint32_t startTime;
    uint32_t finishTime;

    uint16_t baseWidth;
    uint16_t rangeWidth;
    uint8_t  Speed;

    uint8_t  curr_deg;
    uint8_t  pre_deg;

    uint8_t  tickcnt;
    uint8_t  waitcnt;
    uint16_t sleepcnt;

    uint16_t appendedWidth;

    void          SetDegree(uint8_t deg);   
    void          SetHigh();
    static void   CmdExecutor(QDevServo*, CmdInfo*);

    static QP::QState initial (QDevServo *me, QP::QEvent const *e);
    static QP::QState pulsing (QDevServo *me, QP::QEvent const *e);
    static QP::QState onIdle  (QDevServo *me, QP::QEvent const *e);
};

//............................................................................

class ArduServo : public QDevice {
  
  private:
    Servo    ardusvo;
    QP::QTimeEvt m_DONEtimingEvt;

  public:
    ArduServo(
      const uint8_t     id,
      const uint8_t     ctrl,
      uint16_t bWidth = 1000,
      uint16_t rWidth = 1000,
      uint16_t Speed  = 120
    );
    
  private:
    const    uint8_t ctrlpin;
    
    bool     go_Idle;
    
    uint16_t baseWidth;
    uint16_t rangeWidth;
    uint16_t Speed;
    
    bool     moving;
    
    static void   CmdExecutor(ArduServo*, CmdInfo*);

    static QP::QState initial (ArduServo *me, QP::QEvent const *e);
    static QP::QState pulsing (ArduServo *me, QP::QEvent const *e);
    static QP::QState onIdle  (ArduServo *me, QP::QEvent const *e);
};

//............................................................................

class QDevMortor : public QDevice {
  
  private:
  
    const    uint8_t In1_pin;
    const    uint8_t In2_pin;
    
    uint8_t  pwm_pin;    
    
    int8_t   curr_duty;
    
    QP::QTimeEvt m_FREQtimingEvt;
    QP::QTimeEvt m_LOWtimingEvt;
    
    bool     go_Idle;
    
    uint16_t wait_tick;
    uint8_t  remcnt;
    
  public:
    QDevMortor(
      const uint8_t  id,
      const uint8_t  in1,
      const uint8_t  in2
      );
    
  private:

    void SetPwm(int8_t d);
    
    static void   CmdExecutor(QDevMortor*, CmdInfo*);

    static QP::QState initial (QDevMortor *me, QP::QEvent const *e);
    static QP::QState pulsing (QDevMortor *me, QP::QEvent const *e);
    static QP::QState onIdle  (QDevMortor *me, QP::QEvent const *e);
};

//............................................................................

class QDevMortorVref : public QDevice {
  
  private:
  
    const    uint8_t In1_pin;
    const    uint8_t In2_pin;
    const    uint8_t vref;
    
    uint8_t   curr_val;
    
    QP::QTimeEvt m_FREQtimingEvt;
    QP::QTimeEvt m_LOWtimingEvt;
    
    bool     go_Idle;
    
    uint16_t wait_tick;
    uint8_t  remcnt;
    
  public:
    QDevMortorVref(
      const uint8_t  id,
      const uint8_t  in1,
      const uint8_t  in2,
      const uint8_t  vrf
      );
    
  private:

    void SetPwm(uint8_t v);
    
    static void   CmdExecutor(QDevMortorVref*, CmdInfo*);

    static QP::QState initial (QDevMortorVref *me, QP::QEvent const *e);
    static QP::QState pulsing (QDevMortorVref *me, QP::QEvent const *e);
    static QP::QState onIdle  (QDevMortorVref *me, QP::QEvent const *e);
};

//............................................................................

class trickLEDgroup : public QDevice {
  private:
    QP::QTimeEvt m_timeEvt;

  public:
    trickLEDgroup(
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
    
  static void CmdExecutor(trickLEDgroup* me, CmdInfo* p);

  static QP::QState initial (trickLEDgroup *me, QP::QEvent const *e);
  static QP::QState blinkForward(trickLEDgroup *me, QP::QEvent const *e);
  static QP::QState blinkBackward(trickLEDgroup *me, QP::QEvent const *e);
};
//............................................................................

#endif

