#ifndef _VLIV_HANDLERS_H
#define _VLIV_HANDLERS_H

#include <vliv.h>

FormatHandlerPtr GetHandlerFromFile(const TCHAR* name);
void RegisterFormatHandlers(ImagePtr img);
void UnRegisterFormatHandlers();
const TCHAR* GetCustomFilter();

void RegisterFormatHandler(ACCEPTF accept,
			   GETDESCRIPTIONF getdescription,
			   GETEXTENSIONF getextension,
			   OPENF open,
			   SETDIRECTORYF setdirectory,
			   LOADTILEF loadtile,
			   CLOSEF close);

#endif
