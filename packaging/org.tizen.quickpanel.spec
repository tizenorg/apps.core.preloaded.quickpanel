%bcond_with wayland

%define PKGNAME org.tizen.quickpanel
%define PREFIX    /usr/apps/%{PKGNAME}
%define PREFIX_RW    /opt/usr/apps/%{PKGNAME}
%define RESDIR    %{PREFIX}/res
%define DATADIR    %{PREFIX}/data

Name:       org.tizen.quickpanel
Summary:    Quick Panel
Version:    0.6.23
Release:    1
Group:      util
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source102:  quickpanel-system.service

%if "%{?tizen_profile_name}" == "wearable" 
ExcludeArch: %{arm} %ix86 x86_64
%endif

%if "%{?tizen_profile_name}"=="tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-system-runtime-info)
BuildRequires: pkgconfig(capi-system-info)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(capi-network-wifi)
BuildRequires: pkgconfig(capi-network-bluetooth)
BuildRequires: pkgconfig(capi-network-tethering)
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(capi-media-player)
BuildRequires: pkgconfig(capi-media-sound-manager)
BuildRequires: pkgconfig(capi-media-metadata-extractor)
BuildRequires: pkgconfig(capi-system-system-settings)
BuildRequires: pkgconfig(capi-base-utils-i18n)
BuildRequires: pkgconfig(capi-ui-efl-util)
BuildRequires: pkgconfig(tapi)
BuildRequires: pkgconfig(feedback)
BuildRequires: pkgconfig(appcore-common)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(badge)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(libprivilege-control)
BuildRequires: pkgconfig(edbus)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(edje)
BuildRequires: pkgconfig(icu-i18n)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(efl-assist)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(minicontrol-viewer)
BuildRequires: pkgconfig(minicontrol-monitor)
BuildRequires: pkgconfig(pkgmgr)
BuildRequires: pkgconfig(pkgmgr-info)
BuildRequires: pkgconfig(iniparser)
BuildRequires: pkgconfig(alarm-service)
BuildRequires: gettext-tools
BuildRequires: cmake
BuildRequires: edje-tools

%if %{with wayland}
BuildRequires: pkgconfig(ecore-wayland)
%else
BuildRequires: pkgconfig(ecore-x)
BuildRequires: pkgconfig(x11)
BuildRequires: pkgconfig(xi)
BuildRequires: pkgconfig(utilX)
BuildRequires: pkgconfig(inputproto)
%endif

Requires(post): /usr/bin/vconftool
Requires: e17
%description
Quick Panel

%prep
%setup -q


%build
%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

%if %{with wayland}
export WAYLAND_SUPPORT=On
export X11_SUPPORT=Off
%else
export WAYLAND_SUPPORT=Off
export X11_SUPPORT=On
%endif


LDFLAGS+="-Wl,--rpath=%{PREFIX}/lib -Wl,--as-needed";export LDFLAGS
LDFLAGS="$LDFLAGS" %cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX} -DPREFIX_RW=%{PREFIX_RW} -DWAYLAND_SUPPORT=${WAYLAND_SUPPORT} -DX11_SUPPORT=${X11_SUPPORT} \

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
install -m 0644 %SOURCE102 %{buildroot}%{_libdir}/systemd/system/quickpanel.service
ln -s ../quickpanel.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/quickpanel.service

mkdir -p %{buildroot}/usr/share/license
cp -f LICENSE %{buildroot}/usr/share/license/%{PKGNAME}


%post


%files
%manifest %{PKGNAME}.manifest
%defattr(-,root,root,-)
%attr(755,-,-) %{_sysconfdir}/init.d/quickpanel
%attr(775,app,app) %{DATADIR}
%attr(775,app,app) %{PREFIX_RW}/data
%{PREFIX_RW}/data
%{PREFIX}/bin/*
%{RESDIR}/*
/usr/share/packages/%{PKGNAME}.xml
%{_sysconfdir}/init.d/quickpanel
%{_libdir}/systemd/system/quickpanel.service
%{_libdir}/systemd/system/multi-user.target.wants/quickpanel.service
/usr/share/license/%{PKGNAME}
/etc/smack/accesses.d/%{PKGNAME}.efl
/usr/apps/%{PKGNAME}/shared/res/icons/*
/usr/apps/%{PKGNAME}/shared/res/noti_icons/*
