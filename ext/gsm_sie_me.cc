
#ifdef HAVE_CONFIG_H
#include <gsm_config.h>
#endif
#include <gsmlib/gsm_me_ta.h>
#include <gsmlib/gsm_parser.h>
#include <gsm_sie_me.h>

using namespace std;
using namespace gsmlib;

// SieMe members

void SieMe::init() throw(GsmException)
{
}

SieMe::SieMe(Ref<Port> port) throw(GsmException) : MeTa::MeTa(port)
{
  // initialize Siemens ME

  init();
}

vector<string> SieMe::getSupportedPhonebooks() throw(GsmException)
{
  Parser p(_at->chat("^SPBS=?", "^SPBS:"));
  return p.parseStringList();
}

string SieMe::getCurrentPhonebook() throw(GsmException)
{
  if (_lastPhonebookName == "")
  {
    Parser p(_at->chat("^SPBS?", "^SPBS:"));
    // answer is e.g. ^SPBS: "SM",41,250
    _lastPhonebookName = p.parseString();
    p.parseComma();
    int _currentNumberOfEntries = p.parseInt();
    p.parseComma();
    int _maxNumberOfEntries = p.parseInt();
  }
  return _lastPhonebookName;
}

void SieMe::setPhonebook(string phonebookName) throw(GsmException)
{
  if (phonebookName != _lastPhonebookName)
  {
    _at->chat("^SPBS=\"" + phonebookName + "\"");
    _lastPhonebookName = phonebookName;
  }
}


IntRange SieMe:: getSupportedSignalTones() throw(GsmException)
{
  Parser p(_at->chat("^SPST=?", "^SPST:"));
  // ^SPST: (0-4),(0,1)
  IntRange typeRange = p.parseRange();
  p.parseComma();
  vector<bool> volumeList = p.parseIntList();
  return typeRange;
}

void SieMe:: playSignalTone(int tone) throw(GsmException)
{
  _at->chat("^SPST=" + intToStr(tone) + ",1");
}

void SieMe:: stopSignalTone(int tone) throw(GsmException)
{
  _at->chat("^SPST=" + intToStr(tone) + ",0");
}


IntRange SieMe::getSupportedRingingTones() throw(GsmException) // (AT^SRTC=?)
{
  Parser p(_at->chat("^SRTC=?", "^SRTC:"));
  // ^SRTC: (0-42),(1-5)
  IntRange typeRange = p.parseRange();
  p.parseComma();
  IntRange volumeRange = p.parseRange();
  return typeRange;
}

int SieMe::getCurrentRingingTone() throw(GsmException) // (AT^SRTC?)
{
  Parser p(_at->chat("^SRTC?", "^SRTC:"));
  // ^SRTC: 41,2,0
  int type = p.parseInt();
  p.parseComma();
  int volume = p.parseInt();
  p.parseComma();
  int ringing = p.parseInt();
  return type;
}

void SieMe::setRingingTone(int tone, int volume) throw(GsmException)
{
  _at->chat("^SRTC=" + intToStr(tone) + "," + intToStr(volume));
}

void SieMe:: ringingToneOn() throw(GsmException) // (AT^SRTC)
{
  // get ringing bool
  Parser p(_at->chat("^SRTC?", "^SRTC:"));
  // ^SRTC: 41,2,0
  int type = p.parseInt();
  p.parseComma();
  int volume = p.parseInt();
  p.parseComma();
  int ringing = p.parseInt();

  if (ringing == 0)
    _at->chat("^SRTC");
}

void SieMe::ringingToneOff() throw(GsmException) // (AT^SRTC)
{
  // get ringing bool
  Parser p(_at->chat("^SRTC?", "^SRTC:"));
  // ^SRTC: 41,2,0
  int type = p.parseInt();
  p.parseComma();
  int volume = p.parseInt();
  p.parseComma();
  int ringing = p.parseInt();

  if (ringing == 1)
    _at->chat("^SRTC");
}

void SieMe::toggleRingingTone() throw(GsmException) // (AT^SRTC)
{
  _at->chat("^SRTC");
}

// Siemens get supported binary read
vector<string> SieMe::getSupportedBinaryReads() throw(GsmException)
{
  Parser p(_at->chat("^SBNR=?", "^SBNR:"));
  // ^SBNR: ("bmp",(0-3)),("mid",(0-4)),("vcf",(0-500)),("vcs",(0-50))

  return p.parseStringList();
}

// Siemens get supported binary write
vector<string> SieMe::getSupportedBinaryWrites() throw(GsmException)
{
  Parser p(_at->chat("^SBNW=?", "^SBNW:"));
  // ^SBNW: ("bmp",(0-3)),("mid",(0-4)),("vcf",(0-500)),("vcs",(0-50)),("t9d",(0))

  return p.parseStringList();
}

// Siemens Binary Read
void SieMe::getBinary(string binary) throw(GsmException) // (AT^SBNR)
{
    // same as chat() above but also get pdu if expectPdu == true
    //string chat(string atCommand,
    //            string response,
    //            string &pdu,
    //            bool ignoreErrors = false,
    //            bool expectPdu = true,
    //            bool acceptEmptyResponse = false) throw(GsmException);

    // same as above, but expect several response lines
    //vector<string> chatv(string atCommand = "",
    //                     string response = "",
    //                     bool ignoreErrors = false)
}

// Siemens Binary Write
void SieMe::setBinary(string binary) throw(GsmException) // (AT^SBNW)
{
    // send pdu (wait for <CR><LF><greater_than><space> and send <CTRL-Z>
    // at the end
    // return text after response
    //string sendPdu(string atCommand, string response,
    //               string pdu) throw(GsmException);
}

