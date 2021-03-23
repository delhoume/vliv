
#include <rawinput.h>

PRAWINPUTDEVICELIST g_pRawInputDeviceList = 0;
PRAWINPUTDEVICE     g_pRawInputDevices = 0;
int                 g_nUsagePage1Usage8Devices;

void InitRawInputDevice(HWND hwnd) {
  UINT nDevices;
  if (GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) == 0) {
    g_pRawInputDeviceList = (PRAWINPUTDEVICELIST)MYALLOC(sizeof(RAWINPUTDEVICELIST) * nDevices);
    if (GetRawInputDeviceList(g_pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0) {
      UINT i;
      g_pRawInputDevices = (PRAWINPUTDEVICE)MYALLOC(nDevices * sizeof(RAWINPUTDEVICE));
      g_nUsagePage1Usage8Devices = 0;
      for(i = 0; i < nDevices; i++) {
	if (g_pRawInputDeviceList[i].dwType == RIM_TYPEHID) { // only HID
	  RID_DEVICE_INFO dinfo;
	  UINT sizeofdinfo = sizeof(dinfo);
	  dinfo.cbSize = sizeofdinfo;
	  if (GetRawInputDeviceInfo(g_pRawInputDeviceList[i].hDevice,
				    RIDI_DEVICEINFO, &dinfo, &sizeofdinfo ) >= 0) {
	    if (dinfo.dwType == RIM_TYPEHID) {
	      RID_DEVICE_INFO_HID *phidInfo = &dinfo.hid;
	      // "VID = 0x%x\n", phidInfo->dwVendorId);
	      // "PID = 0x%x\n", phidInfo->dwProductId);
	      // "Version = 0x%x\n", phidInfo->dwVersionNumber);
	      // "UsagePage = 0x%x\n", phidInfo->usUsagePage);
	      // "Usage = 0x%x\n", phidInfo->usUsage);
	      if (phidInfo->usUsagePage == 1 && phidInfo->usUsage == 8) {
		g_pRawInputDevices[g_nUsagePage1Usage8Devices].usUsagePage = phidInfo->usUsagePage;
		g_pRawInputDevices[g_nUsagePage1Usage8Devices].usUsage     = phidInfo->usUsage;
		g_pRawInputDevices[g_nUsagePage1Usage8Devices].dwFlags     = 0;
		g_pRawInputDevices[g_nUsagePage1Usage8Devices].hwndTarget  = hwnd;
		g_nUsagePage1Usage8Devices++;
	      }
	    }
	  }
	}
      }
      if (g_nUsagePage1Usage8Devices > 0) {
	if (RegisterRawInputDevices(g_pRawInputDevices, g_nUsagePage1Usage8Devices, sizeof(RAWINPUTDEVICE) ) == TRUE) {
	  //	  char buffer[100];
	  //	  wsprintf(buffer, "%d", g_nUsagePage1Usage8Devices);
	  //	  MessageBox(0, buffer, buffer, MB_OK);
	}
      }
    }
  }
}

void FreeRawInputDevices() {  
  if (g_pRawInputDeviceList)
    MYFREE(g_pRawInputDeviceList);
  if (g_pRawInputDevices)
    MYFREE(g_pRawInputDevices);
}
