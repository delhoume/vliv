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

File:	OriginWnd.cpp
Owner:	russf@gipsysoft.com
Purpose:	Implementation of the origin window.
----------------------------------------------------------------------*/
#include "stdafx.h"
#include <ownd.h>
#include "OriginWnd.h"
#include "resource.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
enum {
			knTimerID = 999
			, knTimerDuration = 25
			, knDistanceToScrollDivisor = 16
		};

static const UINT uOriginWindowUpdateMessage = ::RegisterWindowMessage( OWND_WINDOW_UPDATE );
static const UINT uOriginWindowEndMessage = ::RegisterWindowMessage( OWND_WINDOW_END );


//
//	One and only origin window, used by the one and only hook.
static COriginWnd *g_pOriginWnd = NULL;
static HHOOK g_hdlHook = NULL;

//
//	These arrays are identical except for the middle cursor, this just makes 
static const UINT g_arrCursorsBoth[3][3] =
												{
													IDC_NW,			IDC_WEST,		IDC_SW,
													IDC_NORTH,	IDC_NOMOVE,	IDC_SOUTH,
													IDC_NE,			IDC_EAST,		IDC_SE,
												};

static const UINT g_arrCursorsVert[3][3] =
												{
													IDC_NW,			IDC_WEST,		IDC_SW,
													IDC_NORTH,	IDC_NOMOVE_VERTICAL,	IDC_SOUTH,
													IDC_NE,			IDC_EAST,		IDC_SE,
												};

static const UINT g_arrCursorsHorz[3][3] =
												{
													IDC_NW,			IDC_WEST,		IDC_SW,
													IDC_NORTH,	IDC_NOMOVE_HORIZONTAL,	IDC_SOUTH,
													IDC_NE,			IDC_EAST,		IDC_SE,
												};


static CursorArray FASTCALL GetCursorArray( BOOL bHorz, BOOL bVert )
//
//	Return a pointer to the array above to use for the mouse cursor
{
	if( bHorz == FALSE )
	{
		return &g_arrCursorsVert;
	}
	else if( bVert == FALSE )
	{
		return &g_arrCursorsHorz;
	}
	return &g_arrCursorsBoth;
}


COriginWnd::COriginWnd( BOOL bCanScrollHorizontal, BOOL bCanScrollVertical, POINT pt )
	: m_sizeBitmap( 0, 0 )
	, m_bScrollVert( bCanScrollVertical ? 1 : 0 )		//	Our boolean values are used in the lookup table below so the must be either one or zero
	, m_bScrollHorz( bCanScrollHorizontal ? 1 : 0 )
	, m_ptOrigin( pt )
	, m_sizeDistance( 0, 0 )
	, m_ptCursorPrevious( 0, 0 )
	, m_parrnIDs( GetCursorArray( m_bScrollHorz, m_bScrollVert ) )
	, m_bScrolledAlready( FALSE )
	, m_hwnd( NULL )
	, m_hwndParent( NULL )
	, m_hcurOld( NULL )
{

	//	It's pointless displaying the origin window when you are not going to do any scrolling
	ASSERT( m_bScrollVert || m_bScrollHorz );

	//
	//	Determine which of the origin bitmaps to display using a simple table. We want pretty
	//	much the same behaviour as IE.
	static const UINT arrOrigin[2][2] = { 0, IDB_ORIGIN_VERTICAL, IDB_ORIGIN_HORIZONTAL, IDB_ORIGIN_ALL };
	ASSERT( arrOrigin[ m_bScrollHorz ][ m_bScrollVert ] != 0 );	//	Oops, how did this happen?
	m_bitmap = ::LoadBitmap( g_hInst, MAKEINTRESOURCE( arrOrigin[ m_bScrollHorz ][ m_bScrollVert ] ) );

	BITMAP bmp;
	VAPI( GetObject( m_bitmap, sizeof( bmp ), &bmp ) );
	m_sizeBitmap.cx = bmp.bmWidth;
	m_sizeBitmap.cy = bmp.bmHeight;

}

COriginWnd::~COriginWnd()
{

}


LRESULT CALLBACK HookProc(int code, WPARAM wParam ,LPARAM lParam)
//
//	Hook procedure to catch messages that should terminate our origin window.
{
	MSG		*pMsg = reinterpret_cast<MSG *>(lParam);

	if( pMsg->hwnd == g_pOriginWnd->GetParentWindow()  )
	{
		switch( pMsg -> message )
		{
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_CHAR:
		case WM_KILLFOCUS:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_MOUSEWHEEL:
			g_pOriginWnd->DestroyWindow();
			break;
		default:
			//	Intentionally do nothing
			;
		}
	}
	return CallNextHookEx( g_hdlHook, code, wParam, lParam );
}


