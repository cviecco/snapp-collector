# This spec is intended to build and install on multiple distributions
# (someday). Detect the distribution we're building on.

%define ostag unknown
 
%define is_rh   %(test -e /etc/redhat-release && echo 1 || echo 0)
%define is_fc   %(test -e /etc/fedora-release && echo 1 || echo 0)

%if %{is_fc}
%define ostag %(sed -e 's/^.*release /fc/' -e 's/ .*$//' -e 's/\\./_/g' < /etc/fedora-release)
%else
%if %{is_rh}
%define ostag %(sed -e 's/^.*release /rh/' -e 's/ .*$//' -e 's/\\./_/g' < /etc/redhat-release)
%endif
%endif

%define release 1.%{ostag}

Summary:  SNAPP SNMP collector 
Name: snapp-collector
Version: 3.0.7
Release: %{release}
License: GPL
Group:   Applications/Network
URL:     http://www.grnoc.iu.edu/releases/snapp/snapp-%{version}.tar.gz 
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: rrdtool   >= 1.3
Requires: pcre      >= 6.6
Requires: libdbi    >= 0.8
Requires: net-snmp  >= 5.3
Requires: libxml2   >= 2.6
Requires: libstdc++ >= 4.1
Requires: libdbi-dbd-mysql
#Requires: perl-TermReadKey
BuildRequires: autoconf, automake, gcc, gcc-c++, rrdtool-devel, pcre-devel, net-snmp-devel, elinks, libxslt
Provides: grnoc-SNAPP-collector %{release}

%description
High-performance SNMP collector

%define bindir    /usr/bin/
%define confdir	  /etc/snapp/
%define etcdir    /etc/

%prep
%setup -n  %{name}-%{version}

%build
%configure --target=%{_target}
%{__make}
cd doc
%{__make} index.html
cd ..

%install
rm -rf %{buildroot}
#make install 	basedir=%{buildroot}
#make install
#rm -rf $RPM_BUILD_ROOT
#mkdir -p $RPM_BUILD_ROOT%{etcdir}/hflow
#mkdir -p $RPM_BUILD_ROOT%{bindir}
#mkdir -p $RPM_BUILD_ROOT%{confdir}/misc

%{__install} -Dp -m0755 src/snapp-collector	            %{buildroot}%{_sbindir}/snapp-collector
#%{__install} -Dp -m0755 socksserver                %{buildroot}%{_bindir}/socksserver
%{__install} -d -m0755 %{buildroot}%{_sysconfdir}/snapp/
#%{__install} -p -m0644  router_list.txt          %{buildroot}%{_sysconfdir}/tdor/

 %__mkdir -p -m 0755 $RPM_BUILD_ROOT%{_mandir}/man8
 %__install -p -m 0644 snapp-collector.8 $RPM_BUILD_ROOT%{_mandir}/man8
 %__gzip $RPM_BUILD_ROOT%{_mandir}/man8/snapp-collector.8

 %__mkdir -p -m 0755 $RPM_BUILD_ROOT%{_defaultdocdir}/snapp-collector-%{version}/doc
# %__install -p -m 0644 doc/docbook_doc.xml $RPM_BUILD_ROOT%{_defaultdocdir}/snapp-collector-%{version}/doc
# %__install -p -m 0644 doc/Makefile $RPM_BUILD_ROOT%{_defaultdocdir}/snapp-collector-%{version}/doc
 %__install -p -m 0644 doc/*.html $RPM_BUILD_ROOT%{_defaultdocdir}/snapp-collector-%{version}/doc


 %__install -p -m 0644 README $RPM_BUILD_ROOT%{_defaultdocdir}/snapp-collector-%{version} 
 %__mkdir -p -m 0755 $RPM_BUILD_ROOT%{_defaultdocdir}/snapp-collector-%{version}/xml
 %__install -p -m 0644 xml/config.rnc $RPM_BUILD_ROOT%{_defaultdocdir}/snapp-collector-%{version}/xml 
 %__install -p -m 0644 xml/example_config.xml $RPM_BUILD_ROOT%{_defaultdocdir}/snapp-collector-%{version}/xml
 %__mkdir -p -m 0755 $RPM_BUILD_ROOT%{_defaultdocdir}/snapp-collector-%{version}/sql
 %__install -p -m 0644 sql/snapp.mysql.sql $RPM_BUILD_ROOT%{_defaultdocdir}/snapp-collector-%{version}/sql
 %__install -p -m 0644 sql/base_example.sql $RPM_BUILD_ROOT%{_defaultdocdir}/snapp-collector-%{version}/sql
 %__install -p -m 0755 snapp_configure.pl  $RPM_BUILD_ROOT%{_defaultdocdir}/snapp-collector-%{version}/

 


%clean
#rm -rf $RPM_BUILD_ROOT
rm -rf %{buildroot}

 
%files
%defattr(-,root,root,0755)
%{_sbindir}/snapp-collector
#%{bindir}/socksserver
#%{_sysconfdir}/hflow/hflowd.schema 
#%{_sysconfdir}/tdor/router_list.txt 
%{_mandir}/man8/snapp-collector.8.gz
%{_defaultdocdir}/snapp-collector-%{version}/README
#%{_defaultdocdir}/snapp-collector-%{version}/doc/docbook_doc.xml
#%{_defaultdocdir}/snapp-collector-%{version}/doc/Makefile
%{_defaultdocdir}/snapp-collector-%{version}/doc/*.html
%{_defaultdocdir}/snapp-collector-%{version}/xml/config.rnc
%{_defaultdocdir}/snapp-collector-%{version}/xml/example_config.xml
%{_defaultdocdir}/snapp-collector-%{version}/sql/snapp.mysql.sql
%{_defaultdocdir}/snapp-collector-%{version}/sql/base_example.sql
%{_defaultdocdir}/snapp-collector-%{version}/snapp_configure.pl

%attr(0755,root,root) %dir %{_sysconfdir}/snapp
#%attr(0755,root,root) %dir %{_sysconfdir}/hflow/misc


%post

if [ $1 -eq 1 ]; then
        #--- install
 
  #add the tdor user
  /usr/sbin/groupadd _snapp 
  /usr/sbin/useradd  -m  -c "SNAPP" -d /var/log/snapp -s /dev/null -g _snapp _snapp 


fi


if [ $1 -ge 2 ]; then
        # do not create anything, snapp users is already there
echo "nothing here"
fi

%postun
if [ $1 = 0 ] ; then
        ##do not delete the snapp user, otherwise ownership
        ##problems will ocur on resintallation.

        #/usr/sbin/userdel _snapp 2>/dev/null
        echo "nothing to delete"
fi



