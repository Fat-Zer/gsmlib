// *************************************************************************
// * GSM TA/ME library
// *
// * File:    gsm_me_ta.cc
// *
// * Purpose: Mobile Equipment/Terminal Adapter functions
// *          (ETSI GSM 07.07)
// *
// * Author:  Peter Hofmann (software@pxh.de)
// *
// * Created: 10.5.1999
// *************************************************************************

#ifdef HAVE_CONFIG_H
#include <gsm_config.h>
#endif
#include <gsmlib/gsm_nls.h>
#include <gsmlib/gsm_me_ta.h>
#include <gsmlib/gsm_parser.h>

using namespace std;
using namespace gsmlib;

// Capabilities members

Capabilities::Capabilities() :
  _hasSMSSCAprefix(true),
  _cpmsParamCount(-1),          // initialize to -1, must be set later by
                                // setSMSStore() function
  _omitsColon(true)             // FIXME
{
}

// MeTa members

void MeTa::init() throw(GsmException)
{
  // switch on extended error codes
  // caution: may be ignored by some TAs, so allow it to fail
  _at->chat("+CMEE=1", "", true, true);
  
  // select SMS pdu mode
  _at->chat("+CMGF=0");

  // now fill in capability object
  MEInfo info = getMEInfo();
  
  // Ericsson model 6050102
  if (info._manufacturer == "ERICSSON" &&
      (info._model == "1100801" ||
       info._model == "1140801") ||
      getenv("GSMLIB_SH888_FIX") != NULL)
  {
    // the Ericsson leaves out the service centre address
    _capabilities._hasSMSSCAprefix = false;
  }
  
  // set default event handler
  // necessary to handle at least RING indications that might
  // otherwise confuse gsmlib
  _at->setEventHandler(&_defaultEventHandler);
}

MeTa::MeTa(Ref<Port> port) throw(GsmException) : _port(port)
{
  // initialize AT handling
  _at = new GsmAt(*this);

  init();
}

// MeTa::MeTa(Ref<GsmAt> at) throw(GsmException) :
//   _at(at)
// {
//   init();
// }

void MeTa::setPhonebook(string phonebookName) throw(GsmException)
{
  if (phonebookName != _lastPhonebookName)
  {
    _at->chat("+CPBS=\"" + phonebookName + "\"");
    _lastPhonebookName = phonebookName;
  }
}

string MeTa::setSMSStore(string smsStore, bool needResultCode)
  throw(GsmException)
{
  if (_capabilities._cpmsParamCount == -1)
  {
    // count the number of parameters for the CPMS AT sequences
    _capabilities._cpmsParamCount = 1;
    Parser p(_at->chat("+CPMS=?", "+CPMS:"));
    p.parseStringList();
    while (p.parseComma(true))
    {
      ++_capabilities._cpmsParamCount;
      p.parseStringList();
    }
  }

  // optimatization: only set current SMS store if different from last call
  // or the result code is needed
  if (needResultCode || _lastSMSStoreName != smsStore)
  {
    _lastSMSStoreName = smsStore;

    // build chat string
    string chatString = "+CPMS=\"" + smsStore + "\"";
    for (int i = 1; i < _capabilities._cpmsParamCount; ++i)
      chatString += ",\"" + smsStore + "\"";

    return _at->chat(chatString, "+CPMS:");
  }
  return "";
}

void MeTa::waitEvent(GsmTime timeout) throw(GsmException)
{
  if (_at->wait(timeout))
    _at->chat();                // send AT, wait for OK, handle events
}

MEInfo MeTa::getMEInfo() throw(GsmException)
{
  MEInfo result;
  // some TAs just return OK and no info line
  // leave the info empty in this case
  result._manufacturer = _at->chat("+CGMI", "+CGMI:", false, true);
  result._model = _at->chat("+CGMM", "+CGMM:", false, true);
  result._revision = _at->chat("+CGMR", "+CGMR:", false, true);
  result._serialNumber = _at->chat("+CGSN", "+CGSN:", false, true);
  return result;
}

