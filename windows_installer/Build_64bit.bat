@rem load the  NIGHTLY_BUILD_SVN variable from the txt file
for /f "delims== tokens=1,2" %%G in (Build_Version_Number.txt) do set %%G=%%H

if "%GITHUB_ACTIONS%" == "true" (
    "C:\Program Files (x86)\NSIS\makensis.exe" "/DBUILD_TYPE=64" "/DNIGHTLY_BUILD_SVN=%NIGHTLY_BUILD_SVN%" "Installer_NSIS_Simple.nsi"
) else (
    "C:\Program Files (x86)\NSIS\makensis.exe" "/DBUILD_TYPE=64" "/DNIGHTLY_BUILD_SVN=%NIGHTLY_BUILD_SVN%" "Installer_NSIS_UMUI.nsi"
)
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" /Qp "/DBUILD_TYPE=64" "/DNIGHTLY_BUILD_SVN=%NIGHTLY_BUILD_SVN%" "/DCB_ADMIN_INSTALLER=True" "Installer_ISS_full_only.iss"
