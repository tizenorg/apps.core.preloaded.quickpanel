%bcond_with wayland
%define __usrdir /usr/lib/systemd/user

Name: org.tizen.quickpanel
Summary: Quick access panel for the notifications and various kinds of services.
Version: 0.8.0
Release: 1
Group: Applications/Core Applications
License: Apache-2.0
Source0: %{name}-%{version}.tar.gz
Source103: org.tizen.quickpanel.manifest

%if "%{?profile}" == "wearable"
ExcludeArch: %{arm} %ix86 x86_64
%endif

%if "%{?profile}"=="tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-system-runtime-info)
BuildRequires: pkgconfig(capi-system-info)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(capi-location-manager)
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
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(badge)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(edbus)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(edje)
BuildRequires: pkgconfig(icu-i18n)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(bundle)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(syspopup-caller)
BuildRequires: pkgconfig(minicontrol-viewer)
BuildRequires: pkgconfig(minicontrol-monitor)
BuildRequires: pkgconfig(iniparser)
BuildRequires: pkgconfig(capi-appfw-package-manager)
BuildRequires: pkgconfig(alarm-service)
BuildRequires: pkgconfig(ecore-wayland)
BuildRequires: pkgconfig(voice-control-setting)
BuildRequires: pkgconfig(tzsh-quickpanel-service)
BuildRequires: gettext-tools
BuildRequires: cmake
BuildRequires: edje-tools
Requires(post): /usr/bin/vconftool

%description
Quick Panel

%prep
%setup -q

cp %SOURCE103 %{name}.manifest

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

LDFLAGS+="-Wl,--rpath=%{name}/lib -Wl,--as-needed";
export LDFLAGS
export WINSYS="wayland"
export WAYLAND_SUPPORT=On
export X11_SUPPORT=Off

%cmake . -DPKGNAME=%{name} -DWINSYS=${WINSYS}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%post

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%attr(755,-,-) %{_sysconfdir}/init.d/quickpanel
%{_prefix}/apps/%{name}
%{_prefix}/share/packages/%{name}.xml
%{_prefix}/share/license/%{name}
