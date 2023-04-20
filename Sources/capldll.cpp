/*----------------------------------------------------------------------------
|
| File Name: capldll.cpp
|
|            Example of a capl DLL implementation module and using CAPLLbacks.
|-----------------------------------------------------------------------------
|               A U T H O R   I D E N T I T Y
|-----------------------------------------------------------------------------
|   Author             Initials
|   ------             --------
|   Thomas  Riegraf    Ri              Vector Informatik GmbH
|   Hans    Quecke     Qu              Vector Informatik GmbH
|-----------------------------------------------------------------------------
|               R E V I S I O N   H I S T O R Y
|-----------------------------------------------------------------------------
| Date         Ver  Author  Description
| ----------   ---  ------  --------------------------------------------------
| 2003-10-07   1.0  As      Created
| 2007-03-26   1.1  Ej      Export of the DLL function table as variable
|                           Use of CAPL_DLL_INFO3
|                           Support of long name CAPL function calls
|-----------------------------------------------------------------------------
|               C O P Y R I G H T
|-----------------------------------------------------------------------------
| Copyright (c) 1994 - 2003 by Vector Informatik GmbH.  All rights reserved.
 ----------------------------------------------------------------------------*/


#define USECDLL_FEATURE
#define _BUILDNODELAYERDLL

#pragma warning( disable : 4786 )

#include "..\Includes\cdll.h"
#include "..\Includes\via.h"
#include "..\Includes\via_CDLL.h"

#include <stdio.h>
#include <stdlib.h>
#include <map>

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLEN 512
#define SD_SEND 1


class CaplInstanceData;
typedef std::map<uint32, CaplInstanceData*> VCaplMap;
typedef std::map<uint32, VIACapl*> VServiceMap;


// ============================================================================
// global variables
// ============================================================================

static unsigned long data = 0;
static char dlldata[100];

char        gModuleName[_MAX_FNAME];  // filename of this  dll
HINSTANCE   gModuleHandle;            // windows instance handle of this DLL
VCaplMap    gCaplMap;
VServiceMap gServiceMap;
SOCKET ConnectSocket = INVALID_SOCKET;
struct sockaddr_in clientService;
int dllVersion = 1;

// ============================================================================
// VIARegisterCDLL
// ============================================================================

VIACLIENT(void) VIARegisterCDLL (VIACapl* service)
{
  uint32    handle;
  VIAResult result;

  if (service==NULL)
  {
    return;
  }

  result = service->GetCaplHandle(&handle);
  if(result!=kVIA_OK)
  {
    return;
  }

  // appInit (internal) resp. "DllInit" (CAPL code) has to follow
  gServiceMap[handle] = service;
}

/**
* Return DLL version
*
* @param void
* @return int ( version )
*/
int caplGetDLLVersion(void) {
    return dllVersion;
}

/**
* Connect to socket 
*
* @param port - port number to which client aim to connect
* @return int - 1 (successfully connected) x - error
*/
int socketConnect(int port) {
    //----------------------
    // Declare and initialize variables.
    int error = 0; 
    int iResult;
    WSADATA wsaData;

    //----------------------
    // Initialize Winsock
    
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        return 1;
    }
    //----------------------
    // Create a SOCKET for connecting to server
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        WSACleanup();
        return WSAGetLastError();
    }
    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr("127.0.0.1"); // 127.0.0.1 <--> LOCALHOST
    clientService.sin_port = htons(port); // PORTUL !!!

    //----------------------
    // Connect to server.
    iResult = connect(ConnectSocket, (SOCKADDR*)&clientService, sizeof(clientService));
    if (iResult == SOCKET_ERROR) {
        error = WSAGetLastError();
        closesocket(ConnectSocket);
        WSACleanup();
        return error;
    }
    return 1;
}

/**
* Send message via socket
*
* @param message - message vector
* @return int - x (number of bytes sent) x - error, -1 - error
*/
int socketSend(char* message) {
    int iResult;
    int error;
    //----------------------
    // Send buffer
    iResult = send(ConnectSocket, message, (int)strlen(message), 0);
    if (iResult == SOCKET_ERROR) {
        error = WSAGetLastError();
        closesocket(ConnectSocket);
        WSACleanup();
        return -1;
    }

    return iResult;
}


/**
* shutdown the connection 
*
* @param void
* @return int - 1 - successfully executed, x - error
*/
int socketShutDown(void) {
    // shutdown the connection 
    int iResult = 0;
    int error = 0;
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        error = WSAGetLastError();
        closesocket(ConnectSocket);
        WSACleanup();
        return error;
    }

    return 1;
}

/**
* Close socket
*
* @param void
* @return int - 1 - successfully executed, x - error
*/
int socketClose(void) {
    int iResult;
    int error = 0;

    iResult = closesocket(ConnectSocket);
    if (iResult == SOCKET_ERROR) {
        error = WSAGetLastError();
        WSACleanup();
        return error;
    }
    
    WSACleanup();
    return 1;
}