string MeTa::getExtendedErrorReport() throw(GsmException)
{
  return _at->chat("+CEER", "+CEER:");
}

void MeTa::dial(string number) throw(GsmException)
{
  _at->chat("D" + number + ";");
}

vector<OPInfo> MeTa::getAvailableOPInfo() throw(GsmException)
{
  vector<OPInfo> result;
  vector<string> responses = _at->chatv("+COPS=?", "+COPS:");
  // GSM modems might return
  // 1. quadruplets of info enclosed in brackets separated by comma
  // 2. several lines of quadruplets of info enclosed in brackets
  // 3. several lines of quadruplets without brackets and additional
  //    info at EOL (e.g. Nokia 8290)
  for (vector<string>::iterator i = responses.begin();
       i != responses.end(); ++i)
  {
    bool expectClosingBracket = false;
    Parser p(*i);
    while (1)
    {
      OPInfo opi;
      if (p.parseComma(true)) break;
      expectClosingBracket = p.parseChar('(', true);
      int status = p.parseInt(true);
      opi._status = (status == NOT_SET ? UnknownOPStatus : (OPStatus)status);
      p.parseComma();
      opi._longName = p.parseString(true);
      p.parseComma();
      opi._shortName = p.parseString(true);
      p.parseComma();
      try
      {
        opi._numericName = p.parseInt(true);
      }
      catch (GsmException &e)
      {
        if (e.getErrorClass() == ParserError)
        {
          // the Ericsson GM12 GSM modem returns the numeric ID as string
          string s = p.parseString();
          opi._numericName = checkNumber(s);
        }
        else
          throw e;
      }
      if (expectClosingBracket) p.parseChar(')');
      p.parseComma(true);
      result.push_back(opi);
    }
    // without brackets, the ME/TA must use format 3.
    if (! expectClosingBracket) break;
  }
  return result;
}

OPInfo MeTa::getCurrentOPInfo() throw(GsmException)
{
  OPInfo result;
  bool done = false;

  // 1. This exception thing is necessary because not all ME/TA combinations
  // might support all the formats and then return "ERROR".
  // 2. Additionally some modems return "ERROR" for all "COPS=3,n" command
  // and report only one format with the "COPS?" command (e.g. Nokia 8290).

  // get long format
  try
  {
    try
    {
      _at->chat("+COPS=3,0");
    }
    catch (GsmException &e)
    {
      if (e.getErrorClass() != ChatError) throw;
    }
    Parser p(_at->chat("+COPS?", "+COPS:"));
    result._mode = (OPModes)p.parseInt();
    p.parseComma();
    if (p.parseInt() == 0)
    {
      p.parseComma();
      result._longName = p.parseString();
      done = true;
    }
  }
  catch (GsmException &e)
  {
    if (e.getErrorClass() != ChatError) throw;
  }

  // get short format
  try
  {
    try
    {
      _at->chat("+COPS=3,1");
    }
    catch (GsmException &e)
    {
      if (e.getErrorClass() != ChatError) throw;
    }
    Parser p(_at->chat("+COPS?", "+COPS:"));
    result._mode = (OPModes)p.parseInt();
    p.parseComma();
    if (p.parseInt() == 1)
    {
      p.parseComma();
      result._shortName = p.parseString();
      done = true;
    }
  }
  catch (GsmException &e)
  {
    if (e.getErrorClass() != ChatError) throw;
  }

  // get numeric format
  try
  {
    try
    {
      _at->chat("+COPS=3,2");
    }
    catch (GsmException &e)
    {
      if (e.getErrorClass() != ChatError) throw;
    }
    Parser p(_at->chat("+COPS?", "+COPS:"));
    result._mode = (OPModes)p.parseInt();
    p.parseComma();
    if (p.parseInt() == 2)
    {
      p.parseComma();
      try
      {
        result._numericName = p.parseInt();
      }
      catch (GsmException &e)
      {
        if (e.getErrorClass() == ParserError)
        {
          // the Ericsson GM12 GSM modem returns the numeric ID as string
          string s = p.parseString();
          result._numericName = checkNumber(s);
        }
        else
          throw e;
      }
      done = true;
    }
  }
  catch (GsmException &e)
  {
    if (e.getErrorClass() != ChatError) throw;
  }
  if (! done)
    throw GsmException(_("cannot read current network operator"), OtherError);
  return result;
}

