%define LIBVER 1.0.1
Summary: Library to access GSM mobile phones through GSM modems
Name: gsmlib
Version: 1.7
Release: 1
Source: gsmlib-%{version}.tar.gz
Group: System Environment/Libraries
Copyright: GNU LIBRARY GENERAL PUBLIC LICENSE
URL: http://www.pxh.de/fs/gsmlib/
Vendor: Peter Hofmann <software@pxh.de>
Buildroot: /var/tmp/gsmlib-root

%package devel
Summary: Development tools for programs which will use the gsmlib library.
Group: Development/Libraries
Requires: gsmlib

%description
This distribution contains a library to access
GSM mobile phones through GSM modems. Features include:
 * modification of phonebooks stored in the
   mobile phone or on the SIM card
 * reading and writing of SMS messages stored in
   the mobile phone
 * sending and reception of SMS messages
Additionally, some simple command line programs are
provided to use these functionalities.

%description devel
The gsmlib-devel package includes the header files and static libraries
necessary for developing programs which use the gsmlib library.

%prep
%setup

%build
CXXFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=/usr
make

%install
make DESTDIR="$RPM_BUILD_ROOT" install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/lib/libgsmme.so
/usr/lib/libgsmme.so.%{LIBVER}
/usr/bin/gsmsmsstore
/usr/bin/gsmctl
/usr/bin/gsmsmsd
/usr/bin/gsmpb
/usr/bin/gsmsendsms
/usr/man
/usr/share/locale/de/LC_MESSAGES/gsmlib.mo

%doc README INSTALL ABOUT-NLS AUTHORS COPYING NEWS TODO
%doc doc/README.NLS doc/README.developers doc/FAQ ChangeLog

%files devel
%defattr(-,root,root)
/usr/lib/libgsmme.a
/usr/include/gsmlib


