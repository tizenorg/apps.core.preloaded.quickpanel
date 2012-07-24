%define PREFIX	"/opt/apps/org.tizen.quickpanel"
%define RESDIR  "/opt/apps/org.tizen.quickpanel/res"
%define DATADIR "/opt/apps/org.tizen.quickpanel/data"

Name:       org.tizen.quickpanel
Summary:    Quick Panel
Version:    0.1.1
Release:    1
Group:      util
License:    Flora Software License
Source0:    %{name}-%{version}.tar.gz
Source101:  quickpanel.service
Source1001: org.tizen.quickpanel.manifest 

BuildRequires: pkgconfig(appcore-efl)
BuildRequires: pkgconfig(appcore-common)
BuildRequires: pkgconfig(heynoti)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(aul)
BuildRequires: pkgconfig(appsvc)
BuildRequires: pkgconfig(svi)
BuildRequires: pkgconfig(libprivilege-control)
BuildRequires: pkgconfig(ecore-x)
BuildRequires: pkgconfig(edbus)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(edje)
BuildRequires: pkgconfig(mm-sound)
BuildRequires: pkgconfig(iniparser)
BuildRequires: pkgconfig(icu-i18n)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(syspopup-caller)
	
BuildRequires: gettext-tools
BuildRequires: cmake
BuildRequires: edje-tools

Requires(post): /usr/bin/vconftool

%description
Quick Panel

%prep
%setup -q


%build
cp %{SOURCE1001} .
LDFLAGS+="-Wl,--rpath=%{PREFIX}/lib -Wl,--as-needed";export LDFLAGS
cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}/%{_sysconfdir}/rc.d/rc5.d/
mkdir -p %{buildroot}/%{_sysconfdir}/rc.d/rc3.d/
ln -s ../../init.d/quickpanel %{buildroot}/%{_sysconfdir}/rc.d/rc5.d/S51quickpanel
ln -s ../../init.d/quickpanel %{buildroot}/%{_sysconfdir}/rc.d/rc3.d/S51quickpanel

mkdir -p %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants
install -m 0644 %SOURCE101 %{buildroot}%{_libdir}/systemd/user/
ln -s ../quickpanel.service %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants/quickpanel.service

%clean
rm -rf %{buildroot}


%files
%manifest org.tizen.quickpanel.manifest
%attr(775,app,app) /opt/apps/org.tizen.quickpanel/data
%attr(755,-,-) %{_sysconfdir}/init.d/quickpanel
%{_sysconfdir}/rc.d/rc3.d/S51quickpanel
%{_sysconfdir}/rc.d/rc5.d/S51quickpanel
%{_sysconfdir}/init.d/quickpanel
%{_libdir}/systemd/user/quickpanel.service
%{_libdir}/systemd/user/core-efl.target.wants/quickpanel.service
/opt/apps/org.tizen.quickpanel/bin/*
/opt/apps/org.tizen.quickpanel/res/*
/opt/share/applications/org.tizen.quickpanel.desktop