void MeTa::setCurrentOPInfo(OPModes mode,
                            string longName,
                            string shortName,
                            int numericName) throw(GsmException)
{
  bool done = false;
  if (longName != "")
  {
    try
    {
      _at->chat("+COPS=" + intToStr((int)mode) + ",0,\"" + longName + "\"");
      done = true;
    }
    catch (GsmException &e)
    {
      if (e.getErrorClass() != ChatError) throw;
    }
  }
  if (shortName != "" && ! done)
  {
    try
    {
      _at->chat("+COPS=" + intToStr((int)mode) + ",1,\"" + shortName + "\"");
      done = true;
    }
    catch (GsmException &e)
    {
      if (e.getErrorClass() != ChatError) throw;
    }
  }
  if (numericName != NOT_SET && ! done)
  {
    try
    {
      _at->chat("+COPS=" + intToStr((int)mode) + ",2," +
                intToStr(numericName));
      done = true;
    }
    catch (GsmException &e)
    {
      if (e.getErrorClass() != ChatError) throw;
    }
  }
  if (! done)
    throw GsmException(_("unable to set operator"), OtherError);
}

vector<string> MeTa::getFacilityLockCapabilities() throw(GsmException)
{
  Parser p(_at->chat("+CLCK=?", "+CLCK:"));
  return p.parseStringList();
}

bool MeTa::getFacilityLockStatus(string facility, FacilityClass cl)
  throw(GsmException)
{
  Parser p(_at->chat("+CLCK=\"" + facility + "\",2,," + intToStr((int)cl),
                     "+CLCK:"));
  return p.parseInt() == 1;
}

void MeTa::lockFacility(string facility, FacilityClass cl, string passwd)
  throw(GsmException)
{
  if (passwd == "")
    _at->chat("+CLCK=\"" + facility + "\",1,," + intToStr((int)cl));
  else
    _at->chat("+CLCK=\"" + facility + "\",1,\"" + passwd + "\","
              + intToStr((int)cl));
}

void MeTa::unlockFacility(string facility, FacilityClass cl, string passwd)
  throw(GsmException)
{
  if (passwd == "")
    _at->chat("+CLCK=\"" + facility + "\",0,," + intToStr((int)cl));
  else
    _at->chat("+CLCK=\"" + facility + "\",0,\"" + passwd + "\","
              + intToStr((int)cl));
}

vector<PWInfo> MeTa::getPasswords() throw(GsmException)
{
  vector<PWInfo> result;
  Parser p(_at->chat("+CPWD=?", "+CPWD:"));
  while (1)
  {
    PWInfo pwi;
    if (!p.parseChar('(', true)) break; // exit if no new tuple
    pwi._facility = p.parseString();
    p.parseComma();
    pwi._maxPasswdLen = p.parseInt();
    p.parseChar(')');
    p.parseComma(true);
    result.push_back(pwi);
  }
  return result;
}

void MeTa::setPassword(string facility, string oldPasswd, string newPasswd)
  throw(GsmException)
{
  _at->chat("+CPWD=\"" + facility + "\",\"" + oldPasswd + "\",\"" +
            newPasswd + "\"");
}

bool MeTa::getNetworkCLIP() throw(GsmException)
{
  Parser p(_at->chat("+CLIP?", "+CLIP:"));
  p.parseInt();                 // ignore result code presentation
  p.parseComma();
  return p.parseInt() == 1;
}

