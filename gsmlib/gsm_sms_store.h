// *************************************************************************
// * GSM TA/ME library
// *
// * File:    gsm_sms_store.h
// *
// * Purpose: SMS functions, SMS store
// *          (ETSI GSM 07.05)
// *
// * Author:  Peter Hofmann (software@pxh.de)
// *
// * Created: 20.5.1999
// *************************************************************************

#ifndef GSM_SMS_STORE_H
#define GSM_SMS_STORE_H

#include <string>
#include <iterator>
#include <gsmlib/gsm_at.h>
#include <gsmlib/gsm_util.h>
#include <gsmlib/gsm_sms.h>

using namespace std;

namespace gsmlib
{
  // forward declarations
  class SMSStore;
  class MeTa;

  // a single entry in the SMS store

  class SMSStoreEntry
  {
  public:
    // status in ME memory
    enum SMSMemoryStatus {ReceivedUnread = 0, ReceivedRead = 1,
                          StoredUnsent = 2, StoredSent = 3,
                          All = 4, Unknown = 5};

  private:
    // this constructor is only used by SMSStore
    SMSStoreEntry() {}
    SMSMessageRef _message;
    SMSMemoryStatus _status;
    bool _cached;
    SMSStore *_mySMSStore;
    int _index;

  public:
    // create new entry given a SMS message
    SMSStoreEntry(SMSMessageRef message) :
      _message(message), _status(Unknown), _cached(true), _mySMSStore(NULL),
      _index(0) {}

    // create new entry given a SMS message and an index
    // only to be used for file-based stores (see gsm_sorted_sms_store)
    SMSStoreEntry(SMSMessageRef message, int index) :
      _message(message), _status(Unknown), _cached(true), _mySMSStore(NULL),
      _index(index) {}
    
    // return SMS message stored in the entry
    SMSMessageRef message() const throw(GsmException);

    // return message status in store
    SMSMemoryStatus status() const throw(GsmException);

    // return true if empty, ie. no SMS in this entry
    bool empty() const throw(GsmException);

    // send this PDU from store
    // returns message reference and ACK-PDU (if requested)
    // only applicate to SMS-SUBMIT and SMS-COMMAND
    unsigned char send(Ref<SMSMessage> &ackPdu) throw(GsmException);
    
    // same as above, but ACK-PDU is discarded
    unsigned char send() throw(GsmException);

    // return index (guaranteed to be unique,
    // can be used for identification in store)
    int index() const {return _index;}

    // equality operator
    bool operator==(const SMSStoreEntry &e) const;

    friend class SMSStore;
  };

  // this class corresponds to a SMS store in the ME
  // all functions directly update storage in the ME
  // if the ME is exchanged, the storage may become corrupted because
  // of internal buffering in the SMSStore class

  class SMSStore : public RefBase, public NoCopy
  {
  private:
    SMSStoreEntry *_store;      // array of size _capacity of entries
    int _capacity;              // maximum size of phonebook
    string _storeName;          // name of the store, 2-byte like "SM"
    Ref<GsmAt> _at;             // my GsmAt class
    MeTa &_meTa;                // my MeTa class

    // internal access functions
    // read/write entry from/to ME
    void readEntry(int index, SMSMessageRef &message,
                   SMSStoreEntry::SMSMemoryStatus &status) throw(GsmException);
    void writeEntry(int &index, SMSMessageRef message)
      throw(GsmException);
    // erase entry
    void eraseEntry(int index) throw(GsmException);
    // send PDU index from store
    // returns message reference and ACK-PDU (if requested)
    // only applicate to SMS-SUBMIT and SMS-COMMAND
    unsigned char send(int index, Ref<SMSMessage> &ackPdu) throw(GsmException);
    

    // do the actual insertion, return index of new element
    int doInsert(SMSMessageRef message) throw(GsmException);

    // used by class MeTa
    SMSStore(string storeName, Ref<GsmAt> at, MeTa &meTa) throw(GsmException);

  public:
    // iterator defs
    typedef SMSStoreEntry *iterator;
    typedef const SMSStoreEntry *const_iterator;
    typedef SMSStoreEntry &reference;
    typedef const SMSStoreEntry &const_reference;

    // return name of this store (2-character string)
    string name() const {return _storeName;}

    // SMS store traversal commands
    // these are suitable to use stdc++ lib algorithms and iterators
    // ME have fixed storage space implemented as memory slots
    // that may either be empty or used
    
    // traversal commands
    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;
    reference front();
    const_reference front() const;
    reference back();
    const_reference back() const;
    reference operator[](int n);
    const_reference operator[](int n) const;

    // the size macros return the number of used entries
    int size() const throw(GsmException);
    int max_size() const {return _capacity;}
    int capacity() const {return _capacity;}
    bool empty() const throw(GsmException) {return size() == 0;}

    // insert iterators insert into the first empty cell regardless of position
    // existing iterators may be invalidated after an insert operation
    // return position
    // insert only writes to available positions
    // warning: insert fails silently if size() == max_size()
    iterator insert(iterator position, const SMSStoreEntry& x)
      throw(GsmException);

    // insert n times, same procedure as above
    void insert (iterator pos, int n, const SMSStoreEntry& x)
      throw(GsmException);
    void insert (iterator pos, long n, const SMSStoreEntry& x)
      throw(GsmException);

    // erase operators set used slots to "empty"
    iterator erase(iterator position) throw(GsmException);
    iterator erase(iterator first, iterator last) throw(GsmException);
    void clear() throw(GsmException);

    // destructor
    ~SMSStore();

    friend class SMSStoreEntry;
    friend class MeTa;
  };

  typedef Ref<SMSStore> SMSStoreRef;
};

#endif // GSM_SMS_STORE_H
