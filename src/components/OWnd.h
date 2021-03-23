/*----------------------------------------------------------------------
Copyright (c) 1998 Russell Freeman. All Rights Reserved.
Email: russf@gipsysoft.com
Web site: http://www.gipsysoft.com

This notice must remain intact
This file belongs wholly to Russell Freeman
You may use this file compiled form in your applications.
You may not sell this file in source form.
This source code may not be distributed as part of a library,

This file is provided 'as is' with no expressed or implied warranty.
The author accepts no liability if it causes any damage to your
computer.

Please use and enjoy. Please let me know of any bugs/mods/improvements 
that you have found/implemented and I will fix/incorporate them into this
file.

File:	OWnd.h
Owner:	russf@gipsysoft.com
Purpose:	Origin Window header file.
----------------------------------------------------------------------*/
#ifndef OWND_H
#define OWND_H


#define OWND_LIBNAME "OWND"

#ifndef BUILD_OWND
	#ifndef OWND_DYNAMIC
		#pragma comment(lib, OWND_LIBNAME )
	#endif // OWND_DYNAMIC
#endif // BUILD_OWND

//	call to start panning.
extern "C" HWND WINAPI StartPanning( HWND hwndParent, BOOL bCanScrollHorizontal, BOOL bCanScrollVertical, POINT pt );

//
//	if OWND_DYNAMIC is defined then the library will not be statically link by the #pragma comment
//	and you can use the following macros to find and call the function
//	
#define START_PANNING_PROC	"StartPanning"
typedef  HWND ( WINAPI *StartPanProc ) ( HWND hwndParent, BOOL bCanScrollVert, BOOL bCanScrollHorz, POINT point );

//
//	registered message sent by the origin window when you need to update your window.
#define OWND_WINDOW_UPDATE _T("gs_OriginWindowUpdate")

//
//	registered message sent by the origin window is about to be destroyed
#define OWND_WINDOW_END _T("gs_OriginWindowEnd")

#endif //OWND_H