void MeTa::setCLIPPresentation(bool enable) throw(GsmException)
{
  if (enable)
    _at->chat("+CLIP=1");
  else
    _at->chat("+CLIP=0");
}

bool MeTa::getCLIPPresentation() throw(GsmException)
{
  Parser p(_at->chat("+CLIP?", "+CLIP:"));
  return p.parseInt() == 1;     // ignore rest of line
}

void MeTa::setCallForwarding(ForwardReason reason,
                             ForwardMode mode,
                             string number,
                             string subaddr,
                             FacilityClass cl,
                             int forwardTime) throw(GsmException)
{
  // FIXME subaddr is currently ignored
  if (forwardTime != NOT_SET && (forwardTime < 0 || forwardTime > 30))
    throw GsmException(_("call forward time must be in the range 0..30"),
                       ParameterError);
  
  int numberType;
  number = removeWhiteSpace(number);
  if (number.length() > 0 && number[0] == '+')
  {
    numberType = InternationalNumberFormat;
    number = number.substr(1);  // skip the '+' at the beginning
  }
  else
    numberType = UnknownNumberFormat;
  _at->chat("+CCFC=" + intToStr(reason) + "," +  intToStr(mode) + "," 
            "\"" + number + "\"," +
            (number.length() > 0 ? intToStr(numberType) : "") +
            "," +  intToStr(cl) +
                                // FIXME subaddr and type
            (forwardTime == NOT_SET ? "" :
             (",,," + intToStr(forwardTime))));
}
                           
void MeTa::getCallForwardInfo(ForwardReason reason,
                              ForwardInfo &voice,
                              ForwardInfo &fax,
                              ForwardInfo &data) throw(GsmException)
{
  vector<string> responses =
    _at->chatv("+CCFC=" + intToStr(reason) + ",2", "+CCFC:");
  for (vector<string>::iterator i = responses.begin();
       i != responses.end(); ++i)
  {
    Parser p(*i);
    int status = p.parseInt();
    p.parseComma();
    FacilityClass cl = (FacilityClass)p.parseInt();
    string number;
    string subAddr;
    int forwardTime = NOT_SET;
      
    // parse number
    if (p.parseComma(true))
    {
      number = p.parseString();
      p.parseComma();
      unsigned int numberType = p.parseInt();
      if (numberType == InternationalNumberFormat) number = "+" + number;

      // parse subaddr
      if (p.parseComma(true))
      {
        // FIXME subaddr type not handled
        subAddr = p.parseString(true);
        p.parseComma();
        p.parseInt(true);
          
        // parse forwardTime
        if (p.parseComma(true))
        {
          forwardTime = p.parseInt();
        }
      }
    }
    switch (cl)
    {
    case VoiceFacility:
      voice._active = (status == 1);
      voice._cl = VoiceFacility;
      voice._number = number;
      voice._subAddr = subAddr;
      voice._time = forwardTime;
      voice._reason = reason;
      break;
    case DataFacility:
      data._active = (status == 1);
      data._cl = DataFacility;
      data._number = number;
      data._subAddr = subAddr;
      data._time = forwardTime;
      data._reason =  reason;
      break;
    case FaxFacility:
      fax._active = (status == 1);
      fax._cl = FaxFacility;
      fax._number = number;
      fax._subAddr = subAddr;
      fax._time = forwardTime;
      fax._reason = reason;
      break;
    }
  }
}

int MeTa::getBatteryChargeStatus() throw(GsmException)
{
  Parser p(_at->chat("+CBC", "+CBC:"));
  return p.parseInt();
}

int MeTa::getBatteryCharge() throw(GsmException)
{
  Parser p(_at->chat("+CBC", "+CBC:"));
  p.parseInt();
  p.parseComma();
  return p.parseInt();
}

int MeTa::getSignalStrength() throw(GsmException)
{
  Parser p(_at->chat("+CSQ", "+CSQ:"));
  return p.parseInt();
}

