// *************************************************************************
// * GSM TA/ME library
// *
// * File:    gsmsmsd.cc
// *
// * Purpose: SMS receiver daemon
// *
// * Author:  Peter Hofmann (software@pxh.de)
// *
// * Created: 5.6.1999
// *************************************************************************

#ifdef HAVE_CONFIG_H
#include <gsm_config.h>
#endif
#include <gsmlib/gsm_nls.h>
#include <string>
#include <unistd.h>
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <fstream>
#include <iostream>
#include <gsmlib/gsm_me_ta.h>
#include <gsmlib/gsm_event.h>
#include <gsmlib/gsm_unix_serial.h>

using namespace std;
using namespace gsmlib;

#ifdef HAVE_GETOPT_LONG
static struct option longOpts[] =
{
  {"requeststat", no_argument, (int*)NULL, 'r'},
  {"direct", no_argument, (int*)NULL, 'D'},
  {"xonxoff", no_argument, (int*)NULL, 'X'},
  {"init", required_argument, (int*)NULL, 'I'},
  {"store", required_argument, (int*)NULL, 't'},
  {"device", required_argument, (int*)NULL, 'd'},
  {"spool", required_argument, (int*)NULL, 's'},
  {"sca", required_argument, (int*)NULL, 'C'},
  {"flush", no_argument, (int*)NULL, 'f'},
  {"action", required_argument, (int*)NULL, 'a'},
  {"baudrate", required_argument, (int*)NULL, 'b'},
  {"help", no_argument, (int*)NULL, 'h'},
  {"version", no_argument, (int*)NULL, 'v'},
  {(char*)NULL, 0, (int*)NULL, 0}
};
#else
#define getopt_long(argc, argv, options, longopts, indexptr) \
  getopt(argc, argv, options)
#endif

// my ME

static MeTa *me;
string receiveStoreName;        // store name for received SMSs

// service centre address (set on command line)

static string serviceCentreAddress;

// signal handler for terminate signal

bool terminateSent = false;

void terminateHandler(int signum)
{
  terminateSent = true;
}

// local class to handle SMS events

struct IncomingMessage
{
  // used if new message is put into store
  int _index;                   // -1 means message want send directly
  string _storeName;
  // used if SMS message was sent directly to TA
  SMSMessageRef _newSMSMessage;
  // used if CB message was sent directly to TA
  CBMessageRef _newCBMessage;
  // used in both cases
  GsmEvent::SMSMessageType _messageType;

  IncomingMessage() : _index(-1) {}
};

vector<IncomingMessage> newMessages;

class EventHandler : public GsmEvent
{
public:
  // inherited from GsmEvent
  void SMSReception(SMSMessageRef newMessage,
                    SMSMessageType messageType);
  void CBReception(CBMessageRef newMessage);
  void SMSReceptionIndication(string storeName, unsigned int index,
                              SMSMessageType messageType);

  virtual ~EventHandler() {}
};

void EventHandler::SMSReception(SMSMessageRef newMessage,
                                SMSMessageType messageType)
{
  IncomingMessage m;
  m._messageType = messageType;
  m._newSMSMessage = newMessage;
  newMessages.push_back(m);
}

void EventHandler::CBReception(CBMessageRef newMessage)
{
  IncomingMessage m;
  m._messageType = GsmEvent::CellBroadcastSMS;
  m._newCBMessage = newMessage;
  newMessages.push_back(m);
}

void EventHandler::SMSReceptionIndication(string storeName, unsigned int index,
                                          SMSMessageType messageType)
{
  IncomingMessage m;
  m._index = index;

  if (receiveStoreName != "" && ( storeName == "MT" || storeName == "mt"))
    m._storeName = receiveStoreName;
  else
    m._storeName = storeName;

  m._messageType = messageType;
  newMessages.push_back(m);
}

// execute action on string

void doAction(string action, string result)
{
  if (action != "")
  {
    FILE *fd = popen(action.c_str(), "w");
    if (fd == NULL)
      throw GsmException(stringPrintf(_("could not execute '%s'"),
                                      action.c_str()), OSError);
    fputs(result.c_str(), fd);
    if (ferror(fd))
      throw GsmException(stringPrintf(_("error writing to '%s'"),
                                      action.c_str()), OSError);
    pclose(fd);
  }
  else
    // default if no action: output on stdout
    cout << result << endl;
}

// send all SMS messages in spool dir

bool requestStatusReport = false;

