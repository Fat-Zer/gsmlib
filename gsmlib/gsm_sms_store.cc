// *************************************************************************
// * GSM TA/ME library
// *
// * File:    gsm_sms_store.cc
// *
// * Purpose: SMS functions, SMS store
// *          (ETSI GSM 07.05)
// *
// * Author:  Peter Hofmann (software@pxh.de)
// *
// * Created: 20.5.1999
// *************************************************************************

#ifdef HAVE_CONFIG_H
#include <gsm_config.h>
#endif
#include <gsmlib/gsm_sms_store.h>
#include <gsmlib/gsm_parser.h>
#include <gsmlib/gsm_me_ta.h>
#include <iostream>

using namespace std;
using namespace gsmlib;

// SMSStoreEntry members

SMSMessageRef SMSStoreEntry::message() const throw(GsmException)
{
  if (!_cached)
  {
    assert(_mySMSStore != NULL);
    // these operations are at least "logically const"
    SMSStoreEntry *thisEntry = const_cast<SMSStoreEntry*>(this);
    _mySMSStore->readEntry(_index, thisEntry->_message, thisEntry->_status);
    thisEntry->_cached = true;
  }
  return _message;
}

SMSStoreEntry::SMSMemoryStatus SMSStoreEntry::status() const
  throw(GsmException)
{
  if (!_cached)
  {
    assert(_mySMSStore != NULL);
    // these operations are at least "logically const"
    SMSStoreEntry *thisEntry = const_cast<SMSStoreEntry*>(this);
    _mySMSStore->readEntry(_index, thisEntry->_message, thisEntry->_status);
    thisEntry->_cached = true;
  }
  return _status;
}

bool SMSStoreEntry::empty() const throw(GsmException)
{
  return message().isnull();
}

unsigned char SMSStoreEntry::send(Ref<SMSMessage> &ackPdu)
  throw(GsmException)
{
  return _mySMSStore->send(_index, ackPdu);
}

unsigned char SMSStoreEntry::send() throw(GsmException)
{
  SMSMessageRef mref;
  return send(mref);
}

bool SMSStoreEntry::operator==(const SMSStoreEntry &e) const
{
  if (_message.isnull() || e._message.isnull())
    return _message.isnull() && e._message.isnull();
  else
    return _message->encode() == e._message->encode();
}

// SMSStore members

void SMSStore::readEntry(int index, SMSMessageRef &message,
                         SMSStoreEntry::SMSMemoryStatus &status)
  throw(GsmException)
{
  // select SMS store
  _meTa.setSMSStore(_storeName);

#ifndef NDEBUG
  if (debugLevel() >= 1)
    cerr << "*** Reading SMS entry " << index << endl;
#endif NDEBUG

  string pdu;
  Parser p(_at->chat("+CMGR=" + intToStr(index + 1), "+CMGR:", pdu, true));
 
  if (pdu.length() == 0)
  {
    message = SMSMessageRef();
    status = SMSStoreEntry::Unknown;
  }
  else
  {
    status = (SMSStoreEntry::SMSMemoryStatus)p.parseInt();
    // ignore the rest of the line
    message = SMSMessageRef(
      SMSMessage::decode(pdu,
                         !(status == SMSStoreEntry::StoredUnsent ||
                           status == SMSStoreEntry::StoredSent)));
  }
}

void SMSStore::writeEntry(int &index, SMSMessageRef message)
  throw(GsmException)
{
  // select SMS store
  _meTa.setSMSStore(_storeName);

#ifndef NDEBUG
  if (debugLevel() >= 1)
    cerr << "*** Writing SMS entry " << index << endl;
#endif
  
  // compute length of pdu
  string pdu = message->encode();

  // set message status to "RECEIVED READ" for SMS_DELIVER, SMS_STATUS_REPORT
  string statusString;
  if (message->messageType() != SMSMessage::SMS_SUBMIT)
    statusString = ",1";

  Parser p(_at->sendPdu("+CMGW=" +
                        intToStr(pdu.length() / 2 -
                                 message->getSCAddressLen()) + statusString,
                        "+CMGW:", pdu));
  index = p.parseInt() - 1;
}

