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

File:	OriginWnd.h
Owner:	russf@gipsysoft.com
Purpose:	Origin Window class
----------------------------------------------------------------------*/
#ifndef ORIGINWND_H
#define ORIGINWND_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef const UINT (*CursorArray)[3][3];

class COriginWnd
{
public:
	void DestroyWindow();
	COriginWnd( BOOL bCanScrollHorizontal, BOOL bCanScrollVertical, POINT pt );
	~COriginWnd();

	HWND Create( HWND hwndParent );

	inline HWND GetWindow() const { return m_hwnd; }
	inline HWND GetParentWindow() const { return m_hwndParent; }

	LRESULT WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
private:
	void OnTimer();
	HCURSOR GetNewCursorAndDistance( CPoint pt );

	//
	//	The bitmap we use for the origin, plus the bitmap description structure
	HBITMAP m_bitmap;
	CSize m_sizeBitmap;

	//
	//	The directions we can scroll in
	const BOOL m_bScrollVert, m_bScrollHorz;
	
	//
	//	The origin point, used to calculate the current mouse distance, this is then used
	//	to pass on to determine how fast we scroll and in what directions
	const CPoint m_ptOrigin;
	CSize m_sizeDistance;
	CRect m_rcCursorMargin;

	//
	//	An optimisation to allow us to not calculate the cursor and the distance if the
	//	mouse has not been moved.
	CPoint m_ptCursorPrevious;

	const CursorArray m_parrnIDs;

	//
	//	Whether we have issued any scroll messages yet, used to process the middle button up event
	//	so that we can properly simulate IE behaviour
	BOOL m_bScrolledAlready;

	HWND m_hwnd;
	HWND m_hwndParent;

	HCURSOR m_hcurOld;

private:
	COriginWnd();
	COriginWnd( const COriginWnd& );
	COriginWnd& operator =( const COriginWnd& );
};

#endif //ORIGINWND_H