void sendSMS(string spoolDir, Ref<GsmAt> at)
{
  if (spoolDir != "")
  {
    // look into spoolDir for any outgoing SMS that should be sent
    DIR *dir = opendir(spoolDir.c_str());
    if (dir == (DIR*)NULL)
      throw GsmException(
        stringPrintf(_("error when calling opendir('%s')"
                       "(errno: %d/%s)"), 
                     spoolDir.c_str(), errno, strerror(errno)),
        OSError);
    struct dirent *entry;
    while ((entry = readdir(dir)) != (struct dirent*)NULL)
      if (strcmp(entry->d_name, ".") != 0 &&
          strcmp(entry->d_name, ".."))
      {
        // read in file
        // the first line is interpreted as the phone number
        // the rest is the message
        string filename = spoolDir + "/" + entry->d_name;
        ifstream ifs(filename.c_str());
        if (! ifs)
          throw GsmException(
            stringPrintf(_("count not open SMS spool file %s"),
                         filename.c_str()), ParameterError);
        char phoneBuf[1001];
        ifs.getline(phoneBuf, 1000);
        string text;
        while (! ifs.eof())
        {
          char c;
          ifs.get(c);
          text += c;
        }
        ifs.close();
        
        // remove trailing newline/linefeed
        while (text[text.length() - 1] == '\n' ||
               text[text.length() - 1] == '\r')
          text = text.substr(0, text.length() - 1);

        // send the message
        string phoneNumber(phoneBuf);
        Ref<SMSSubmitMessage> submitSMS =
          new SMSSubmitMessage(text, phoneNumber);
        // set service centre address in new submit PDU if requested by user
        if (serviceCentreAddress != "")
        {
          Address sca(serviceCentreAddress);
          submitSMS->setServiceCentreAddress(sca);
        }
        submitSMS->setStatusReportRequest(requestStatusReport);
        submitSMS->setAt(at);
        submitSMS->send();
        unlink(filename.c_str());
      }
    closedir(dir);
  }
}

// *** main program

