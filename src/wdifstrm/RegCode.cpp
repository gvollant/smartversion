/*
  Smartversion
  Copyright (c) Gilles Vollant, 2002-2022

  https://github.com/gvollant/smartversion
  https://www.smartversion.com/
  https://www.winimage.com/

 This source code is licensed under MIT licence.


  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/


#include <windows.h>
#include "miscutil.h"
#include "RegCode.h"

void MajAndUnspaceString(LPSTR lpDest,LPSTR lpSrc)
{
  while ((*lpSrc) == ' ') lpSrc++;
  lstrcpy(lpDest,lpSrc);
  AnsiUpper(lpDest);
  RemoveSpace(lpDest);
}


DWORD CalculateMyCrc(LPSTR lpStrToCalc)
{
char szStr[256];
DWORD dwCrc =0x47694c;
DWORD dwFact ;
int i,iln;
  MajAndUnspaceString(szStr,lpStrToCalc);
  iln = lstrlen(szStr);
  for (i=0;i<iln;i++)
    {
      if ((i % 14) == 0) dwFact = 39;
      dwCrc += dwFact * ((BYTE)szStr[i]) ;
      if (((i+3) % 14) != 0) dwFact *= 3;
                        else dwFact *= 7;
    }
  return dwCrc;
}

LPSTR MakeCrcStringFromCrc(LPSTR lpStrOrg,DWORD dwCrc)
{
char szCod[16];
LPSTR lpParcCod = szCod;
LPSTR lpStr = lpStrOrg;
char c;

  wsprintf(szCod,"%lX",dwCrc);
  while ((c=*lpParcCod) != '\0')
    {
      if (c == '8') c = 'B' ;
        else if (c == 'B') c = '8';
      *lpStr = c;
      lpStr++;
      lpParcCod++;
    }
  *lpStr = '\0';
  return lpStrOrg;
}

BOOL CheckCrcFromString(LPSTR lpReg,LPSTR lpTxtCrc,LPBOOL lpbProf)
{
DWORD dwCrcWanted;
char  szCrc[256];
char  szCrcWanted[256];
  if (lpbProf!=NULL)
      *lpbProf=FALSE;
  MajAndUnspaceString(szCrc,lpTxtCrc);
  dwCrcWanted = CalculateMyCrc(lpReg);

  if (lpbProf!=NULL)
      *lpbProf=FALSE;


/***/


  // SmartVersion 1.0
  if (lstrcmp(MakeCrcStringFromCrc(szCrcWanted,dwCrcWanted+0x12091999),szCrc)
           == 0)
    return TRUE;
  // SmartVersion 1.0 Pro. Example : "xp","24D8844"
  if ((lstrcmp(MakeCrcStringFromCrc(szCrcWanted,dwCrcWanted+0x31121999),szCrc)  == 0)||
      (lstrcmp(MakeCrcStringFromCrc(szCrcWanted,dwCrcWanted+0x02062000),szCrc)  == 0))
  {
        if (lpbProf!=NULL)
            *lpbProf=TRUE;
        return TRUE;
  }

  // SmartVersion 2.0
  if (lstrcmp(MakeCrcStringFromCrc(szCrcWanted,dwCrcWanted+0x13062004),szCrc)
           == 0)
    return TRUE;
  // SmartVersion 2.0 Pro. Example : "xp","24D8844"
  if ((lstrcmp(MakeCrcStringFromCrc(szCrcWanted,dwCrcWanted+0x21032004),szCrc)  == 0)||
      (lstrcmp(MakeCrcStringFromCrc(szCrcWanted,dwCrcWanted+0x28032004),szCrc)  == 0))
  {
	    if (lpbProf!=NULL)
			*lpbProf=TRUE;
		return TRUE;
  }

  // SmartVersion 2.0a
  if (lstrcmp(MakeCrcStringFromCrc(szCrcWanted,dwCrcWanted+0x05052005),szCrc)
           == 0)
    return TRUE;
  // SmartVersion 2.0a Pro. Example : "xp","24D8844"
  if ((lstrcmp(MakeCrcStringFromCrc(szCrcWanted,dwCrcWanted+0x29052005),szCrc)  == 0)||
      (lstrcmp(MakeCrcStringFromCrc(szCrcWanted,dwCrcWanted+0x01122004),szCrc)  == 0))
  {
	    if (lpbProf!=NULL)
			*lpbProf=TRUE;
		return TRUE;
  }

  return FALSE;
}

REGCODE::REGCODE()
{
    fRegistered = fRegisteredPro = FALSE;
    lstrcpy(szName,"");
    lstrcpy(szRegCode,"");
}

BOOL REGCODE::TestRegCode(LPTSTR lpszName,LPTSTR lpszCode,BOOL *fPro)
{
    return CheckCrcFromString(lpszName,lpszCode,fPro);
}

BOOL REGCODE::SetRegCode(LPTSTR lpszName,LPTSTR lpszCode,BOOL *fPro)
{
    fRegistered = TestRegCode(lpszName,lpszCode,&fRegisteredPro);
    if (fPro != NULL)
        *fPro = fRegisteredPro;
    lstrcpy(szName,fRegistered ? lpszName : "");
    lstrcpy(szRegCode,fRegistered ? lpszCode : "");
    return fRegistered;
}

LPCTSTR REGCODE::GetNameCode()
{
  return szName;
}

LPCTSTR REGCODE::GetCrcCode()
{
  return szRegCode;
}

BOOL REGCODE::IsRegistered(BOOL * fPro)
{
    if (fPro != NULL)
        *fPro = fRegisteredPro;
    return fRegistered;
}