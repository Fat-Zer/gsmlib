// *************************************************************************
// * GSM TA/ME library
// *
// * File:    gsmsendsms.cc
// *
// * Purpose: GSM sms send program
// *
// * Author:  Peter Hofmann (software@pxh.de)
// *
// * Created: 16.7.1999
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
#include <stdlib.h>
#include <errno.h>
#include <gsmlib/gsm_me_ta.h>
#include <gsmlib/gsm_util.h>
#include <gsmlib/gsm_unix_serial.h>

using namespace std;
using namespace gsmlib;

// options

#ifdef HAVE_GETOPT_LONG
static struct option longOpts[] =
{
  {"xonxoff", no_argument, (int*)NULL, 'X'},
  {"sca", required_argument, (int*)NULL, 'C'},
  {"device", required_argument, (int*)NULL, 'd'},
  {"init", required_argument, (int*)NULL, 'I'},
  {"baudrate", required_argument, (int*)NULL, 'b'},
  {"test", no_argument, (int*)NULL, 't'},
  {"help", no_argument, (int*)NULL, 'h'},
  {"version", no_argument, (int*)NULL, 'v'},
  {(char*)NULL, 0, (int*)NULL, 0}
};
#else
#define getopt_long(argc, argv, options, longopts, indexptr) \
  getopt(argc, argv, options)
#endif

// convert /r and /n to CR and LF

static string unescapeString(char *line)
{
  string result;
  bool escaped = false;
  int index = 0;

  while (line[index] != 0 &&
         line[index] != CR && line[index] != LF)
  {
    if (escaped)
    {
      escaped = false;
      if (line[index] == 'r')
        result += CR;
      else if (line[index] == 'n')
        result += LF;
      else if (line[index] == '\\')
        result += '\\';
      else
        result += line[index];
    }
    else
      if (line[index] == '\\')
        escaped = true;
      else
        result += line[index];

    ++index;
  }
  return result;
}

// *** main program

int main(int argc, char *argv[])
{
  try
  {
    // handle command line options
    string device = "/dev/mobilephone";
    bool test = false;
    string baudrate;
    Ref<GsmAt> at;
    string initString = DEFAULT_INIT_STRING;
    bool swHandshake = false;
    // service centre address (set on command line)
    string serviceCentreAddress;

    int opt;
    int dummy;
    while((opt = getopt_long(argc, argv, "C:I:d:b:thvX", longOpts, &dummy))
          != -1)
      switch (opt)
      {
      case 'C':
        serviceCentreAddress = optarg;
        break;
      case 'X':
        swHandshake = true;
        break;
      case 'I':
        initString = optarg;
        break;
      case 'd':
        device = optarg;
        break;
      case 'b':
        baudrate = optarg;
        break;
      case 't':
        test = true;
        break;
      case 'v':
        cerr << argv[0] << stringPrintf(_(": version %s [compiled %s]"),
                                        VERSION, __DATE__) << endl;
        exit(0);
        break;
      case 'h':
        cerr << argv[0] << _(": [-b baudrate][-C sca][-d device][-h]"
                             "[-I init string]\n"
                             "  [-t][-v][-X] phonenumber [text]") << endl
             << endl
             << _("  -b, --baudrate    baudrate to use for device "
                  "(default: 38400)")
             << endl
             << _("  -C, --sca         SMS service centre address") << endl
             << _("  -d, --device      sets the destination device to connect "
                  "to") << endl
             << _("  -h, --help        prints this message") << endl
             << _("  -I, --init        device AT init sequence") << endl
             << _("  -t, --test        convert text to GSM alphabet and "
                  "vice\n"
                  "                    versa, no SMS message is sent") << endl
             << _("  -v, --version     prints version and exits")
             << endl
             << _("  -X, --xonxoff     switch on software handshake") << endl
             << endl
             << _("  phonenumber       recipient's phone number") << endl
             << _("  text              optional text of the SMS message\n"
                  "                    if omitted: read from stdin")
             << endl << endl;
        exit(0);
        break;
      case '?':
        throw GsmException(_("unknown option"), ParameterError);
        break;
      }

    if (! test)
    {
      // open the port and ME/TA
      Ref<Port> port = new UnixSerialPort(device,
                                          baudrate == "" ? DEFAULT_BAUD_RATE :
                                          baudRateStrToSpeed(baudrate),
                                          initString, swHandshake);
      // switch message service level to 1
      // this enables acknowledgement PDUs
      MeTa m(port);
      m.setMessageService(1);

      at = new GsmAt(m);
    }

    // check parameters
    if (optind == argc)
      throw GsmException(_("phone number and text missing"), ParameterError);

    if (optind + 2 < argc)
      throw GsmException(_("more than two parameters given"), ParameterError);
      
    // get phone number
    string phoneNumber = argv[optind];

    // get text
    string text;
    if (optind + 1 == argc)
    {                           // read from stdin
      char s[1000];
      cin.get(s, 1000);
      text = unescapeString(s);
      if (text.length() > 160)
        throw GsmException(_("text is larger than 160 characters"),
                           ParameterError);
    }
    else
      text = argv[optind + 1];

    if (test)
      cout << gsmToLatin1(latin1ToGsm(text)) << endl;
    else
    {
      // send SMS
      Ref<SMSMessage> submitSMS = new SMSSubmitMessage(text, phoneNumber);
      // set service centre address in new submit PDU if requested by user
      if (serviceCentreAddress != "")
      {
        Address sca(serviceCentreAddress);
        submitSMS->setServiceCentreAddress(sca);
      }
      submitSMS->setAt(at);
      Ref<SMSMessage> ackPDU;
      submitSMS->send(ackPDU);

      // print acknowledgement if available
      if (! ackPDU.isnull())
      cout << ackPDU->toString();
    }
  }
  catch (GsmException &ge)
  {
    cerr << argv[0] << _("[ERROR]: ") << ge.what() << endl;
    return 1;
  }
  return 0;
}
