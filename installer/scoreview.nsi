;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;

;--------------------------------

; The name of the installer
Name "ScoreviewInstaller"

; The file to write
OutFile "ScoreviewInstall.exe"

; The default installation directory
InstallDir $PROGRAMFILES\vreemdelabs\scoreview

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\VREEMDELABS_scoreview" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

; Compression choice
SetCompressor /SOLID lzma

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "Scoreview (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "..\app\dft.cl"
  File "..\app\filterfir.cl"
  File "..\app\glew32.dll"
  File "..\app\libbz2-1.dll"
  File "..\app\libevent_core-2-0-5.dll"
  File "..\app\libevent-2-0-5.dll"
  File "..\app\libFLAC-8.dll"
  File "..\app\libfreetype-6.dll"
  File "..\app\libgcc_s_seh-1.dll"
  File "..\app\libglib-2.0-0.dll"
  File "..\app\libharfbuzz-0.dll"
  File "..\app\libiconv-2.dll"
  File "..\app\libintl-8.dll"
  File "..\app\libjpeg-8.dll"
  File "..\app\liblzma-5.dll"
  File "..\app\libogg-0.dll"
  File "..\app\libpng16-16.dll"
  File "..\app\libportaudio-2.dll"
  File "..\app\libprotobuf-9.dll"
  File "..\app\libsndfile-1.dll"
  File "..\app\libspeex-1.dll"
  File "..\app\libstdc++-6.dll"
  File "..\app\libtiff-5.dll"
  File "..\app\libtinyxml-0.dll"
  File "..\app\libvorbis-0.dll"
  File "..\app\libvorbisenc-2.dll"
  File "..\app\libwebp-5.dll"
  File "..\app\libwinpthread-1.dll"
  File "..\app\mgwfltknox_forms-1.3.dll"
  File "..\app\mgwfltknox_gl-1.3.dll"
  File "..\app\mgwfltknox_images-1.3.dll"
  File "..\app\mgwfltknox-1.3.dll"
  File "..\app\SDL2.dll"
  File "..\app\SDL2_image.dll"
  File "..\app\SDL2_ttf.dll"
  File "..\app\zlib1.dll"
  File "..\app\scoreview.exe"
  SetOutPath "$INSTDIR\dialogs\addinstrumentFLTK\"
  File "..\app\dialogs\addinstrumentFLTK\addinstrument.exe"
  SetOutPath "$INSTDIR\dialogs\configFLTK\"
  File "..\app\dialogs\configFLTK\config.exe"
  SetOutPath "$INSTDIR\dialogs\practiceSDL\"
  File "..\app\dialogs\practiceSDL\practice.exe"
  SetOutPath "$INSTDIR\dialogs\save_openFLTK\"
  File "..\app\dialogs\save_openFLTK\storedialog.exe"
  SetOutPath "$INSTDIR\data\"
  File "..\app\data\ArmWrestler.ttf"
  File "..\app\data\VeraMono.ttf"
  File "..\app\data\cardaddinstr.png"
  File "..\app\data\cardautobeam.png"
  File "..\app\data\cardchordfuse.png"
  File "..\app\data\cardconfig.png"
  File "..\app\data\cardfscale.png"
  File "..\app\data\cardhelp.png"
  File "..\app\data\cardlogsp.png"
  File "..\app\data\cardopensave.png"
  File "..\app\data\cardpractice.png"
  File "..\app\data\cardpracticespeed.png"
  File "..\app\data\cardpracticew.png"
  File "..\app\data\cardrecord.png"
  File "..\app\data\cardscore.png"
  File "..\app\data\cardspectre.png"
  File "..\app\data\cardtrack.png"
  File "..\app\data\cardtscale.png"
  File "..\app\data\contour.png"
  File "..\app\data\images.png"
  File "..\app\data\instruments.png"
  File "..\app\data\skin.png"
  File "..\app\data\bemol.obj"
  File "..\app\data\cleffa.obj"
  File "..\app\data\clefsol.obj"
  File "..\app\data\diese.obj"
  File "..\app\data\doubleflag.obj"
  File "..\app\data\enrest.obj"
  File "..\app\data\flag.obj"
  File "..\app\data\qnrest.obj"
  File "..\app\data\qrest.obj"
  File "..\app\data\sfnrest.obj"
  File "..\app\data\snrest.obj"
  File "..\app\data\tsnrest.obj"
  File "..\app\data\vfboard.obj"
  File "..\app\data\gfboard.obj"
  File "..\app\data\wrest.obj"

  SetOutPath $INSTDIR
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\scoreview "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\scoreview" "DisplayName" "Scoreview"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\scoreview" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\scoreview" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\scoreview" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\scoreview"
  CreateShortCut "$SMPROGRAMS\scoreview\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\scoreview\scoreview.lnk" "$INSTDIR\scoreview.exe" "" "$INSTDIR\scoreview.exe" 0  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\scoreview"
  DeleteRegKey HKLM SOFTWARE\VREEMDELABS_scoreview

  ; Remove files and uninstaller
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\dft.cl
  Delete $INSTDIR\filterfir.cl
  Delete $INSTDIR\glew32.dll
  Delete $INSTDIR\libbz2-1.dll
  Delete $INSTDIR\libevent_core-2-0-5.dll
  Delete $INSTDIR\libevent-2-0-5.dll
  Delete $INSTDIR\libFLAC-8.dll
  Delete $INSTDIR\libfreetype-6.dll
  Delete $INSTDIR\libgcc_s_seh-1.dll
  Delete $INSTDIR\libglib-2.0-0.dll
  Delete $INSTDIR\libharfbuzz-0.dll
  Delete $INSTDIR\libiconv-2.dll
  Delete $INSTDIR\libintl-8.dll
  Delete $INSTDIR\libjpeg-8.dll
  Delete $INSTDIR\liblzma-5.dll
  Delete $INSTDIR\libogg-0.dll
  Delete $INSTDIR\libpng16-16.dll
  Delete $INSTDIR\libportaudio-2.dll
  Delete $INSTDIR\libprotobuf-9.dll
  Delete $INSTDIR\libsndfile-1.dll
  Delete $INSTDIR\libspeex-1.dll
  Delete $INSTDIR\libstdc++-6.dll
  Delete $INSTDIR\libtiff-5.dll
  Delete $INSTDIR\libtinyxml-0.dll
  Delete $INSTDIR\libvorbis-0.dll
  Delete $INSTDIR\libvorbisenc-2.dll
  Delete $INSTDIR\libwebp-5.dll
  Delete $INSTDIR\libwinpthread-1.dll
  Delete $INSTDIR\mgwfltknox_forms-1.3.dll
  Delete $INSTDIR\mgwfltknox_gl-1.3.dll
  Delete $INSTDIR\mgwfltknox_images-1.3.dll
  Delete $INSTDIR\mgwfltknox-1.3.dll
  Delete $INSTDIR\SDL2.dll
  Delete $INSTDIR\SDL2_image.dll
  Delete $INSTDIR\SDL2_ttf.dll
  Delete $INSTDIR\zlib1.dll
  Delete $INSTDIR\scoreview.exe
  Delete $INSTDIR\dialogs\addinstrumentFLTK\addinstrument.exe
  Delete $INSTDIR\dialogs\configFLTK\config.exe
  Delete $INSTDIR\dialogs\practiceSDL\practice.exe
  Delete $INSTDIR\dialogs\save_openFLTK\storedialog.exe
  Delete $INSTDIR\data\ArmWrestler.ttf
  Delete $INSTDIR\data\VeraMono.ttf
  Delete $INSTDIR\data\cardaddinstr.png
  Delete $INSTDIR\data\cardautobeam.png
  Delete $INSTDIR\data\cardchordfuse.png
  Delete $INSTDIR\data\cardconfig.png
  Delete $INSTDIR\data\cardfscale.png
  Delete $INSTDIR\data\cardhelp.png
  Delete $INSTDIR\data\cardlogsp.png
  Delete $INSTDIR\data\cardopensave.png
  Delete $INSTDIR\data\cardpractice.png
  Delete $INSTDIR\data\cardpracticespeed.png
  Delete $INSTDIR\data\cardpracticew.png
  Delete $INSTDIR\data\cardrecord.png
  Delete $INSTDIR\data\cardscore.png
  Delete $INSTDIR\data\cardspectre.png
  Delete $INSTDIR\data\cardtrack.png
  Delete $INSTDIR\data\cardtscale.png
  Delete $INSTDIR\data\contour.png
  Delete $INSTDIR\data\images.png
  Delete $INSTDIR\data\instruments.png
  Delete $INSTDIR\data\skin.png
  Delete $INSTDIR\data\bemol.obj
  Delete $INSTDIR\data\cleffa.obj
  Delete $INSTDIR\data\clefsol.obj
  Delete $INSTDIR\data\diese.obj
  Delete $INSTDIR\data\doubleflag.obj
  Delete $INSTDIR\data\enrest.obj
  Delete $INSTDIR\data\flag.obj
  Delete $INSTDIR\data\qnrest.obj
  Delete $INSTDIR\data\qrest.obj
  Delete $INSTDIR\data\sfnrest.obj
  Delete $INSTDIR\data\snrest.obj
  Delete $INSTDIR\data\tsnrest.obj
  Delete $INSTDIR\data\vfboard.obj
  Delete $INSTDIR\data\gfboard.obj
  Delete $INSTDIR\data\wrest.obj
  
  RMDir  $INSTDIR\data
  RMDir  $INSTDIR\dialogs\

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\scoreview\*.*"
 
  ; Remove directories used
  RMDir "$SMPROGRAMS\scoreview"
  RMDir "$INSTDIR"

SectionEnd
