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

File:	StartPanning.cpp
Owner:	russf@gipsysoft.com
Purpose:	Function to initiate panning
----------------------------------------------------------------------*/
#include "stdafx.h"
#include "OriginWnd.h"
#include <Ownd.h>

extern "C" HWND WINAPI StartPanning( HWND hwndParent, BOOL bCanScrollHorizontal, BOOL bCanScrollVertical, POINT pt )
{
	COriginWnd *pOriginWnd = new COriginWnd( bCanScrollHorizontal, bCanScrollVertical, pt );
	HWND hwndRet = NULL;
	if( pOriginWnd )
	{
		hwndRet = pOriginWnd->Create( hwndParent );
		if( !hwndRet )
		{
			delete pOriginWnd;
		}
	}
	return NULL;
}
