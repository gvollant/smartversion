
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <mbstring.h>

#include "mdump.h"

#define _tcsrchr(a,b) strrchr((a),(b))
#define _tcscat(a,b) strcat((a),(b))
#define _tcscpy(a,b) strcpy((a),(b))
/*
#define _tcsrchr(a,b) _mbsrchr((a),(b))
#define _tcscat(a,b) _mbscat((a),(b))
#define _tcscpy(a,b) _mbscpy((a),(b))
*/
LPCSTR MiniDumper::m_szAppName;



#if _MSC_VER < 1300
/*
#define DECLSPEC_DEPRECATED
// VC6: change this path to your Platform SDK headers
#include "M:\\dev7\\vs\\devtools\\common\\win32sdk\\include\\dbghelp.h"         // must be XP version of file
*/
MiniDumper::MiniDumper( LPCSTR szAppName )
{
}

LONG MiniDumper::TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
    return 0;
}

void MiniDumper::SetHandler()
{
}

void MiniDumper::RemoveHandler()
{
}

#else
// VC7: ships with updated headers
#include <dbghelp.h>

// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
                                    CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                    CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                    CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
                                    );


MiniDumper::MiniDumper( LPCSTR szAppName )
{
    // if this assert fires then you have two instances of MiniDumper
    // which is not allowed
    //assert( m_szAppName==NULL );

    m_szAppName = szAppName ? (szAppName) : "Application";

    LPTOP_LEVEL_EXCEPTION_FILTER lpPrevious = ::SetUnhandledExceptionFilter( TopLevelFilter );
    if (lpPrevious != NULL)
    {
    }
}

void MiniDumper::SetHandler()
{
    ::SetUnhandledExceptionFilter( TopLevelFilter );
}

void MiniDumper::RemoveHandler()
{
    ::SetUnhandledExceptionFilter( NULL );
}

LONG MiniDumper::TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
    LONG retval = EXCEPTION_CONTINUE_SEARCH;
    HWND hParent = NULL;                        // find a better value for your app

    // firstly see if dbghelp.dll is around and has the function we need
    // look next to the EXE first, as the one in System32 might be old
    // (e.g. Windows 2000)
    HMODULE hDll = NULL;
    char szDbgHelpPath[_MAX_PATH];

    if (GetModuleFileName( NULL, szDbgHelpPath, _MAX_PATH ))
    {
        char *pSlash = _tcsrchr( szDbgHelpPath, '\\' );
        if (pSlash)
        {
            _tcscpy( pSlash+1, "DBGHELP.DLL" );
            hDll = ::LoadLibrary( szDbgHelpPath );
        }
    }

    if (hDll==NULL)
    {
        // load any version we can
        hDll = ::LoadLibrary( "DBGHELP.DLL" );
    }

    LPCTSTR szResult = NULL;

    if (hDll)
    {
        MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
        if (pDump)
        {
            char szDumpPath[_MAX_PATH];
            char szScratch [_MAX_PATH+0x80];

            // work out a good place for the dump file
            if (!GetTempPath( _MAX_PATH, szDumpPath ))
                _tcscpy( szDumpPath, "c:\\temp\\" );

            _tcscat( szDumpPath, m_szAppName );
            _tcscat( szDumpPath, ".dmp" );

            // ask the user if they want to save a dump file
            if (::MessageBox( NULL, "Something bad happened in your program, would you like to save a diagnostic file?", m_szAppName, MB_YESNO )==IDYES)
            {
                // create the file
                HANDLE hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL, NULL );

                if (hFile!=INVALID_HANDLE_VALUE)
                {
                    _MINIDUMP_EXCEPTION_INFORMATION ExInfo;

                    ExInfo.ThreadId = ::GetCurrentThreadId();
                    ExInfo.ExceptionPointers = pExceptionInfo;
                    ExInfo.ClientPointers = NULL;

                    // write the dump
                    BOOL bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL );
                    if (bOK)
                    {
                        sprintf( szScratch, "Saved dump file to '%s'\nSend it by email to info@winimage.com", szDumpPath );
                        szResult = szScratch;
                        retval = EXCEPTION_EXECUTE_HANDLER;
                    }
                    else
                    {
                        sprintf( szScratch, "Failed to save dump file to '%s' (error %d)", szDumpPath, GetLastError() );
                        szResult = szScratch;
                    }
                    ::CloseHandle(hFile);
                }
                else
                {
                    sprintf( szScratch, "Failed to create dump file '%s' (error %d)", szDumpPath, GetLastError() );
                    szResult = szScratch;
                }
            }
        }
        else
        {
            szResult = "DBGHELP.DLL too old";
        }
    }
    else
    {
        szResult = "DBGHELP.DLL not found";
    }

    if (szResult)
        ::MessageBox( NULL, szResult, m_szAppName, MB_OK );

    return retval;
}
#endif


static MiniDumper* pMiniDumper = NULL;

void InstallDumperHandler(LPCSTR szAppName)
{
    if (pMiniDumper == NULL)
        pMiniDumper = new MiniDumper(szAppName);
    else
        pMiniDumper->SetHandler();
}

void UnInstallDumperHandler()
{
    if (pMiniDumper != NULL)
    {
        pMiniDumper -> RemoveHandler();
        delete(pMiniDumper);
    }
    pMiniDumper = NULL;
}
