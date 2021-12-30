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


class REGCODE
{
public:
  REGCODE() ;
  ~REGCODE() {} ;
  BOOL TestRegCode(LPTSTR lpszName,LPTSTR lpszCode,BOOL *fPro=FALSE);
  BOOL SetRegCode(LPTSTR lpszName,LPTSTR lpszCode,BOOL *fPro=FALSE);
  LPCTSTR GetNameCode();
  LPCTSTR GetCrcCode();
  BOOL    IsRegistered(BOOL * fPro=FALSE);
private:
  BOOL fRegistered;
  BOOL fRegisteredPro;
  TCHAR szName[MAX_PATH+1];
  TCHAR szRegCode[MAX_PATH+1];
};
