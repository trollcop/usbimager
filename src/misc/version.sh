#!/bin/sh

VERSION=`cat ../main.h|grep USBIMAGER_VERSION|cut -d '"' -f 2`
COMMAVER=${VERSION//\./,}

cat <<EOF >Info.plist
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>English</string>
    <key>CFBundleDisplayName</key>
    <string>USBImager</string>
    <key>CFBundleExecutable</key>
    <string>usbimager</string>
    <key>CFBundleGetInfoString</key>
    <string>$VERSION, Copyright 2020 bzt</string>
    <key>CFBundleIconFile</key>
    <string>usbimager.icns</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>USBImager</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>$VERSION</string>
    <key>CFBundleVersion</key>
    <string>$VERSION</string>
    <key>NSHumanReadableCopyright</key>
    <string>Copyright 2020 bzt</string>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
    <key>CGDisableCoalescedUpdates</key>
    <true/>
</dict>
</plist>
EOF

cat <<EOF | sed 's/$/\r/g' >manifest.xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <assemblyIdentity
      version="$VERSION.0"
      processorArchitecture="*"
      name="USBImager"
      type="win32"
  />
  <description>Write out an image file to an USB device.</description>
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v3">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="requireAdministrator" uiAccess="false"/>
      </requestedPrivileges>
    </security>
  </trustInfo>
  <dependency>
    <dependentAssembly>
      <assemblyIdentity
        type="win32"
        name="Microsoft.Windows.Common-Controls"
        version="6.0.0.0"
        processorArchitecture="*"
        publicKeyToken="6595b64144ccf1df"
        language="*"
      />
    </dependentAssembly>
  </dependency>
  <asmv3:application xmlns:asmv3="urn:schemas-microsoft-com:asm.v3">
    <asmv3:windowsSettings xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">
      <dpiAware>true</dpiAware>
    </asmv3:windowsSettings>
  </asmv3:application>
</assembly>
EOF

cat <<EOF | sed 's/$/\r/g' >resource.rc
#include "../resource.h"
#include <windows.h>
#include <commctrl.h>

CREATEPROCESS_MANIFEST_RESOURCE_ID RT_MANIFEST "manifest.xml"
IDI_APP_ICON ICON "usbimager.ico"
IDI_WRITE    ICON "write.ico"
IDI_READ     ICON "read.ico"

VS_VERSION_INFO    VERSIONINFO
FILEVERSION        $COMMAVER,0
PRODUCTVERSION     $COMMAVER,0
FILEFLAGSMASK      VS_FFI_FILEFLAGSMASK
FILEFLAGS          0
FILEOS             VOS_NT_WINDOWS32
FILETYPE           VFT_APP
FILESUBTYPE        VFT2_UNKNOWN
BEGIN
  BLOCK "StringFileInfo"
  BEGIN
    BLOCK "080904b0"
    BEGIN
      VALUE "CompanyName", "bzt@gitlab"
      VALUE "FileDescription", "USB image writer"
      VALUE "FileVersion", "0.0.1"
      VALUE "InternalName", "Win32App"
      VALUE "LegalCopyright", "©2020 bzt@gitlab"
      VALUE "OriginalFilename", "usbimager.exe"
      VALUE "ProductName", "USBImager by bzt"
      VALUE "ProductVersion", "$VERSION"
    END
  END
  BLOCK "VarFileInfo"
  BEGIN
    VALUE "Translation", 0x809, 1200
  END
END

IDC_MAINDLG DIALOGEX 0, 0, 320, 100
STYLE DS_CENTER | DS_SHELLFONT | WS_SYSMENU
FONT 8, "MS Shell Dlg"
CAPTION "USBImager $VERSION"
BEGIN
    EDITTEXT IDC_MAINDLG_SOURCE, 5, 5, 275, 12, ES_AUTOHSCROLL
    PUSHBUTTON "...", IDC_MAINDLG_SELECT, 285, 4, 30, 14
    PUSHBUTTON "&Write", IDC_MAINDLG_WRITE, 5, 20, 150, 16
    PUSHBUTTON "&Read", IDC_MAINDLG_READ, 165, 20, 150, 16
    COMBOBOX IDC_MAINDLG_TARGET_LIST, 5, 40, 310, 71, CBS_DROPDOWNLIST | WS_TABSTOP
    CHECKBOX "&Verify", IDC_MAINDLG_VERIFY, 10, 60, 125, 14
    CHECKBOX "&Compress", IDC_MAINDLG_COMPRESS, 145, 60, 125, 14
    COMBOBOX IDC_MAINDLG_BLKSIZE, 275, 60, 40, 71, CBS_DROPDOWNLIST | WS_TABSTOP
    CONTROL "", IDC_MAINDLG_PROGRESSBAR, "msctls_progress32", PBS_SMOOTH, 5, 80, 310, 4
    LTEXT "", IDC_MAINDLG_STATUS, 5, 86, 310, 14, WS_TABSTOP
END
EOF

cat <<EOF >usbimager.desktop
[Desktop Entry]
Version=$VERSION
Type=Application
Name=USBImager
Comment=Write image files to USB devices
Exec=usbimager
Icon=usbimager
Terminal=false
StartupNotify=false
Categories=Application;System;
EOF

cat <<EOF >deb_control
Package: usbimager
Version: $VERSION
Architecture: ARCH
Essential: no
Section: Admin
Priority: optional
Depends: udisks2
Maintainer: bzt
Homepage: https://gitlab.com/bztsrc/usbimager
Installed-Size: SIZE
Description: A very minimal GUI app to write compressed images to USB sticks and create backups
  Uncompresses gz, bzip2, xz and zip (zip64) images on-the-fly and writes them to USB devices.
  Also capable of sending images through serial port to Raspberry Pi machines, and to create
  backup image files from USB storage devices.
EOF
