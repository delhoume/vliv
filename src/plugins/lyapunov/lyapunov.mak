CC  = cl
LD  = link

VLIVDIR = ..\..

DEBUG = /Ox 

CFLAGS = /nologo /W3 $(DEBUG) /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. /I$(VLIVDIR)

SYSLIBS = wininet.lib user32.lib gdi32.lib kernel32.lib comctl32.lib comdlg32.lib shlwapi.lib \
	  shell32.lib advapi32.lib version.lib strsafe.lib

all: lyapunov.dll

lyapunov.obj : lyapunov.c lyapunov.h
	$(CC) $(CFLAGS) /c lyapunov.c

lyapunov.dll: lyapunov.obj
	$(LD) /dll /out:lyapunov.dll lyapunov.obj

clean:
	del lyapunov.obj lyapunov.dll
