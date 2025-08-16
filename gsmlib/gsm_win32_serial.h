// *************************************************************************
// * GSM TA/ME library
// *
// * File:    gsm_win32_port.h
// *
// * Purpose: WIN32 serial port implementation
// *
// * Author:  Frediano Ziglio (freddy77@angelfire.com)
// *
// * Created: 25.10.2000
// *************************************************************************

#ifndef GSM_WIN32_SERIAL_H
#define GSM_WIN32_SERIAL_H

#include <string>
#include <gsmlib/gsm_error.h>
#include <gsmlib/gsm_port.h>
#include <gsmlib/gsm_util.h>
#define WIN32_MEAN_AND_LEAN
#include <windows.h>

namespace gsmlib
{
  using std::string;

  class Win32SerialPort : public Port
  {
  private:
    HANDLE _file;               // file handle for device
    int _oldChar;               // character set by putBack() (-1 == none)
//    OVERLAPPED _overIn;         // overlapped structure for wait

    // throw GsmException include UNIX errno
    void throwModemException(string message);
    
  public:
    // create Port given the UNIX device name
    Win32SerialPort(string device, int lineSpeed = DEFAULT_BAUD_RATE,
                   string initString = DEFAULT_INIT_STRING,
                   bool swHandshake = false)

    // inherited from Port
    void putBack(unsigned char c);
    int readByte();
    string getLine();
    void putLine(string line,
                         bool carriageReturn = true);
    bool wait(GsmTime timeout);
    void setTimeOut(unsigned int timeout);

    virtual ~Win32SerialPort();
  };

  // convert baudrate string ("300" .. "460800") to speed_t
  extern int baudRateStrToSpeed(string baudrate);
};

#endif // GSM_UNIX_SERIAL_H
