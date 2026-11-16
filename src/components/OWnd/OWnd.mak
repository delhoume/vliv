# NMAKE build file for OWnd.
# Run from this directory in an x64 Native Tools prompt with:
# nmake /f OWnd.mak [CFG=Debug|Release]

!IFNDEF CFG
CFG=Release
!ENDIF

!IF "$(CFG)" == "Debug"
OUTDIR=build\Standalone\X64\Debug
CFLAGS=/nologo /W3 /Zi /Od /D_DEBUG /DSTRICT /DWIN32 /D_WINDOWS /D_USRDLL /DBUILD_OWND /MTd /I".."
LDFLAGS=/DEBUG
RCFLAGS=/nologo /D_DEBUG /DWIN32 /D_WINDOWS
!ELSEIF "$(CFG)" == "Release"
OUTDIR=build\Standalone\X64\Release
CFLAGS=/nologo /W3 /O2 /DNDEBUG /DSTRICT /DWIN32 /D_WINDOWS /D_USRDLL /DBUILD_OWND /MT /I".."
LDFLAGS=
RCFLAGS=/nologo /DNDEBUG /DWIN32 /D_WINDOWS
!ELSE
!ERROR CFG must be Debug or Release
!ENDIF

!IF "$(UNICODE)" == "1"
CFLAGS=$(CFLAGS) /DUNICODE /D_UNICODE
RCFLAGS=$(RCFLAGS) /DUNICODE /D_UNICODE
!ENDIF
CC=cl.exe
RC=rc.exe
LD=link.exe

OBJS=$(OUTDIR)\OriginWnd.obj $(OUTDIR)\OWnd.obj $(OUTDIR)\StartPanning.obj $(OUTDIR)\StdAfx.obj
RESOURCE=$(OUTDIR)\OWnd.res

all: OWnd.dll

$(OUTDIR):
	if not exist "$(OUTDIR)" mkdir "$(OUTDIR)"

# Prevent stale objects from another toolchain architecture being linked.
$(OUTDIR)\OriginWnd.obj: FORCE OriginWnd.cpp OriginWnd.h StdAfx.h resource.h ..\OWnd.h $(OUTDIR)
	$(CC) $(CFLAGS) /Fd"$(OUTDIR)\OWnd.pdb" /c OriginWnd.cpp /Fo"$@"

$(OUTDIR)\OWnd.obj: FORCE OWnd.cpp StdAfx.h $(OUTDIR)
	$(CC) $(CFLAGS) /Fd"$(OUTDIR)\OWnd.pdb" /c OWnd.cpp /Fo"$@"

$(OUTDIR)\StartPanning.obj: FORCE StartPanning.cpp OriginWnd.h StdAfx.h ..\OWnd.h $(OUTDIR)
	$(CC) $(CFLAGS) /Fd"$(OUTDIR)\OWnd.pdb" /c StartPanning.cpp /Fo"$@"

$(OUTDIR)\StdAfx.obj: FORCE StdAfx.cpp StdAfx.h $(OUTDIR)
	$(CC) $(CFLAGS) /Fd"$(OUTDIR)\OWnd.pdb" /c StdAfx.cpp /Fo"$@"

$(RESOURCE): OWnd.rc resource.h $(OUTDIR)
	$(RC) $(RCFLAGS) /Fo"$(RESOURCE)" OWnd.rc

FORCE:

Ownd.dll: FORCE $(OBJS) $(RESOURCE) OWnd.def
	$(LD) /nologo /DLL /MACHINE:X64 /DEF:OWnd.def /OUT:"$@" /IMPLIB:"Ownd.lib" /PDB:"Ownd.pdb" $(LDFLAGS) $(OBJS) $(RESOURCE) user32.lib gdi32.lib kernel32.lib

clean:
	-if exist build\Standalone\X64\Debug\OriginWnd.obj del /q build\Standalone\X64\Debug\OriginWnd.obj build\Standalone\X64\Debug\OWnd.obj build\Standalone\X64\Debug\StartPanning.obj build\Standalone\X64\Debug\StdAfx.obj build\Standalone\X64\Debug\OWnd.res build\Standalone\X64\Debug\OWnd.pdb
	-if exist build\Standalone\X64\Release\OriginWnd.obj del /q build\Standalone\X64\Release\OriginWnd.obj build\Standalone\X64\Release\OWnd.obj build\Standalone\X64\Release\StartPanning.obj build\Standalone\X64\Release\StdAfx.obj build\Standalone\X64\Release\OWnd.res build\Standalone\X64\Release\OWnd.pdb
	-del /q OWnd.dll OWnd.lib OWnd.exp OWnd.pdb