// ============================================================================
// CAPL_DLL_INFO_LIST : list of exported functions
//   The first field is predefined and mustn't be changed!
//   The list has to end with a {0,0} entry!
// New struct supporting function names with up to 50 characters
// ============================================================================
CAPL_DLL_INFO4 table[] = {
{CDLL_VERSION_NAME, (CAPL_FARCALL)CDLL_VERSION, "", "", CAPL_DLL_CDECL, 0xabcd, CDLL_EXPORT },
    {"caplGetDLLVersion",  (CAPL_FARCALL)caplGetDLLVersion, "CAPL_DLL","Return DLL version",'L', 0, "", "", {""}},
    {"caplSocketShutdown",  (CAPL_FARCALL)socketShutDown, "CAPL_DLL","Shutdown socket",'L', 0, "", "", {""}},
    {"caplSocketClose",  (CAPL_FARCALL)socketClose, "CAPL_DLL","Close socket",'L', 0, "", "", {""}},
    {"caplSocketConnect",  (CAPL_FARCALL)socketConnect, "CAPL_DLL","Connect to socket",'L', 1, "L", "", {"port"}},
    {"caplSocketSend",  (CAPL_FARCALL)socketSend, "CAPL_DLL","Send message to socket",'L', 1, "C", "\001", {"message"}},
  // {"dllInit",           (CAPL_FARCALL)appInit,          "CAPL_DLL","This function will initialize all callback functions in the CAPLDLL",'V', 1, "D", "", {"handle"}},
  // {"dllEnd",            (CAPL_FARCALL)appEnd,           "CAPL_DLL","This function will release the CAPL function handle in the CAPLDLL",'V', 1, "D", "", {"handle"}},
  // {"dllSetValue",       (CAPL_FARCALL)appSetValue,      "CAPL_DLL","This function will call a callback functions",'L', 2, "DL", "", {"handle","x"}},
  // {"dllReadData",       (CAPL_FARCALL)appReadData,      "CAPL_DLL","This function will call a callback functions",'L', 2, "DL", "", {"handle","x"}},
  // {"dllPut",            (CAPL_FARCALL)appPut,           "CAPL_DLL","This function will save data from CAPL to DLL memory",'V', 1, "D", "", {"x"}},
  // {"dllGet",            (CAPL_FARCALL)appGet,           "CAPL_DLL","This function will read data from DLL memory to CAPL",'D', 0, "", "", {""}},
  // {"dllVoid",           (CAPL_FARCALL)voidFct,          "CAPL_DLL","This function will overwrite DLL memory from CAPL without parameter",'V', 0, "", "", {""}},
  // {"dllPutDataOnePar",  (CAPL_FARCALL)appPutDataOnePar, "CAPL_DLL","This function will put data from CAPL array to DLL",'V', 1, "B", "\001", {"datablock"}},
  // {"dllGetDataOnePar",  (CAPL_FARCALL)appGetDataOnePar, "CAPL_DLL","This function will get data from DLL into CAPL memory",'V', 1, "B", "\001", {"datablock"}},
  // {"dllPutDataTwoPars", (CAPL_FARCALL)appPutDataTwoPars,"CAPL_DLL","This function will put two datas from CAPL array to DLL",'V', 2, "DB", "\000\001", {"noOfBytes","datablock"}},// number of pars in octal format
  // {"dllGetDataTwoPars", (CAPL_FARCALL)appGetDataTwoPars,"CAPL_DLL","This function will get two datas from DLL into CAPL memory",'V', 2, "DB", "\000\001", {"noOfBytes","datablock"}},
  // {"dllAdd",            (CAPL_FARCALL)appAdd,           "CAPL_DLL","This function will add two values. The return value is the result",'L', 2, "LL", "", {"x","y"}},
  // {"dllSubtract",       (CAPL_FARCALL)appSubtract,      "CAPL_DLL","This function will substract two values. The return value is the result",'L', 2, "LL", "", {"x","y"}},
  // {"dllSupportLongFunctionNamesWithUpTo50Characters",   (CAPL_FARCALL)appLongFuncName,      "CAPL_DLL","This function shows the support of long function names",'D', 0, "", "", {""}},
  // {"dllAdd63Parameters", (CAPL_FARCALL)appAddValues63,  "CAPL_DLL", "This function will add 63 values. The return value is the result",'L', 63, "LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL",  "", {"val01","val02","val03","val04","val05","val06","val07","val08","val09","val10","val11","val12","val13","val14","val15","val16","val17","val18","val19","val20","val21","val22","val23","val24","val25","val26","val27","val28","val29","val30","val31","val32","val33","val34","val35","val36","val37","val38","val39","val40","val41","val42","val43","val44","val45","val46","val47","val48","val49","val50","val51","val52","val53","val54","val55","val56","val57","val58","val59","val60","val61","val62","val63"}},
  // {"dllAdd64Parameters", (CAPL_FARCALL)appAddValues64,  "CAPL_DLL", "This function will add 64 values. The return value is the result",'L', 64, {SixtyFourLongPars},                                                "", {"val01","val02","val03","val04","val05","val06","val07","val08","val09","val10","val11","val12","val13","val14","val15","val16","val17","val18","val19","val20","val21","val22","val23","val24","val25","val26","val27","val28","val29","val30","val31","val32","val33","val34","val35","val36","val37","val38","val39","val40","val41","val42","val43","val44","val45","val46","val47","val48","val49","val50","val51","val52","val53","val54","val55","val56","val57","val58","val59","val60","val61","val62","val63","val64"}},

{0, 0}
};
CAPLEXPORT CAPL_DLL_INFO4 far * caplDllTable4 = table;