int MeTa::getBitErrorRate() throw(GsmException)
{
  Parser p(_at->chat("+CSQ", "+CSQ:"));
  p.parseInt();
  p.parseComma();
  return p.parseInt();
}

vector<string> MeTa::getPhoneBookStrings() throw(GsmException)
{
  Parser p(_at->chat("+CPBS=?", "+CPBS:"));
  return p.parseStringList();
}

PhonebookRef MeTa::getPhonebook(string phonebookString,
                                bool preload) throw(GsmException)
{
  for (PhonebookVector::iterator i = _phonebookCache.begin();
       i !=  _phonebookCache.end(); ++i)
  {
    if ((*i)->name() == phonebookString)
      return *i;
  }
  PhonebookRef newPb(new Phonebook(phonebookString, _at, *this, preload));
  _phonebookCache.push_back(newPb);
  return newPb;
}

string MeTa::getServiceCentreAddress() throw(GsmException)
{
  Parser p(_at->chat("+CSCA?", "+CSCA:"));
  return p.parseString();
}

void MeTa::setServiceCentreAddress(string sca) throw(GsmException)
{
  int type;
  sca = removeWhiteSpace(sca);
  if (sca.length() > 0 && sca[0] == '+')
  {
    type = InternationalNumberFormat;
    sca = sca.substr(1, sca.length() - 1);
  }
  else
    type = UnknownNumberFormat;
  Parser p(_at->chat("+CSCA=\"" + sca + "\"," + intToStr(type)));
}

vector<string> MeTa::getSMSStoreNames() throw(GsmException)
{
  Parser p(_at->chat("+CPMS=?", "+CPMS:"));
  // only return <mem1> values
  return p.parseStringList();
}

SMSStoreRef MeTa::getSMSStore(string storeName) throw(GsmException)
{
  for (SMSStoreVector::iterator i = _smsStoreCache.begin();
       i !=  _smsStoreCache.end(); ++i)
  {
    if ((*i)->name() == storeName)
      return *i;
  }
  SMSStoreRef newSs(new SMSStore(storeName, _at, *this));
  _smsStoreCache.push_back(newSs);
  return newSs;
}

void MeTa::sendSMS(SMSMessageRef smsMessage) throw(GsmException)
{
  smsMessage->setAt(_at);
  smsMessage->send();
}

void MeTa::setMessageService(int serviceLevel) throw(GsmException)
{
  string s;
  switch (serviceLevel)
  {
  case 0:
    s = "0";
    break;
  case 1:
    s = "1";
    break;
  default:
    throw GsmException(_("only serviceLevel 0 or 1 supported"),
                       ParameterError);
  }
  // some devices (eg. Origo 900) don't support service level setting
  _at->chat("+CSMS=" + s, "+CSMS:", true);
}

unsigned int MeTa::getMessageService() throw(GsmException)
{
  Parser p(_at->chat("+CSMS?", "+CSMS:"));
  return p.parseInt();
}

void MeTa::getSMSRoutingToTA(bool &smsRouted,
                             bool &cbsRouted,
                             bool &statusReportsRouted) throw(GsmException)
{
  Parser p(_at->chat("+CNMI?", "+CNMI:"));
  p.parseInt();
  int smsMode = 0;
  int cbsMode = 0;
  int statMode = 0;
  int bufferMode = 0;

  if (p.parseComma(true))
  {
    smsMode = p.parseInt();
    if (p.parseComma(true))
    {
      cbsMode = p.parseInt();
      if (p.parseComma(true))
      {
        statMode = p.parseInt();
        if (p.parseComma(true))
        {
          bufferMode = p.parseInt();
        }
      }
    }
  }
  
  smsRouted = (smsMode == 2) || (smsMode == 3);
  cbsRouted = (cbsMode == 2) || (cbsMode == 3);
  statusReportsRouted = (statMode == 1);
}