LRESULT CALLBACK OWndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
	case WM_NCCREATE:
		SetWindowLong( hwnd, 0, reinterpret_cast<long>( reinterpret_cast<LPCREATESTRUCT>( lParam )->lpCreateParams ) );
		return 1;

	default:
		{
			COriginWnd *pOWnd = reinterpret_cast<COriginWnd *>( GetWindowLong( hwnd, 0 ) );
			if( pOWnd )
			{
				return pOWnd->WndProc( hwnd, message, wParam, lParam );
			}
		}
	}
	return DefWindowProc( hwnd, message, wParam, lParam );
}


static LPCTSTR RegisterOwndClass()
{
	static ATOM g_Class = NULL;
	if( g_Class == NULL )
	{
		WNDCLASSEX wcex;
		ZeroStructure( wcex );

		wcex.cbSize = sizeof( WNDCLASSEX );

		wcex.style			= CS_DBLCLKS | CS_BYTEALIGNCLIENT;
		wcex.lpfnWndProc	= (WNDPROC)OWndProc;
		wcex.hInstance		= g_hInst;
		wcex.lpszClassName	= _T("gs_Ownd10");
		wcex.cbWndExtra = sizeof( COriginWnd * );

		g_Class = RegisterClassEx( &wcex );
	}
	return _T("gs_Ownd10");
}


HWND COriginWnd::Create( HWND hwndParent )
{
	m_hwndParent = hwndParent;
	//
	//	Save the cursor, suggestion from Jonathan Caryl.
  m_hcurOld = GetCursor();

	CPoint pt = m_ptOrigin;
	::ClientToScreen( hwndParent, &pt );


	//
	//	We create the window, the centre of the window should be the origin point so when all of the startup is done the cursor
	//	will appear over the centre of the origin window.
	const int nHalfHeight = m_sizeBitmap.cy / 2;
	const int nHalfWidth = m_sizeBitmap.cx / 2;

	m_hwnd = CreateWindowEx( WS_EX_TOOLWINDOW, RegisterOwndClass(), NULL, WS_CLIPSIBLINGS | WS_POPUP, pt.x - nHalfWidth, pt.y - nHalfHeight, m_sizeBitmap.cx, m_sizeBitmap.cy, m_hwndParent, 0, g_hInst, this );
	if( m_hwnd )
	{
		VAPI( ::SetWindowPos( m_hwnd, HWND_TOPMOST,0 ,0 ,0 ,0, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE ) );

		//
		//	Determine the thresholds for the cursor changing, while the cursor is between these points the cursor does not
		//	change to one of the diagonal cursors. I think this is how IE does it - sure looks like it.
		m_rcCursorMargin.Set( m_ptOrigin.x - nHalfWidth, m_ptOrigin.y - nHalfHeight, m_ptOrigin.x + nHalfWidth, m_ptOrigin.y + nHalfHeight );

		//
		//	We want a circular(or as much as the bitmap allows) window, so do the usual
		//	thing of creating a region and assigning it to the window.
		//	OBOs(+1s) for region spotted and fixed by Simon Clark.
		HRGN rgn = CreateEllipticRgn( 0, 0, m_sizeBitmap.cx + 1, m_sizeBitmap.cy + 1);
		VAPI( SetWindowRgn( m_hwnd, rgn, TRUE ) );

		//
		//	Set our mouse capture so that all mouse messages go to us and also set a timer
		//	so that we can periodically send our message to our parent.
		SetCapture( m_hwnd );
		VAPI( SetTimer( m_hwnd, knTimerID, knTimerDuration, NULL ) == knTimerID );
		g_pOriginWnd = this;
		g_hdlHook = SetWindowsHookEx( WH_GETMESSAGE, HookProc, NULL, GetCurrentThreadId() );
	}

	return m_hwnd;
}


LRESULT COriginWnd::WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint( hwnd, &ps );
			
			HDC hdcMem =  CreateCompatibleDC( ps.hdc );
			HGDIOBJ hOld = SelectObject( hdcMem, m_bitmap );
			VAPI( BitBlt( ps.hdc, 0, 0, m_sizeBitmap.cx, m_sizeBitmap.cy, hdcMem, 0, 0, SRCCOPY ) );
			(void)SelectObject( hdcMem, hOld );
			DeleteDC( hdcMem );
			EndPaint( hwnd, &ps );
		}
		break;

	case WM_NCDESTROY:
		SetWindowLong( hwnd, 0, 0 );
		delete this;
		SetCursor( m_hcurOld );
		break;

	case WM_DESTROY:
		VERIFY( UnhookWindowsHookEx( g_hdlHook ) );
		g_hdlHook = NULL;
		break;
		
	case WM_RBUTTONDOWN:			//	Intentionally drop through!
	case WM_MBUTTONDOWN:			//	Intentionally drop through!
	case WM_LBUTTONDOWN:			//	Intentionally drop through!
	case WM_CANCELMODE:				//	Intentionally drop through!
	case WM_CAPTURECHANGED:
		DestroyWindow();
		break;		

	case WM_TIMER:
		OnTimer();
		break;

	case WM_MBUTTONUP:
		if( m_bScrolledAlready )
		{
			DestroyWindow( );
		}	
		break;

	default:
		return DefWindowProc( hwnd, message, wParam, lParam );
	}
	return 0;
}




