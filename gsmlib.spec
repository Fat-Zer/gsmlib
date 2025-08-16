%define LIBVER 1.0.5
Summary: Library to access GSM mobile phones through GSM modems
Name: gsmlib
Version: 1.11
Release: 1
Source: gsmlib-%{version}.tar.gz
Group: System Environment/Libraries
Copyright: GNU LIBRARY GENERAL PUBLIC LICENSE
URL: http://www.pxh.de/fs/gsmlib/
Vendor: Peter Hofmann <software@pxh.de>
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root

%package devel
Summary: Development tools for programs which will use the gsmlib library.
Group: Development/Libraries
Requires: gsmlib

%package ext
Summary: Extensions to gsmlib to support non-standard phone features.
Group:  Development/Libraries
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

%description ext
The extension package of gsmlib contains programs, libraries, and
documentation to support non-standard features of GSM phones. The
following phones/phone types are currently supported:
 * Siemens GSM phones

%prep
%setup

%build
%configure
make

%install
make DESTDIR="$RPM_BUILD_ROOT" install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_libdir}/libgsmme.so
%{_libdir}/libgsmme.so.%{LIBVER}
%{_bindir}/gsmsmsstore
%{_bindir}/gsmctl
%{_bindir}/gsmsmsd
%{_bindir}/gsmpb
%{_bindir}/gsmsendsms
%{_mandir}/man1/gsmctl.1.gz
%{_mandir}/man7/gsminfo.7.gz
%{_mandir}/man1/gsmpb.1.gz
%{_mandir}/man1/gsmsendsms.1.gz
%{_mandir}/man8/gsmsmsd.8.gz
%{_mandir}/man1/gsmsmsstore.1.gz
%{_datadir}/locale/de/LC_MESSAGES/gsmlib.mo

%doc README INSTALL ABOUT-NLS AUTHORS COPYING NEWS TODO
%doc doc/README.NLS doc/README.developers doc/FAQ ChangeLog

%files devel
%defattr(-,root,root)
%{_libdir}/libgsmme.a
%{_libdir}/libgsmme.la
%{_libdir}/libgsmext.a
%{_libdir}/libgsmext.la
%{_includedir}/gsmlib

%files ext
%{_bindir}/gsmsiectl
%{_bindir}/gsmsiexfer
%{_libdir}/libgsmext.so
%{_libdir}/libgsmext.so.%{LIBVER}

%doc ext/README.sieme
