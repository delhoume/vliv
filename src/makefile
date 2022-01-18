CC  = cl
RC  = rc
LD  = link

#DEBUG=/Od /Zi
#LDDEBUG=/debug

DEBUG=/Ox 

VERSION=2.5
VERSIONSHORT=254

CFLAGS = /nologo /W3 $(DEBUG) /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. 
LDFLAGS = $(LDDEBUG) /SUBSYSTEM:WINDOWS /nodefaultlib:libc /VERSION:$(VERSION)

SYSLIBS = wininet.lib user32.lib gdi32.lib kernel32.lib comctl32.lib comdlg32.lib shlwapi.lib \
	  shell32.lib advapi32.lib version.lib strsafe.lib msimg32.lib winmm.lib

OBJECTS = vliv.obj urlctrl.obj dialogs.obj handlers.obj bitmap.obj recent.obj window.obj tilemgr.obj rawinput.obj

all: vliv.exe

vliv.obj : vliv.c vliv.h
	$(CC) $(CFLAGS) /c vliv.c

urlctrl.obj : urlctrl.c
	$(CC) $(CFLAGS) /c urlctrl.c

dialogs.obj : dialogs.c
	$(CC) $(CFLAGS) /c dialogs.c

recent.obj : recent.c
	$(CC) $(CFLAGS) /c recent.c

bitmap.obj : bitmap.c
	$(CC) $(CFLAGS) /c bitmap.c

window.obj : window.c
	$(CC) $(CFLAGS) /c window.c

handlers.obj : handlers.c
	$(CC) $(CFLAGS) /c handlers.c

tilemgr.obj : tilemgr.c
	$(CC) $(CFLAGS) /c tilemgr.c

rawinput.obj : rawinput.c
	$(CC) $(CFLAGS) /c rawinput.c

vliv.res : resources\vliv.rc
	$(RC) /fo vliv.res resources\vliv.rc

vliv.exe : $(OBJECTS) vliv.res
	$(LD) /OUT:vliv.exe $(LDFLAGS) $(OBJECTS) vliv.res $(SYSLIBS)

languages: fra.dll

fra.dll: resources/fra.rc
	$(RC) /fo fra.res resources/fra.rc
	$(LD) /dll /machine:x86 /noentry /out:fra.dll fra.res

clean:
	del /q *.pdb *.ilk *.obj vliv.exe *~ *.res