int main(int argc, char *argv[])
{
  try
  {
    string device = "/dev/mobilephone";
    string action;
    string baudrate;
    bool enableSMS = true;
    bool enableCB = true;
    bool enableStat = true;
    bool flushSMS = false;
    bool onlyReceptionIndication = true;
    string spoolDir;
    string initString = DEFAULT_INIT_STRING;
    bool swHandshake = false;

    int opt;
    int dummy;
    while((opt = getopt_long(argc, argv, "C:I:t:fd:a:b:hvs:XDr",
                             longOpts, &dummy)) != -1)
      switch (opt)
      {
      case 'r':
        requestStatusReport = true;
        break;
      case 'D':
        onlyReceptionIndication = false;
        break;
      case 'X':
        swHandshake = true;
        break;
      case 'I':
        initString = optarg;
        break;
      case 't':
        receiveStoreName = optarg;
        break;
      case 'd':
        device = optarg;
        break;
      case 'C':
        serviceCentreAddress = optarg;
        break;
      case 's':
        spoolDir = optarg;
        break;
      case 'f':
        flushSMS = true;
        break;
      case 'a':
        action = optarg;
        break;
      case 'b':
        baudrate = optarg;
        break;
      case 'v':
        cerr << argv[0] << stringPrintf(_(": version %s [compiled %s]"),
                                        VERSION, __DATE__) << endl;
        exit(0);
        break;
      case 'h':
        cerr << argv[0] << _(": [-a action][-b baudrate][-C sca][-d device]"
                             "[-f][-h][-I init string]\n"
                             "  [-s spool dir][-t][-v]{sms_type}")
             << endl << endl
             << _("  -a, --action      the action to execute when an SMS "
                  "arrives\n"
                  "                    (SMS is send to stdin of action)")
             << endl
             << _("  -b, --baudrate    baudrate to use for device "
                  "(default: 38400)")
             << endl
             << _("  -C, --sca         SMS service centre address") << endl
             << _("  -d, --device      sets the device to connect to") << endl
             << _("  -D, --direct      enable direct routing of SMSs") << endl
             << _("  -f, --flush       flush SMS from store") << endl
             << _("  -h, --help        prints this message") << endl
             << _("  -I, --init        device AT init sequence") << endl
             << _("  -r, --requeststat request SMS status report") << endl
             << _("  -s, --spool       spool directory for outgoing SMS")
             << endl
             << _("  -t, --store       name of SMS store to use for flush\n"
                  "                    and/or temporary SMS storage") << endl
             << endl
             << _("  -v, --version     prints version and exits") << endl
             << _("  -X, --xonxoff     switch on software handshake") << endl
             << endl
             << _("  sms_type may be any combination of") << endl << endl
             << _("    sms, no_sms     controls reception of normal SMS")
             << endl
             << _("    cb, no_cb       controls reception of cell broadcast"
                  " messages") << endl
             << _("    stat, no_stat   controls reception of status reports")
             << endl << endl
             << _("  default is \"sms cb stat\"") << endl << endl
             << _("If no action is given, the SMS is printed to stdout")
             << endl << endl;
        exit(0);
        break;
      case '?':
        throw GsmException(_("unknown option"), ParameterError);
        break;
      }
  
    // find out which kind of message to route
    for (int i = optind; i < argc; ++i)
    {
      string s = lowercase(argv[i]);
      if (s == "sms")
        enableSMS = true;
      else if (s == "no_sms")
        enableSMS = false;
      else if (s == "cb")
        enableCB = true;
      else if (s == "no_cb")
        enableCB = false;
      else if (s == "stat")
        enableStat = true;
      else if (s == "no_stat")
        enableStat = false;
    }

    // register signal handler for terminate signal
    struct sigaction terminateAction;
    terminateAction.sa_handler = terminateHandler;
    sigemptyset(&terminateAction.sa_mask);
    terminateAction.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &terminateAction, NULL) != 0 ||
        sigaction(SIGTERM, &terminateAction, NULL) != 0)
      throw GsmException(
        stringPrintf(_("error when calling sigaction() (errno: %d/%s)"), 
                     errno, strerror(errno)),
        OSError);

    // open GSM device
    MeTa m(new UnixSerialPort(device,
                              baudrate == "" ? DEFAULT_BAUD_RATE :
                              baudRateStrToSpeed(baudrate), initString,
                              swHandshake));
    me = &m;

    // if flush option is given get all SMS from store and dispatch them
    if (flushSMS)
    {
      if (receiveStoreName == "")
        throw GsmException(_("store name must be given for flush option"),
                           ParameterError);
      
      SMSStoreRef store = me->getSMSStore(receiveStoreName);
      for (SMSStore::iterator s = store->begin(); s != store->end(); ++s)
        if (! s->empty())
        {
          string result =  _("Type of message: ");
          switch (s->message()->messageType())
          {
          case SMSMessage::SMS_DELIVER:
            result += _("SMS message\n");
            break;
          case SMSMessage::SMS_SUBMIT_REPORT:
            result += _("submit report message\n");
            break;
          case SMSMessage::SMS_STATUS_REPORT:
            result += _("status report message\n");
            break;
          }
          result += s->message()->toString();
          doAction(action, result);
          store->erase(s);
        }
    }

    // set default SMS store if -t option was given or
    // read from ME otherwise
    if (receiveStoreName == "")
    {
      string dummy1, dummy2;
      m.getSMSStore(dummy1, dummy2, receiveStoreName );
    }
    else
      m.setSMSStore(receiveStoreName, 3);

    // switch message service level to 1
    // this enables SMS routing to TA
    m.setMessageService(1);

    // switch on SMS routing
    m.setSMSRoutingToTA(enableSMS, enableCB, enableStat,
                        onlyReceptionIndication);

    // register event handler
    m.setEventHandler(new EventHandler());
    
    // wait for new messages
    bool exitScheduled = false;
    while (1)
    {
      struct timeval timeoutVal;
      timeoutVal.tv_sec = 5;
      timeoutVal.tv_usec = 0;
      m.waitEvent(&timeoutVal);

      // if it returns, there was an event or a timeout
      while (newMessages.size() > 0)
      {
        // get first new message and remove it from the vector
        SMSMessageRef newSMSMessage = newMessages.begin()->_newSMSMessage;
        CBMessageRef newCBMessage = newMessages.begin()->_newCBMessage;
        GsmEvent::SMSMessageType messageType =
          newMessages.begin()->_messageType;
        int index = newMessages.begin()->_index;
        string storeName = newMessages.begin()->_storeName;
        newMessages.erase(newMessages.begin());

        // process the new message
        string result = _("Type of message: ");
        switch (messageType)
        {
        case GsmEvent::NormalSMS:
          result += _("SMS message\n");
          break;
        case GsmEvent::CellBroadcastSMS:
          result += _("cell broadcast message\n");
          break;
        case GsmEvent::StatusReportSMS:
          result += _("status report message\n");
          break;
        }
        if (! newSMSMessage.isnull())
          result += newSMSMessage->toString();
        else if (! newCBMessage.isnull())
          result += newCBMessage->toString();
        else
        {
          SMSStoreRef store = me->getSMSStore(storeName);

          if (messageType == GsmEvent::CellBroadcastSMS)
            result += (*store.getptr())[index].cbMessage()->toString();
          else
            result += (*store.getptr())[index].message()->toString();
            
          store->erase(store->begin() + index);
        }
        
        // call the action
        doAction(action, result);
      }

      // if no new SMS came in and program exit was scheduled, then exit
      if (exitScheduled)
        exit(0);
      
      // handle terminate signal
      if (terminateSent)
      {
        exitScheduled = true;
        // switch off SMS routing
        try
        {
          m.setSMSRoutingToTA(false, false, false);
        }
        catch (GsmException &ge)
        {
          // some phones (e.g. Motorola Timeport 260) don't allow to switch
          // off SMS routing which results in an error. Just ignore this.
        }
        // the AT sequences involved in switching of SMS routing
        // may yield more SMS events, so go round the loop one more time
      }

      // send spooled SMS
      if (! terminateSent)
        sendSMS(spoolDir, m.getAt());
    }
  }
  catch (GsmException &ge)
  {
    cerr << argv[0] << _("[ERROR]: ") << ge.what() << endl;
    if (ge.getErrorClass() == MeTaCapabilityError)
      cerr << argv[0] << _("[ERROR]: ")
           << _("(try setting sms_type, please refer to gsmsmsd manpage)")
           << endl;
    return 1;
  }
  return 0;
}
