CC  = cl
LD  = link

VLIVDIR = ..\..

DEBUG=/Ox 

CFLAGS = /nologo /W3 $(DEBUG) /D_CRT_SECURE_NO_DEPRECATE /DWIN32 /DWINDOWS /I. /I$(VLIVDIR)

SYSLIBS = 

all: debug.dll

debug.obj : debug.c debug.h
	$(CC) $(CFLAGS) /c debug.c

debug.dll: debug.obj
	$(LD) /dll /out:debug.dll debug.obj

clean:
	del debug.obj debug.dll
