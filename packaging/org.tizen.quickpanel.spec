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
LDFLAGS+="-Wl,--rpath=%{PREFIX}/lib -Wl,--as-needed";export LDFLAGS
cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%clean
rm -rf %{buildroot}

%post
INHOUSE_ID="5000"

change_dir_permission()
{
    chown $INHOUSE_ID:$INHOUSE_ID $@ 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "Failed to change the owner of $@"
    fi  
    chmod 775 $@ 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "Failed to change the perms of $@"
    fi  
}

change_file_executable()
{
    chmod +x $@ 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "Failed to change the perms of $@"
    fi  
}

change_dir_permission %{DATADIR}
change_file_executable /etc/init.d/quickpanel
mkdir -p /etc/rc.d/rc5.d/
mkdir -p /etc/rc.d/rc3.d/
ln -s /etc/init.d/quickpanel /etc/rc.d/rc5.d/S51quickpanel
ln -s /etc/init.d/quickpanel /etc/rc.d/rc3.d/S51quickpanel

%postun
/sbin/ldconfig
rm -f /etc/rc.d/rc5.d/S51quickpanel
rm -f /etc/rc.d/rc3.d/S51quickpanel

%files
%defattr(-,root,root,-)
/etc/init.d/quickpanel
/opt/apps/org.tizen.quickpanel/bin/*
/opt/apps/org.tizen.quickpanel/res/*
/opt/share/applications/org.tizen.quickpanel.desktop
