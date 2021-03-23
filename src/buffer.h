//**********************************************************************
// File: buffer.h				Last Modified:	03/02/01
//						Modified by:	PM
//	
// Purpose:      Custom Buffer-Manipulation Routines
//
// Developed By: Piotr Mintus 2001
//**********************************************************************

#ifndef _BUFFER_H_
#define _BUFFER_H_

#ifdef FillMemory
#undef FillMemory
#endif
#ifdef ZeroMemory
#undef ZeroMemory
#endif
#ifdef CopyMemory
#undef CopyMemory
#endif
#ifdef MoveMemory
#undef MoveMemory
#endif

/* Modified [05-01-03] to mimic FillMemory() parameters not memset() 
   (Thanks to Shailesh) */
__forceinline 
void FillMemory(void *dest,unsigned __int32 count,unsigned __int8 c)
{
	unsigned __int32	size32=count>>2;
	unsigned __int32	fill=(c<<24|c<<16|c<<8|c);
	unsigned __int32	*dest32=(unsigned __int32*)dest;

	switch( (count-(size32<<2)) )
	{
	case 3:	((unsigned __int8*)dest)[count - 3] = c;
	case 2:	((unsigned __int8*)dest)[count - 2] = c;
	case 1:	((unsigned __int8*)dest)[count - 1] = c;
	}

	while( size32-- > 0 )
		*(dest32++) = fill;

}  /* FillMemory */

#define ZeroMemory(dest,count) FillMemory(dest, count, 0)

/* Corrected [07-21-02] (Thanks to Tre_) */
__forceinline 
void CopyMemory(void *dest,const void *src,unsigned __int32 count)
{
	unsigned __int32	size32=count>>2;
	unsigned __int32	*dest32=(unsigned __int32*)dest;
	unsigned __int32	*src32=(unsigned __int32*)src;

	switch( (count-(size32<<2)) )
	{
	case 3:	((unsigned __int8*)dest)[count-3] = 
		((unsigned __int8*)src)[count-3];
	case 2:	((unsigned __int8*)dest)[count-2] = 
		((unsigned __int8*)src)[count-2];
	case 1:	((unsigned __int8*)dest)[count-1] = 
		((unsigned __int8*)src)[count-1];
	}

	/* Modified [05-01-03] to use forward caching */
	while( size32-- > 0 )
		*(dest32++) = *(src32++);

}  /* CopyMemory */


/* Corrected [07-21-02] (Thanks to Tre_) */
__forceinline 
void MoveMemory(void *dest,const void *src,unsigned __int32 count)
{
	unsigned __int32	size32=count>>2,i;
	unsigned __int32	*dest32=(unsigned __int32*)dest;
	unsigned __int32	*src32=(unsigned __int32*)src;

	if( dest > src )
	{
		switch( (count-(size32<<2)) )
		{
		case 3:	((unsigned __int8*)dest)[count-1] = 
			((unsigned __int8*)src)[count-1];
			((unsigned __int8*)dest)[count-2] = 
			((unsigned __int8*)src)[count-2];
			((unsigned __int8*)dest)[count-3] = 
			((unsigned __int8*)src)[count-3];
			break;
		case 2:	((unsigned __int8*)dest)[count-1] = 
			((unsigned __int8*)src)[count-1];
			((unsigned __int8*)dest)[count-2] = 
			((unsigned __int8*)src)[count-2];
			break;
		case 1:	((unsigned __int8*)dest)[count-1] = 
			((unsigned __int8*)src)[count-1];
			break;
		}

		while( size32-- > 0 )
			dest32[size32] = src32[size32];
	}
	else
	{
		for(i=0;i<size32;i++)
			*(dest32++) = *(src32++);

		switch( (count-(size32<<2)) )
		{
		case 3:	*(dest32++) = *(src32++);
		case 2:	*(dest32++) = *(src32++);
		case 1:	*(dest32++) = *(src32++);
		}
	}

}  /* MoveMemory */
#endif  // _BUFFER_H_
