%define name            gsmlib
%define release         1
%define version 	1.11
%define libversion 	1.0.5
%define prefix          /usr

Summary: 		Library to access GSM mobile phones through GSM modems
Name: 			%{name} 
Version: 		%{version}
Release: 		%{release} 
Copyright:	 	LGPL
Source0: 		%{name}-%{version}.tar.gz
Group: 			System Environment/Libraries
URL: 			http://www.pxh.de/fs/gsmlib/
Vendor: 		Peter Hofmann <software@pxh.de>
Packager: 		Peter Hofmann <software@pxh.de>
Buildroot: 		%{_tmppath}/%{name}-%{version}-buildroot

%package devel
Summary: 		Development tools for programs which will use the gsmlib library
Group: 			Development/Libraries
Requires: 		gsmlib

%package ext
Summary: 		Extensions to gsmlib to support non-standard phone features
Group: 			Development/Libraries
Requires: 		gsmlib

%description
This distribution contains a library to access GSM mobile phones through
GSM modems. Features include:
 * modification of phonebooks stored in the
   mobile phone or on the SIM card
 * reading and writing of SMS messages stored in
   the mobile phone
 * sending and reception of SMS messages
Additionally, some simple command line programs are provided to use
these functionalities.

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
%configure

%build
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR="$RPM_BUILD_ROOT" install
rm -f $RPM_BUILD_ROOT/usr/lib/libgsmext.la
rm -f $RPM_BUILD_ROOT/usr/lib/libgsmme.la

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/gsmsmsstore
%{_bindir}/gsmctl
%{_bindir}/gsmsmsd
%{_bindir}/gsmpb
%{_bindir}/gsmsendsms
%{_libdir}/libgsmme.so
%{_libdir}/libgsmme.so.%{libversion}
%{_mandir}/man*/*
%{_datadir}/locale/de/LC_MESSAGES/*
%doc README INSTALL ABOUT-NLS AUTHORS COPYING NEWS TODO ChangeLog
%doc doc/README.NLS doc/README.developers doc/FAQ

%files devel
%defattr(-,root,root)
%{_libdir}/libgsmme.a
%{_includedir}/gsmlib

%files ext
%defattr(-,root,root)
%{_bindir}/gsmsiectl
%{_bindir}/gsmsiexfer
%{_libdir}/libgsmext.a
%{_libdir}/libgsmext.so
%{_libdir}/libgsmext.so.%{libversion}
%doc ext/README.sieme
