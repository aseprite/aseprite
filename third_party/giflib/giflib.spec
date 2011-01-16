Summary: A library for manipulating GIF format image files.
Name: giflib
Version: 4.1.3
Release: 1
License: MIT
URL: http://sourceforge.net/projects/libungif/
Source: http://osdn.dl.sourceforge.net/sourceforge/libungif/%{name}-%{version}.tar.bz2
Group: System Environment/Libraries
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
Obsoletes: libungif
Provides: libungif.so libungif

%description
GIF loading and saving shared library.

IMPORTANT!  Please read /usr/doc/PATENT_PROBLEMS for information on
why using this library may put you at legal risk and what
alternatives you may have.

%package devel
Summary: Development tools for programs which will use the giflib library.
Group: Development/Libraries
Requires: %{name} = %{version}

%description devel
Libraries and headers needed for developing programs that use giflib
to load and save gif image files.

IMPORTANT!  Please read /usr/doc/PATENT_PROBLEMS for information on
why using this library may put you at legal risk and what
alternatives you may have.

%package progs
Summary: Programs for manipulating GIF format image files.
Group: Applications/Multimedia
Requires: %{name} = %{version}

%description progs
This package contains various programs for manipulating gif image files.

IMPORTANT!  Please read /usr/doc/PATENT_PROBLEMS for information on
why using this library may put you at legal risk and what
alternatives you may have.

%prep
%setup

%build
%configure
make all

%install
rm -rf ${RPM_BUILD_ROOT}

%{makeinstall}
ln -sf libungif.so.%{version} ${RPM_BUILD_ROOT}%{_libdir}/libgif.so.%{version}
ln -sf libgif.so.%{version} ${RPM_BUILD_ROOT}%{_libdir}/libgif.so.4
ln -sf libgif.so.4 ${RPM_BUILD_ROOT}%{_libdir}/libgif.so
ln -sf libungif.a ${RPM_BUILD_ROOT}%{_libdir}/libgif.a

chmod 755 ${RPM_BUILD_ROOT}%{_libdir}/*.so*
chmod 644 ${RPM_BUILD_ROOT}%{_libdir}/*.a*
chmod 644 COPYING README UNCOMPRESSED_GIF NEWS ONEWS
chmod 644 ChangeLog TODO BUGS AUTHORS
chmod 644 doc/* util/giffiltr.c util/gifspnge.c

%clean
rm -rf ${RPM_BUILD_ROOT}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc COPYING README PATENT_PROBLEMS NEWS ONEWS
%doc ChangeLog TODO BUGS AUTHORS
%{_libdir}/lib*.so.*

%files devel
%defattr(-,root,root)
%doc doc/* util/giffiltr.c util/gifspnge.c
%{_libdir}/lib*.a
%{_libdir}/lib*.so
%{_libdir}/lib*.la
%{_includedir}/gif_lib.h

%files progs
%attr(0755 root root) %{_bindir}/*

%changelog
* Sat May 29 2004 Toshio Kuratomi <toshio@tiki-lounge.com> - 4.1.3-1
- Upgrade to version 4.1.3

* Wed May 26 2004 Toshio Kuratomi <toshio@tiki-lounge.com> - 4.1.2-1
- Upgrade to 4.1.2

* Tue Feb 17 2004 Toshio Kuratomi <toshio@tiki-lounge.com> - 4.1.1-1
- Upgrade to 4.1.1
- Modernize the spec a bit

* Tue Jan 19 1999 Toshio Kuratomi <badger@prtr-13.ucsc.edu>
- Upgrade to version 4.1
  + Fix a few minor memory leaks in error conditions.
  + Add a new function EGifOpen( void *userData, OutputFunc writeFunc) that
    allows user specified gif writing function.
- Merge spec file from libungif-3.1-5

* Fri Dec 17 1998 Toshio Kuratomi <badger@prtr-13.ucsc.edu>
- Add note to read PATENT_PROBLEMS.

* Mon Dec 14 1998 Toshio Kuratomi <badger@prtr-13.ucsc.edu>
- Upgrade to version 4.0 (Fixes rather nasty behaviour when dealing with
  Extensions.)