HCURSOR COriginWnd::GetNewCursorAndDistance( CPoint pt )
//
//	Figures out the new cursor to display and the distance the cursor is from our
//	origin point.
//
//	Returns a new cursor or NULL if the cursor and therefore the distance has not changed
{
	if( m_ptCursorPrevious != pt )
	{
		m_ptCursorPrevious = pt;

		//
		//	If the point is within one of our margins or we are doing one way scrolling then we correct the point
		//	by setting it's axis value to the same as our origin. This makes makes the cursor behaviour
		//	the same as IE.
		if( m_bScrollHorz == FALSE || ( pt.x > m_rcCursorMargin.left && pt.x < m_rcCursorMargin.right ) )
			pt.x = m_ptOrigin.x;

		if( m_bScrollVert == FALSE || ( pt.y > m_rcCursorMargin.top && pt.y < m_rcCursorMargin.bottom ) )
			pt.y = m_ptOrigin.y;

		//
		//	Determine the distance the the cursor is from our origin point
		m_sizeDistance.cx = pt.x - m_ptOrigin.x;
		m_sizeDistance.cy = pt.y - m_ptOrigin.y;

		//
		//	Now that we have our distance from our origin and we have normalised it to be within our margins
		//	where necessary we now figure out which cursor to use, this is a simple lookup into a table again.
		const int nHoriz = max( -1, min( 1, m_sizeDistance.cx ) ) + 1;
		const int nVert = max( -1, min( 1, m_sizeDistance.cy ) ) + 1;

		return ::LoadCursor( g_hInst, MAKEINTRESOURCE( (*m_parrnIDs)[nHoriz][nVert] ) );
	}
	return NULL;
} 


void COriginWnd::OnTimer()
{
	CPoint ptCursor;
	VERIFY( GetCursorPos( &ptCursor ) );

	ScreenToClient( m_hwndParent, &ptCursor );
	HCURSOR hc = GetNewCursorAndDistance( ptCursor );
	if( hc )
	{
		SetCursor( hc );
	}

	//
	//	If the parent did not respond then we will handle the scrolling.
	CSize sizeLocalDistance( m_sizeDistance.cx / knDistanceToScrollDivisor, m_sizeDistance.cy / knDistanceToScrollDivisor );
	if( !SendMessage( m_hwndParent, uOriginWindowUpdateMessage, 0, reinterpret_cast<LPARAM>( &sizeLocalDistance ) ) )
	{
		int nMin, nMaxHorz, nMaxVert;
		GetScrollRange( m_hwnd, SB_HORZ, &nMin, &nMaxHorz);
		GetScrollRange( m_hwnd, SB_VERT, &nMin, &nMaxVert);
		CScrollInfo infoHorz( SIF_PAGE | SIF_POS ), infoVert( SIF_PAGE | SIF_POS );
		if( GetScrollInfo( m_hwnd, SB_HORZ, &infoHorz) )
		{
			nMaxHorz -= max(infoHorz.nPage-1,0);
		}

		if( GetScrollInfo( m_hwnd, SB_VERT, &infoVert ) )
		{
			nMaxVert -= max(infoVert.nPage-1,0);
		}

		const CSize sizeScrollLimit( nMaxHorz, nMaxVert );

		const int nHorzScrollPos = infoHorz.nPos;
		const int nVertScrollPos = infoVert.nPos;

		const int nNewHorzScrollPos = max( 0, min( sizeScrollLimit.cx, nHorzScrollPos + sizeLocalDistance.cx ) );
		const int nNewVertScrollPos = max( 0, min( sizeScrollLimit.cy, nVertScrollPos + sizeLocalDistance.cy ) );

		CSize sizeDiff( nNewHorzScrollPos - nHorzScrollPos , nNewVertScrollPos - nVertScrollPos  );
		
		if( sizeDiff.cx || sizeDiff.cy )
		{
			m_bScrolledAlready = TRUE;

			if( nNewVertScrollPos != infoVert.nPos )
				(void)SetScrollPos( m_hwndParent, SB_VERT, nNewVertScrollPos, FALSE );

			if( nNewHorzScrollPos != infoHorz.nPos )
				(void)SetScrollPos( m_hwndParent, SB_HORZ, nNewHorzScrollPos, FALSE );

			VAPI( ScrollWindow( m_hwndParent, -sizeDiff.cx, -sizeDiff.cy, NULL, NULL ) );
			VAPI( UpdateWindow( m_hwndParent ) );
		}
	}
}


void COriginWnd::DestroyWindow()
{
	VAPI( ::DestroyWindow( m_hwnd ) );
	(void)PostMessage( m_hwndParent, uOriginWindowEndMessage, 0, 0 );
}
