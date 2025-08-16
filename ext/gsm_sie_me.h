// *************************************************************************
// * GSM TA/ME library
// *
// * File:    gsm_sie_me.h
// *
// * Purpose: Mobile Equipment/Terminal Adapter and SMS functions
// *          (According to "AT command set for S45 Siemens mobile phones"
// *           v1.8, 26. July 2001 - Common AT prefix is "^S")
// *
// * Author:  Christian W. Zuckschwerdt  <zany@triq.net>
// *
// * Created: 2001-12-15
// *************************************************************************

#ifndef GSM_SIE_ME_H
#define GSM_SIE_ME_H

#include <gsmlib/gsm_error.h>
#include <gsmlib/gsm_at.h>
#include <string>
#include <vector>

namespace gsmlib
{
  using std::string;

  // *** Siemens mobile phone binary objects (bitmap, midi, vcal, vcard)

  struct BinaryObject
  {
    string _type;               // Object type
    int _subtype;               // Object subtype (storage number)
    unsigned char *_data;       // Object binary data
    int _size;                  // Object data size
  };

  // *** this class allows extended access to Siemens moblie phones

  class SieMe : public MeTa
  {
  private:
    // init ME/TA to sensible defaults
    void init();

  public:
    // initialize a new MeTa object given the port
    SieMe(Ref<Port> port);


    // get the current phonebook in the Siemens ME
    vector<string> getSupportedPhonebooks();// (AT^SPBS=?)

    // get the current phonebook in the Siemens ME
    string getCurrentPhonebook(); // (AT^SPBS?)

    // set the current phonebook in the Siemens ME
    // remember the last phonebook set for optimisation
    void setPhonebook(string phonebookName); // (AT^SPBS=)


    // Siemens get supported signal tones
    IntRange getSupportedSignalTones(); // (AT^SPST=?)

    // Siemens set ringing tone
    void playSignalTone(int tone); // (AT^SRTC=x,1)

    // Siemens set ringing tone
    void stopSignalTone(int tone); // (AT^SRTC=x,0)


    // Siemens get ringing tone
    IntRange getSupportedRingingTones(); // (AT^SRTC=?)
    // Siemens get ringing tone
    int getCurrentRingingTone(); // (AT^SRTC?)
    // Siemens set ringing tone
    void setRingingTone(int tone, int volume);// (AT^SRTC=)
    // Siemens set ringing tone on
    void playRingingTone();
    // Siemens set ringing tone of
    void stopRingingTone();
    // Siemens toggle ringing tone
    void toggleRingingTone(); // (AT^SRTC)

    // Siemens get supported binary read
    vector<ParameterRange> getSupportedBinaryReads();

    // Siemens get supported binary write
    vector<ParameterRange> getSupportedBinaryWrites();

    // Siemens Binary Read
    BinaryObject getBinary(string type, int subtype);

    // Siemens Binary Write
    void setBinary(string type, int subtype, BinaryObject obj);

    // Siemens Sim Toolkit
    void setSSTK(string pdu);
  };
};

#endif // GSM_ME_TA_H
