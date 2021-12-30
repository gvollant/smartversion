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

/* LruMenu.h */

#define NBLRUMENUSHOW   6
#define NBLRUMENU       9
#define IDLRUENT        8000
#define MAXSIZELRUITEM  (128+MAX_PATH)

class LRUMENU
{
public:
LRUMENU (WORD wNbLruShowInit=NBLRUMENUSHOW,
            WORD wNbLruMenuInit=NBLRUMENU,WORD wMaxSizeLruItemInit=MAXSIZELRUITEM);
~LRUMENU (void);
void SetNbLruShow(WORD wNbLruShowInit);
BOOL SetMenuItem(WORD wItem,LPSTR lpItem);
BOOL GetMenuItem(WORD wItem,BOOL fIDMBased,LPSTR lpItem,UINT uiSize);
BOOL DelMenuItem(WORD wItem,BOOL fIDMBased);
void AddNewItem(LPSTR lpItem);
void CleanMenu(HMENU hMenu);
void PlaceMenuLRUItem(HMENU hMenu,UINT uiItem);
void SetIdLruEntry(WORD wSet) { wIdLruEntry = wSet ; }
private:
WORD wNbItemFill;
WORD wNbLruShow;
WORD wNbLruMenu;
WORD wMaxSizeLruItem;
WORD wIdLruEntry;
//char szFNLRU[NBLRUMENU][MAXSIZELRUITEM];
LPSTR lpLRU;
} ;
typedef LRUMENU FAR * PLRUMENU;
