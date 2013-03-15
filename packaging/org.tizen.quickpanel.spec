%define PREFIX    /usr/apps/%{name}
%define RESDIR    %{PREFIX}/res
%define DATADIR    %{PREFIX}/data

Name:       org.tizen.quickpanel
Summary:    Quick Panel
Version:    0.3.22
Release:    2
Group:      util
License:    Flora Software License
Source0:    %{name}-%{version}.tar.gz
Source101:  quickpanel.service

BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-appfw-app-manager)
BuildRequires: pkgconfig(capi-system-runtime-info)
BuildRequires: pkgconfig(capi-system-info)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(capi-network-tethering)
BuildRequires: pkgconfig(capi-media-player)
BuildRequires: pkgconfig(feedback)
BuildRequires: pkgconfig(appcore-common)
BuildRequires: pkgconfig(heynoti)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(appsvc)
BuildRequires: pkgconfig(devman_haptic)
BuildRequires: pkgconfig(libprivilege-control)
BuildRequires: pkgconfig(ecore-x)
BuildRequires: pkgconfig(edbus)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(edje)
BuildRequires: pkgconfig(icu-i18n)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(minicontrol-viewer)
BuildRequires: pkgconfig(minicontrol-monitor)
BuildRequires: pkgconfig(utilX)
BuildRequires: gettext-tools
BuildRequires: cmake
BuildRequires: edje-tools

Requires(post): /usr/bin/vconftool
Requires: e17
%description
Quick Panel

%prep
%setup -q


%build
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

mkdir -p %{buildroot}/usr/share/license
cp -f LICENSE.Flora %{buildroot}/usr/share/license/%{name}

%post
vconftool set -t bool db/setting/rotate_lock 0 -u 5000
vconftool set -t bool db/setting/drivingmode/drivingmode 0 -u 5000
vconftool set -t bool memory/private/%{name}/started 0 -i -u 5000
vconftool set -t bool memory/private/%{name}/enable_ask 1 -i -u 5000
vconftool set -t bool memory/private/%{name}/disable_ask 1 -i -u 5000
vconftool set -t bool memory/private/%{name}/hotspot/enable_ask 1 -i -u 5000

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%attr(755,-,-) %{_sysconfdir}/init.d/quickpanel
%attr(775,app,app) %{DATADIR}
%{DATADIR}/*
%{PREFIX}/bin/*
%{RESDIR}/*
/usr/share/packages/%{name}.xml
%{_sysconfdir}/rc.d/rc3.d/S51quickpanel
%{_sysconfdir}/rc.d/rc5.d/S51quickpanel
%{_sysconfdir}/init.d/quickpanel
%{_libdir}/systemd/user/quickpanel.service
%{_libdir}/systemd/user/core-efl.target.wants/quickpanel.service
/usr/share/license/%{name}
