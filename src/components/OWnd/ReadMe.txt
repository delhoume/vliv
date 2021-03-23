Written by Russell Freeman Copyright ©1998.
Email: russf@gipsysoft.com
Web site: http://www.gipsysoft.com/
Last updated: December 13, 1998



Why Use Ownd?
=============
OWnd provides the bulk of the work associated with custom origin window
handling. It gives the user a similar experience to Microsoft Internet
Explorer when they press the middle mouse button in your application.
Using OWnd will allow you to choose how and when auto-panning takes place, 

The Intellimouse driver version 2.2 provides a great deal of automatic
panning behaviour for controls that did not support it. Relying on this
lets your application down. If the user has not installed the driver then
you have no support, if they have instaled it then you get the lowest
common denominator support which in most cases is ugly and jerky scrolling.

Add Ownd to your project and in just a few lines of code you can have a similar
auto-panning look and feel as Microsoft Internet Explorer.


What is it?
===========
OWnd is the little window that apears in Microsoft Internet Explorer and Zoom+
when you press the middle mouse button.

Once you make the single function call this window will appear. It will send you
a message telling when to scroll your window, by how much and in what direction.
It will scroll your window for you if your drawing code and your scroll method
can handle it. 

OWnd started when I wrote Internet Explorer style auto-panning for an application
at work. I thought that a reuseable solution would be good for an article on the
CodeGuru web site so I wrote it using MFC. This version is written in pure Win32
API and can be used in either debug or release with no real difference. 


How to Use Ownd
===============

Once the files and paths are setup and ready to use you can start making use of
Ownd by including the Ownd.h file in your source file:

	#include <Ownd.h>

Next, you will need to register the messages sent by OWnd to your window:

	static const UINT uOriginWindowUpdateMessage = ::RegisterWindowMessage( OWND_WINDOW_UPDATE );
	static const UINT uOriginWindowEndMessage = ::RegisterWindowMessage( OWND_WINDOW_END );

Then, in your WM_MBUTTONDOWN handler you will need to call the StartPanning(...)
function.

	StartPanning( m_hwnd, TRUE, TRUE, pt );

Typical applications will check to ensure they can actually scroll before calling
StartPanning(...), this way the user will get good feedback when they press the
middle mouse button.

Scrolling your window takes place in a message handler for OWND_WINDOW_UPDATE.
The Zoom+ handler looks like this:

	if( message == uOriginWindowUpdateMessage )
	{
		const SIZE *psizeDistance = reinterpret_cast<const SIZE *>( lParam );
		MoveZoomBy( psizeDistance->cx, psizeDistance->cy );
		return 1;
	}

If you want to load the OWnd DLL dynamically, as Zoom+ does, then your WM_MBUTTONDOWN
handler would look something like this:

	g_hInstOwnd = ::LoadLibrary( OWND_LIBNAME );
	if( g_hInstOwnd )
	{
		StartPanProc pStartPanning = (StartPanProc)::GetProcAddress( g_hInstOwnd, START_PANNING_PROC );
		if( pStartPanning )
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			pStartPanning( m_hwnd, TRUE, TRUE, pt );
		}
	}

and you then need to respond to the OWND_WINDOW_END message to allow you to unload
the Ownd library, as in this code, again taken from Zoom+:

	if( message == uOriginWindowEndMessage )
	{
		VAPI( ::FreeLibrary( g_hInstOwnd ) );
	}


Reference.
==========

StartPanning( HWND hwndParent, BOOL bCanScrollHorizontal, BOOL bCanScrollVertical, POINT pt )
---------------------------------------------------------------------------------------------

Parameters 

hwndParent 
	A handle to the window that will reciece the messages. 
bCanScrollHorizontal 
	TRUE if the window can scroll horizontally 
bCanScrollVertical 
	TRUE if the window can scroll vertically 
pt 
	The mouse point the origin window should be centred on. 

Return Value
	The HWND of the origing window if successful, otherwise it returns NULL. 

Remarks 
	Call this function to create an origin window and start your window panning.
	Once this function returns your window will start to recieve the
	OWND_WINDOW_UPDATE registered windows messages. When the origin window is
	dismissed your window will recieve an OWND_WINDOW_END registered windows message,
	this gives you an opportunity to do any cleanup required. 


OWND_WINDOW_UPDATE
------------------
This is a registered message. This is sent every time your window needs to scroll. The
message contains the directions and the amounts your window should scroll.


psizeScroll = (PSIZE)lparam;

psizeScroll 
	A pointer to a SIZE structure. 

Return Value 
	If an application processes this message it should return non-zero. 

Remarks 
	Return non-zero signals to Ownd that you have handled the scrolling and that it should
	do nothing. If you return zero ownd will attempt to scroll your window for you using
	the ScrollWindow(...) API. If your window appears to 'drag' when Ownd scrolls or jumps
	and hops then you should perform your own scrolling. 
	
	The psizeScroll members cx and cy are positive to scroll down and right and negative to
	scroll up or left. How you interpret the amount of scroll is up to you, Zoom+ simply
	treats it as the number of pixels to scroll.



OWND_WINDOW_END
---------------
This is a registered message. This is sent as an opportunity for you to perform any
cleanup required.

Return Value 

No return value. 

Remarks 

Posted to your to your window just after the origin window has been destroyed.



Important:
----------
If you redistribute Ownd please leave the zip file intact 
Do not replace/modify the help file.
You must not sell Ownd or redistribute it as part of a commercial
product without prior written agreement from Russ Freeman.

This file is provided "as is" with no expressed or implied warranty.
The author accepts no liability if it causes any damage to your
computer.

Expect bugs.

Please use and enjoy. Please let me know of any bugs/mods/improvements 
that you have found/would like and I will fix/incorporate them.



History
-------
1.0	December 19 1998
The first version released to the web site.