void MeTa::setSMSRoutingToTA(bool enableSMS, bool enableCBS,
                             bool enableStatReport,
                             bool onlyReceptionIndication)
  throw(GsmException)
{
  bool smsModesSet = false;
  bool cbsModesSet = false;
  bool statModesSet = false;
  bool bufferModesSet = false;

  // find out capabilities
  Parser p(_at->chat("+CNMI=?", "+CNMI:"));
  vector<bool> modes = p.parseIntList();
  vector<bool> smsModes(1);
  vector<bool> cbsModes(1);
  vector<bool> statModes(1);
  vector<bool> bufferModes(1);
  if (p.parseComma(true))
  {
    smsModes = p.parseIntList();
    smsModesSet = true;
    if (p.parseComma(true))
    {
      cbsModes = p.parseIntList();
      cbsModesSet = true;
      if (p.parseComma(true))
      {
        statModes = p.parseIntList();
        statModesSet = true;
        if (p.parseComma(true))
        {
          bufferModes = p.parseIntList();
          bufferModesSet = true;
        }
      }
    }
  }

  // now set the mode vectors to the default if not set
  if (! smsModesSet) smsModes[0] = true;
  if (! cbsModesSet) cbsModes[0] = true;
  if (! statModesSet) statModes[0] = true;
  if (! bufferModesSet) bufferModes[0] = true;
  
  string chatString;
    
  // now try to set some optimal combination depending on
  // ME/TA's capabilities

  // handle modes
  if (isSet(modes, 2))
    chatString = "2";
  else if (isSet(modes, 1))
    chatString = "1";
  else if (isSet(modes, 0))
    chatString = "0";
  else if (isSet(modes, 3))
    chatString = "3";

  if (onlyReceptionIndication)
  {
    // handle sms mode
    if (enableSMS)
    {
      if (isSet(smsModes, 1))
        chatString += ",1";
      else 
        throw GsmException(_("cannot route SMS messages to TE"),
                           MeTaCapabilityError);
    }
    else
      chatString += ",0";
      
    // handle cbs mode
    if (enableCBS)
    {
      if (isSet(cbsModes, 1))
        chatString += ",1";
      else if (isSet(cbsModes, 2))
        chatString += ",2";
      else 
        throw GsmException(_("cannot route cell broadcast messages to TE"),
                           MeTaCapabilityError);
    }
    else
      chatString += ",0";

    // handle stat mode
    if (enableStatReport)
    {
      if (isSet(statModes, 2))
        chatString += ",2";
      else 
        throw GsmException(_("cannot route status reports messages to TE"),
                           MeTaCapabilityError);
    }
    else
      chatString += ",0";
  }
  else
  {
    // handle sms mode
    if (enableSMS)
    {
      if (isSet(smsModes, 2))
        chatString += ",2";
      else if (isSet(smsModes, 3))
        chatString += ",3";
      else 
        throw GsmException(_("cannot route SMS messages to TE"),
                           MeTaCapabilityError);
    }
    else
      chatString += ",0";
      
    // handle cbs mode
    if (enableCBS)
    {
      if (isSet(cbsModes, 2))
        chatString += ",2";
      else if (isSet(cbsModes, 3))
        chatString += ",3";
      else 
        throw GsmException(_("cannot route cell broadcast messages to TE"),
                           MeTaCapabilityError);
    }
    else
      chatString += ",0";

    // handle stat mode
    if (enableStatReport)
    {
      if (isSet(statModes, 1))
        chatString += ",1";
      else if (isSet(statModes, 2))
        chatString += ",2";
      else 
        throw GsmException(_("cannot route status report messages to TE"),
                           MeTaCapabilityError);
    }
    else
      chatString += ",0";
  }

  // handle buffer mode but only if it was reported by the +CNMI=? command
  // the Ericsson GM12 GSM modem does not like it otherwise
  if (bufferModesSet)
    if (isSet(bufferModes, 1))
      chatString += ",1";
    else
      chatString += ",0";

  _at->chat("+CNMI=" + chatString);
}

