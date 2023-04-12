; Unicode is not enabled by default
; Unicode installers will not be able to run on Windows 9x!
Unicode true

!define APPNAME "haikangnvr"
!define PRODUCT_NAME "海康设备接入测试系统"
!define PRODUCT_VERSION "1.0.0.1"
!define PRODUCT_PUBLISHER "山东作为技术有限公司"
!define PRODUCT_WEB_SITE "www.efforttech.com.cn/"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\haikangnvr.exe"
; 卸载程序注册表路径
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APPNAME}.exe"
!define MUI_FINISHPAGE_RUN_CHECKED
!define MUI_FINISHPAGE_RUN_TEXT "运行${PRODUCT_NAME}"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"

!define MUI_FINISHPAGE_SHOWREADME
!define INSTALL_PATH "install"
!define INSTALL_NAME "efforttech\haikangnvr"

; 压缩算法
SetCompressor lzma

; ------- MUI现代界面定义(1.67版本以上兼容) -------
!include "MUI.nsh"
!include "x64.nsh"
!include "nsProcess.nsh"
!include "LogicLib.nsh"
;!include  "WordFunc.nsh"
;!include "EnvVarUpdate.nsh"

; MUI 预定义常量
!define MUI_ABORTWARNING
!define MUI_ICON "haikangnvr.ico"
!define MUI_UNICON "uninstall.ico"

; 欢迎页面
!insertmacro MUI_PAGE_WELCOME
; 许可协议页面
!insertmacro MUI_PAGE_LICENSE "license.rtf"
; 安装选择路径
!define MUI_PAGE_CUSTOMFUNCTION_SHOW disableOpenFileBrower
!insertmacro MUI_PAGE_DIRECTORY
; 安装过程页面
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_CONFIRM
; 安装完成页面
;!define MUI_FINISHPAGE_RUN "$INSTDIR\payment.exe"
!insertmacro MUI_PAGE_FINISH

; 安装卸载过程页面
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; 安装界面包含的语言设置
!insertmacro MUI_LANGUAGE "SimpChinese"

; 文件版本声明
VIProductVersion "1.0.0.1"
VIAddVersionKey /LANG=2052 "ProductName" "海康设备接入测试系统"
VIAddVersionKey /LANG=2052 "Comments" "软件版权归山东作为技术有限公司所有,他人不得复制或二次开发"
VIAddVersionKey /LANG=2052 "CompanyName" "山东作为技术有限公司"
VIAddVersionKey /LANG=2052 "LegalTrademarks" "Copyright (C) 2023-2035 ${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=2052 "LegalCopyright"  "Copyright (C) 2023-2035 efforttech.ltd"
VIAddVersionKey /LANG=2052 "FileDescription" "海康设备接入测试系统"
VIAddVersionKey /LANG=2052 "FileVersion" "1.0.0.1"
VIAddVersionKey /LANG=2052 "ProductVersion" "1.0.0.1"

; 安装预释放文件
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; -------- MUI 现代界面定义结束 --------
Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "海康设备接入测试系统.exe"
InstallDir "$PROGRAMFILES\${INSTALL_NAME}"
ShowInstDetails show
ShowUninstDetails show
BrandingText "海康设备接入测试系统"
LicenseText "" "我同意(I)"
UninstallButtonText "解除安装(U)"

RequestExecutionLevel admin

; 制作安装程序
Section "MainSection" SEC01
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  CreateShortCut "$DESKTOP\海康设备接入测试系统.lnk" "$INSTDIR\haikangnvr.exe"
  File /r "${INSTALL_PATH}\*.*"
  Call InstallVC
  Delete "$INSTDIR\vc_redist.x64.exe"
SectionEnd

Section -AdditionalIcons
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\${PRODUCT_NAME}.lnk" "$INSTDIR\haikangnvr.exe"
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$INSTDIR\公司网址.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\访问网站.lnk" "$INSTDIR\${PRODUCT_NAME}.url" "" "$SYSDIR\url.dll"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\卸载${PRODUCT_NAME}.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
 
  ;app 注册表内容
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\haikangnvr.exe"
  WriteRegDword HKLM "${PRODUCT_DIR_REGKEY}" "Installed" 1
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "InstalledPath" "$INSTDIR"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "version" "${PRODUCT_VERSION}"

  ;卸载注册表内容
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\haikangnvr.exe"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr HKLM "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"

  ;把安装目录添加到环境变量path中
  ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
  WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "PATH" "$0;$INSTDIR;"
  SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment"

  ;Push $INSTDIR
  ;Call AddToPath
	;${EnvVarUpdate} $0 "PATH" "A" "HKLM" $INSTDIR ; Append
  ;ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "PATH"
  ;WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "PATH" "$0;$INSTDIR;"
  ;SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment"
SectionEnd

; 卸载程序
Section Uninstall
  SetShellVarContext all
  RMDir /r "$INSTDIR"
  Delete "$DESKTOP\海康设备接入测试系统.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\访问网站.lnk"
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\卸载${PRODUCT_NAME}.lnk"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"

  DeleteRegKey HKLM "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
SectionEnd

; INSTALL VC LIB
Function InstallVC
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Version"
   ${If} $0 == ""
      ExecWait "$INSTDIR\vc_redist.x64.exe /q"
   ${EndIf}
FunctionEnd

Function LaunchLink
  ExecShell "" "$INSTDIR\${APPNAME}.exe"
FunctionEnd

; 预定义安装目录
Function .onInit
  nsProcess::_FindProcess "${APPNAME}.exe" $R0

  ${IF} $r0 == 0
    MessageBox MB_OK "安装程序检测到${PRODUCT_NAME}正在运行,请关闭址后再次安装!"
    Quit
  ${EndIf}

	StrCpy $R1 "C:\"
	StrCpy $INSTDIR "$R1${INSTALL_NAME}"
FunctionEnd

Function un.onInit
  nsProcess::_FindProcess "${APPNAME}.exe" $R0

  ${IF} $r0 == 0
    MessageBox MB_OK "卸载程序检测到${PRODUCT_NAME}正在运行,请关闭址后再次卸载!"
    Quit
  ${EndIf}
FunctionEnd

;disable file brower
Function disableOpenFileBrower
  FindWindow $0 "#32770" "" $HWNDPARENT
  GetDlgItem $0 $0 1001
  EnableWindow $0 0

  FindWindow $0 "#32770" "" $HWNDPARENT
  GetDlgItem $0 $0 1019
  EnableWindow $0 0
FunctionEnd