void SMSStore::eraseEntry(int index) throw(GsmException)
{
  // Select SMS store
  _meTa.setSMSStore(_storeName);

#ifndef NDEBUG
  if (debugLevel() >= 1)
    cerr << "*** Erasing SMS entry " << index << endl;
#endif
  
  _at->chat("+CMGD=" + intToStr(index + 1));
}

unsigned char SMSStore::send(int index, Ref<SMSMessage> &ackPdu)
 throw(GsmException)
{
  Parser p(_at->chat("+CMSS=" + intToStr(index + 1), "+CMSS:"));
  unsigned char messageReference = p.parseInt();

  if (p.parseComma(true))
  {
    string pdu = p.parseEol();

    // add missing service centre address if required by ME
    if (! _at->getMeTa().getCapabilities()._hasSMSSCAprefix)
      pdu = "00" + pdu;

    ackPdu = SMSMessage::decode(pdu);
  }
  else
    ackPdu = SMSMessageRef();

  return messageReference;
}

int SMSStore::doInsert(SMSMessageRef message)
  throw(GsmException)
{
  int index;
  writeEntry(index, message);
  // it is safer to force reading back the SMS from the ME
  _store[index]._cached = false;
  return index;
}

SMSStore::SMSStore(string storeName, Ref<GsmAt> at, MeTa &meTa)
  throw(GsmException) :
  _storeName(storeName), _at(at), _meTa(meTa)
{
  // select SMS store
  Parser p(_meTa.setSMSStore(_storeName, true));
  
  p.parseInt();                 // skip number of used mems
  p.parseComma();
  _capacity = p.parseInt();     // ignore rest of line

  // initialize store entries
  _store = new SMSStoreEntry[_capacity];
  for (int i = 0; i < _capacity; i++)
  {
    _store[i]._index = i;
    _store[i]._cached = false;
    _store[i]._mySMSStore = this;
  }
}

SMSStore::iterator SMSStore::begin()
{
  return &_store[0];
}

SMSStore::const_iterator SMSStore::begin() const
{
  return &_store[0];
}

SMSStore::iterator SMSStore::end()
{
  return &_store[_capacity];
}

SMSStore::const_iterator SMSStore::end() const
{
  return &_store[_capacity];
}

SMSStore::reference SMSStore::operator[](int n)
{
  return _store[n];
}

SMSStore::const_reference SMSStore::operator[](int n) const
{
  return _store[n];
}

SMSStore::reference SMSStore::front()
{
  return _store[0];
}

SMSStore::const_reference SMSStore::front() const
{
  return _store[0];
}

SMSStore::reference SMSStore::back()
{
  return _store[_capacity - 1];
}

SMSStore::const_reference SMSStore::back() const
{
  return _store[_capacity - 1];
}

int SMSStore::size() const throw(GsmException)
{
  // select SMS store
  Parser p(_meTa.setSMSStore(_storeName, true));
  
  return p.parseInt();  
}

SMSStore::iterator SMSStore::insert(iterator position,
                                    const SMSStoreEntry& x)
  throw(GsmException)
{
  int index = doInsert(x.message());
  return &_store[index];
}

void SMSStore::insert (iterator pos, int n, const SMSStoreEntry& x)
  throw(GsmException)
{
  for (int i = 0; i < n; i++)
    doInsert(x.message());
}

void SMSStore::insert (iterator pos, long n, const SMSStoreEntry& x)
  throw(GsmException)
{
  for (long i = 0; i < n; i++)
    doInsert(x.message());
}

SMSStore::iterator SMSStore::erase(iterator position)
  throw(GsmException)
{
  eraseEntry(position->_index);
  position->_cached = false;
  return position;
}

SMSStore::iterator SMSStore::erase(iterator first, iterator last)
  throw(GsmException)
{
  iterator i;
  for (i = first; i != last; i++)
    erase(i);
  return i;
}

void SMSStore::clear() throw(GsmException)
{
  for (iterator i = begin(); i != end(); i++)
    erase(i);
}

SMSStore::~SMSStore()
{
  delete []_store;
}

