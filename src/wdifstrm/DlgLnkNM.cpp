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
#include <windowsx.h>
#include <commctrl.h>

#include "global.h"
#include "ltoolsCPP.h"
#include "resource.h"
#include "DlgLnkNM.h"
#include "miscutil.h"
#include "ExtInfo.h"
#include "GuiItem.h"

#include "../../lib/engine/svfile/common/DirSet.h"

#include "RegCode.h"
#include "DoExtracting.h"
#include "uiMain.h"

class DialogObject
{
friend BOOL CALLBACK DialogObjectDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
public:
    DialogObject()  { } ;
    virtual ~DialogObject() { } ;
    virtual BOOL DlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam) { return FALSE; } ;
protected:
    HWND hDlg;
};


BOOL CALLBACK DialogObjectDlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    BOOL fRet=FALSE;
    DialogObject* pDialogObject=NULL;
    if (message==WM_INITDIALOG)
    {
        pDialogObject= (DialogObject*)lParam;
        pDialogObject->hDlg = hDlg;
        MySetWindowLongPtr(hDlg,DWLP_USER,lParam);
    }
    else
        pDialogObject = (DialogObject*)MyGetWindowLongPtr(hDlg,DWLP_USER);

    if (pDialogObject!=NULL)
      fRet=pDialogObject->DlgProc(hDlg,message,wParam,lParam);

    if (message==WM_DESTROY)
    {
        MySetWindowLongPtr(hDlg,DWLP_USER,0);
    }
    return fRet;
}

/***************************************************************************/
/***************************************************************************/

typedef struct
{
    dfwcharpc* dfOldNameArray;
    dfuLong32 dfNbOldName;
    dfwcharpc* dfNewNameArray;
    dfuLong32 dfNbNewName;

    dfuLong32 * pNewNameToOldNameLink;
} EDITLINKRENAMEDPARAM;

typedef struct
{
    dfwcharpc dfOldName;
    dfuLong32 dfLinkedNewItem;
    BOOL fLinkToIdenticalName;
} OLDITEM;

typedef struct
{
    dfwcharpc dfNewName;
    dfuLong32 dfLinkedOldItem;
    BOOL fLinkToIdenticalName;
} NEWITEM;

typedef struct
{
    OLDITEM *pOldItem;
    NEWITEM *pNewItem;

    dfwcharpc* dfOldNameArray;
    dfuLong32 dfNbOldName;
    dfwcharpc* dfNewNameArray;
    dfuLong32 dfNbNewName;

} OLDTONEW_FILENAME_ASSOCIATION;

class EditLinkRenamedObject:public DialogObject
{
public:
    EditLinkRenamedObject(const EDITLINKRENAMEDPARAM & EditLinkRenamedParamSet);
    ~EditLinkRenamedObject();
    BOOL DlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam);
private:
    EDITLINKRENAMEDPARAM EditLinkRenamedParam;
    OLDTONEW_FILENAME_ASSOCIATION OldToNewFileAssociation;

    BOOL LinkOldAndNewFileName(dfuLong32 dfOldPos,dfuLong32 dfNewPos);
    BOOL DeleteLinkOldAndNewFileName(dfuLong32 dfOldPos,dfuLong32 &dfNewPos);
    BOOL CheckButtonEnablingStattus();


    RESIZABLEDLGHELP ResizableDlgHelp;
    HWND hLbOldName ;
    HWND hLbNewName ;
    HWND hLbLinkName;
};

/********************************/


typedef struct
{
    dfuLong32 dfPosition;
    dfwcharpc dfOldName;
} OLDNAMEWITHPOSITION;


long DFSCALLBACK fncCompareOldNameWithPosition(const void *lpElem1, const void *lpElem2)
{
  const OLDNAMEWITHPOSITION *pfi1 = (const OLDNAMEWITHPOSITION *) lpElem1;
  const OLDNAMEWITHPOSITION *pfi2 = (const OLDNAMEWITHPOSITION *) lpElem2;
  return dfUnicodeStrcmpi(pfi1->dfOldName, pfi2->dfOldName);
}

BOOL DFSCALLBACK fncDestructorOldNameWithPosition(const void *lpElem)
{
  return TRUE;
}

BOOL BuildLinkFileIdentical(dfwcharpc* dfOldNameArray,dfuLong32 dfNbOldName,
                            dfwcharpc* dfNewNameArray,dfuLong32 dfNbNewName,
                            dfuLong32 *  pNewNameToOldNameLink,
                            dfuLong32 *  pdfNbFileIdentical)
{
    dfuLong32 i;
    dfuLong32 dfNbFileIdentical=0;
    STATIC_ARRAY saOldName;

    saOldName.InitStaticArray(sizeof(OLDNAMEWITHPOSITION), dfNbOldName+1);
    saOldName.SetFuncCompareDataSA(fncCompareOldNameWithPosition);
    saOldName.SetFuncDestructorSA(fncDestructorOldNameWithPosition);

    for (i=0;i<dfNbOldName;i++)
    {
        OLDNAMEWITHPOSITION OldNameWithPosition;
        OldNameWithPosition.dfPosition=i;
        OldNameWithPosition.dfOldName=*(dfOldNameArray+i);

        if (!saOldName.FindSameElemPosSA(&OldNameWithPosition, NULL))
          saOldName.InsertSortedSA(&OldNameWithPosition);

    }

    for (i=0;i<dfNbNewName;i++)
    {
        OLDNAMEWITHPOSITION owItemToSearch;
        dfuLong32 dwPos=0;
        owItemToSearch.dfPosition = NO_LINK_VALUE;
        owItemToSearch.dfOldName=*(dfNewNameArray+i);


        if (saOldName.FindSameElemPosSA(&owItemToSearch, &dwPos))
        {
                const OLDNAMEWITHPOSITION *pOldNameWithPosition =
                    (const OLDNAMEWITHPOSITION *) saOldName.GetElemPtrSA(dwPos);
                *(pNewNameToOldNameLink+i) = pOldNameWithPosition->dfPosition;
                dfNbFileIdentical++;
        }
        else
        *(pNewNameToOldNameLink+i)=NO_LINK_VALUE;
    }

    if (pdfNbFileIdentical!=NULL)
        *pdfNbFileIdentical = dfNbFileIdentical;
    return TRUE;
}

/********************************/

BOOL SetupSameNameLink(OLDTONEW_FILENAME_ASSOCIATION& OldToNewFileAssociation)
{
    dfuLong32 i,j;
    BOOL fRet = TRUE;
    dfuLong32 dfNbFileIdentical;
    dfuLong32* pOldNameToNewNameLinkFileNameIdentical;

    OLDITEM *pOldItem=OldToNewFileAssociation.pOldItem ;
    NEWITEM *pNewItem=OldToNewFileAssociation.pNewItem ;

    for (i=0;i<OldToNewFileAssociation.dfNbOldName;i++)
    {
        (pOldItem+i)->dfLinkedNewItem = NO_LINK_VALUE;
        (pOldItem+i)->fLinkToIdenticalName = FALSE;
    }

    for (i=0;i<OldToNewFileAssociation.dfNbNewName;i++)
    {
        (pNewItem+i)->dfLinkedOldItem = NO_LINK_VALUE;
        (pNewItem+i)->fLinkToIdenticalName = FALSE;
    }


    pOldNameToNewNameLinkFileNameIdentical = (dfuLong32*)DfsMalloc(
                         sizeof(dfuLong32)*(OldToNewFileAssociation.dfNbNewName+1));
    if (pOldNameToNewNameLinkFileNameIdentical == NULL)
        return TRUE;


    if (!BuildLinkFileIdentical(OldToNewFileAssociation.dfOldNameArray,
                                OldToNewFileAssociation.dfNbOldName,
                                OldToNewFileAssociation.dfNewNameArray,
                                OldToNewFileAssociation.dfNbNewName,
                                pOldNameToNewNameLinkFileNameIdentical,
                                &dfNbFileIdentical))
           fRet=FALSE;
    else
    for (j=0;j<OldToNewFileAssociation.dfNbNewName;j++)
    {
        i = *(pOldNameToNewNameLinkFileNameIdentical+j);
        if (i!=NO_LINK_VALUE)
            {
                ((pNewItem+j)->dfLinkedOldItem) = i;
                ((pNewItem+j)->fLinkToIdenticalName) = TRUE;
                ((pOldItem+i)->dfLinkedNewItem) = j;
                ((pOldItem+i)->fLinkToIdenticalName) = TRUE;


                #ifdef _DEBUG
                if (dfUnicodeStrcmpi(*(OldToNewFileAssociation.dfOldNameArray+i),
                                     *(OldToNewFileAssociation.dfNewNameArray+j))!=0)
                {
                    TCHAR szTxt[(MAX_PATH*3)];
                    wsprintf(szTxt,"'%ws' cmp '%ws' = %d\n",
                                     *(OldToNewFileAssociation.dfOldNameArray+i),
                                     *(OldToNewFileAssociation.dfNewNameArray+j),
                                     dfUnicodeStrcmpi(*(OldToNewFileAssociation.dfOldNameArray+i),
                                     *(OldToNewFileAssociation.dfNewNameArray+j)));

                    MessageBox(0,szTxt,"big er",MB_OK);
                }
                #endif
            }
    }
    DfsFree(pOldNameToNewNameLinkFileNameIdentical);

    return fRet;
}

EditLinkRenamedObject::EditLinkRenamedObject(const EDITLINKRENAMEDPARAM & EditLinkRenamedParamSet)
{
    dfuLong32 i;

    EditLinkRenamedParam=EditLinkRenamedParamSet;

    OldToNewFileAssociation.dfNbOldName = EditLinkRenamedParam.dfNbOldName;
    OldToNewFileAssociation.dfNbNewName = EditLinkRenamedParam.dfNbNewName;

    OldToNewFileAssociation.pOldItem = (OLDITEM *)DfsMalloc(sizeof(OLDITEM) * (EditLinkRenamedParam.dfNbOldName+1));
    OldToNewFileAssociation.pNewItem = (NEWITEM *)DfsMalloc(sizeof(NEWITEM) * (EditLinkRenamedParam.dfNbNewName+1));


    for (i=0;i<OldToNewFileAssociation.dfNbOldName;i++)
        (OldToNewFileAssociation.pOldItem+i)->dfOldName = *(EditLinkRenamedParam.dfOldNameArray+i);

    for (i=0;i<OldToNewFileAssociation.dfNbNewName;i++)
        (OldToNewFileAssociation.pNewItem+i)->dfNewName = *(EditLinkRenamedParam.dfNewNameArray+i);

    OldToNewFileAssociation.dfOldNameArray = EditLinkRenamedParam.dfOldNameArray;
    OldToNewFileAssociation.dfNewNameArray = EditLinkRenamedParam.dfNewNameArray;

    SetupSameNameLink(OldToNewFileAssociation);
}

EditLinkRenamedObject::~EditLinkRenamedObject()
{
    DfsFree(OldToNewFileAssociation.pOldItem);
    DfsFree(OldToNewFileAssociation.pNewItem);
}

/*****************************************************/

BOOL DeleteListBoxItemFromItemData(HWND hLB,dfuLong32 dfItem)
{
    dfuLong32 i;
    dfuLong32 dfLbNbItem = ListBox_GetCount(hLB);
    for (i=0;i<dfLbNbItem;i++)
    {
        if (((dfuLong32)ListBox_GetItemData(hLB,i))==dfItem)
        {
            ListBox_DeleteString(hLB,i);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL ListBoxAddOldFileName(HWND hDlg,const OLDTONEW_FILENAME_ASSOCIATION& OldToNewFileAssociation,dfuLong32 i)
{
    HWND hLbOldName = GetDlgItem(hDlg,IDC_LB_LISTOLDNAME);

    TCHAR szStr[MAX_PATH+1];
    wsprintf(szStr,"%ws",(OldToNewFileAssociation.pOldItem+i)->dfOldName);
    int item=(int)ListBox_AddString(hLbOldName, szStr);
    ListBox_SetItemData(hLbOldName, item, i);
    return TRUE;
}

BOOL ListBoxRemoveOldFileName(HWND hDlg,dfuLong32 i)
{
    HWND hLbOldName = GetDlgItem(hDlg,IDC_LB_LISTOLDNAME);
    return DeleteListBoxItemFromItemData(hLbOldName,i);
}

BOOL ListBoxAddNewFileName(HWND hDlg,const OLDTONEW_FILENAME_ASSOCIATION& OldToNewFileAssociation,dfuLong32 i)
{
    HWND hLbNewName = GetDlgItem(hDlg,IDC_LB_LISTNEWNAME);
    TCHAR szStr[MAX_PATH+1];

    wsprintf(szStr,"%ws",(OldToNewFileAssociation.pNewItem+i)->dfNewName);
    int item=(int)ListBox_AddString(hLbNewName, szStr);
    ListBox_SetItemData(hLbNewName, item, i);
    return TRUE;
}

BOOL ListBoxRemoveNewFileName(HWND hDlg,dfuLong32 i)
{
    HWND hLbNewName = GetDlgItem(hDlg,IDC_LB_LISTNEWNAME);
    return DeleteListBoxItemFromItemData(hLbNewName,i);
}

BOOL ListBoxAddLinkFileName(HWND hDlg,const OLDTONEW_FILENAME_ASSOCIATION& OldToNewFileAssociation,dfuLong32 i)
{
    TCHAR szStr[(MAX_PATH*2)+0x10];
    HWND hLbLinkName = GetDlgItem(hDlg,IDC_LB_LISTLINKNAME);

    wsprintf(szStr,"%ws  -->  %ws ",(OldToNewFileAssociation.pOldItem+i)->dfOldName,
                            (OldToNewFileAssociation.pNewItem+
                            ((OldToNewFileAssociation.pOldItem+i)->dfLinkedNewItem))->dfNewName);
    int item=(int)ListBox_AddString(hLbLinkName, szStr);
    ListBox_SetItemData(hLbLinkName, item, i);
    return TRUE;
}


BOOL ListBoxRemoveLinkFileName(HWND hDlg,dfuLong32 i)
{
    HWND hLbLinkName = GetDlgItem(hDlg,IDC_LB_LISTLINKNAME);
    return DeleteListBoxItemFromItemData(hLbLinkName,i);
}

BOOL FillListBoxed(HWND hDlg,const OLDTONEW_FILENAME_ASSOCIATION& OldToNewFileAssociation,LPCTSTR lpszExtension)
{
    dfuLong32 i;
    HWND hLbOldName = GetDlgItem(hDlg,IDC_LB_LISTOLDNAME);
    HWND hLbNewName = GetDlgItem(hDlg,IDC_LB_LISTNEWNAME);
    HWND hLbLinkName = GetDlgItem(hDlg,IDC_LB_LISTLINKNAME);

    ListBox_ResetContent(hLbOldName);
    ListBox_ResetContent(hLbNewName);
    ListBox_ResetContent(hLbLinkName);

    for (i=0;i<OldToNewFileAssociation.dfNbOldName;i++)
        if ((OldToNewFileAssociation.pOldItem+i)->dfLinkedNewItem == NO_LINK_VALUE)
            ListBoxAddOldFileName(hDlg,OldToNewFileAssociation,i);

    for (i=0;i<OldToNewFileAssociation.dfNbNewName;i++)
        if ((OldToNewFileAssociation.pNewItem+i)->dfLinkedOldItem == NO_LINK_VALUE)
            ListBoxAddNewFileName(hDlg,OldToNewFileAssociation,i);

    for (i=0;i<OldToNewFileAssociation.dfNbOldName;i++)
        if (((OldToNewFileAssociation.pOldItem+i)->dfLinkedNewItem != NO_LINK_VALUE) &&
            ((!(OldToNewFileAssociation.pOldItem+i)->fLinkToIdenticalName)))
            ListBoxAddLinkFileName(hDlg,OldToNewFileAssociation,i);

    return TRUE;
}

BOOL EditLinkRenamedObject::LinkOldAndNewFileName(dfuLong32 dfOldPos,dfuLong32 dfNewPos)
{
    OLDITEM *pOldItem=OldToNewFileAssociation.pOldItem ;
    NEWITEM *pNewItem=OldToNewFileAssociation.pNewItem ;

    if ((((OldToNewFileAssociation.pOldItem+dfOldPos)->dfLinkedNewItem) != NO_LINK_VALUE) ||
        (((OldToNewFileAssociation.pNewItem+dfNewPos)->dfLinkedOldItem) != NO_LINK_VALUE))
        return FALSE;

    (pOldItem+dfOldPos)->dfLinkedNewItem = dfNewPos;
    (pOldItem+dfOldPos)->fLinkToIdenticalName = FALSE;

    (pNewItem+dfNewPos)->dfLinkedOldItem = dfOldPos;
    (pNewItem+dfNewPos)->fLinkToIdenticalName = FALSE;
    return TRUE;
}

BOOL EditLinkRenamedObject::DeleteLinkOldAndNewFileName(dfuLong32 dfOldPos,dfuLong32 &dfNewPos)
{
    OLDITEM *pOldItem=OldToNewFileAssociation.pOldItem ;
    NEWITEM *pNewItem=OldToNewFileAssociation.pNewItem ;

    dfNewPos = (OldToNewFileAssociation.pOldItem+dfOldPos)->dfLinkedNewItem;
    if (dfNewPos == NO_LINK_VALUE)
        return FALSE;

    (pOldItem+dfOldPos)->dfLinkedNewItem = NO_LINK_VALUE;
    (pOldItem+dfOldPos)->fLinkToIdenticalName = FALSE;

    (pNewItem+dfNewPos)->dfLinkedOldItem = NO_LINK_VALUE;
    (pNewItem+dfNewPos)->fLinkToIdenticalName = FALSE;
    return TRUE;
}

BOOL EditLinkRenamedObject::CheckButtonEnablingStattus()
{
    BOOL fEnableAdd,fEnableRemove;

    fEnableAdd = (ListBox_GetCurSel(hLbOldName) != LB_ERR) &&
                 (ListBox_GetCount(hLbOldName)>0) &&
                 (ListBox_GetCurSel(hLbNewName) != LB_ERR) &&
                 (ListBox_GetCount(hLbNewName)>0);

    /*
    fEnableRemove = (ListBox_GetCurSel(hLbLinkName) != LB_ERR) &&
                    (ListBox_GetCount(hLbLinkName)>0);
                    */
    fEnableRemove = (ListBox_GetSelCount(hLbLinkName)>0);

    EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_ADDLINK),fEnableAdd);
    EnableWindow(GetDlgItem(hDlg,IDC_BUTTON_REMOVELINK),fEnableRemove);

    return TRUE;
}

BOOL EditLinkRenamedObject::DlgProc(HWND hDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
    switch (message)
    {
      case WM_INITDIALOG:
          {
              FillListBoxed(hDlg,OldToNewFileAssociation,NULL);
              ShowWindow(GetDlgItem(hDlg,IDC_COMBO_LISTTYPE_STATICTEXT),SW_HIDE);
              ShowWindow(GetDlgItem(hDlg,IDC_COMBO_LISTTYPE),SW_HIDE);

              hLbOldName = GetDlgItem(hDlg,IDC_LB_LISTOLDNAME);
              hLbNewName = GetDlgItem(hDlg,IDC_LB_LISTNEWNAME);
              hLbLinkName = GetDlgItem(hDlg,IDC_LB_LISTLINKNAME);

              {
                  ResizableDlgHelp.Init(hDlg);
                  ResizableDlgHelp.InitRatio(0x1000);

                  const ITEMCTLINFO ItemCtlInfo[] =
                  {
                    {IDC_STATIC_EXPLAIN_LINK,0x0000,0x0000,0x01000,0x0000,FALSE},
                    {IDC_COMBO_LISTTYPE_STATICTEXT,0x0000,0x0000,0x0000,0x0000,FALSE},
                    {IDC_COMBO_LISTTYPE,0x0000,0x0000,0x0000,0x0000,FALSE},

                    {IDC_STATIC_FILE_LATEST_RECORDED,0x0000,0x0000,0x0800,0x000,TRUE},
                    {IDC_LB_LISTOLDNAME,0x0000,0x0000,0x0800,0x0a00,FALSE},

                    {IDC_BUTTON_REMOVELINK,0x0800,0x0800,0x0000,0x0000,TRUE},
                    {IDC_BUTTON_ADDLINK,0x0800,0x0800,0x0000,0x0000,TRUE},

                    {IDC_STATIC_FILE_CUR_ADDED,0x0800,0x0000,0x0800,0x000,TRUE},
                    {IDC_LB_LISTNEWNAME,0x0800,0x0000,0x0800,0x0a00,FALSE},

                    {IDC_LB_LISTLINKNAME,0x0000,0x0a00,0x1000,0x600,TRUE},
                    {IDOK,0x1000,0x1000,0x0000,0x000,TRUE},
                    {IDCANCEL,0x1000,0x1000,0x0000,0x000,TRUE},
                    {IDC_GETHELP,0x1000,0x1000,0x0000,0x000,TRUE},
                    {0,0,0,0,0,FALSE}};

                    ResizableDlgHelp.InitCtlList(ItemCtlInfo);
              }
              return TRUE;
          }

      case WM_GETMINMAXINFO:
      {
          SIZE sz;
          MINMAXINFO* pMinMaxInfo = (MINMAXINFO*)lParam;
          ResizableDlgHelp.GetOriginalDialogSize(&sz);
          pMinMaxInfo->ptMinTrackSize.x=sz.cx;
          pMinMaxInfo->ptMinTrackSize.y=sz.cy;
          return 0;
      }

      case WM_SIZE:
          ResizableDlgHelp.OnResize();
          return TRUE;

      case WM_COMMAND:
          {
              WORD wId = GET_WM_COMMAND_ID(wParam,lParam);

              switch (wId)
              {
              case IDOK:
              {
                  dfuLong32 i;
                  for (i=0;i<OldToNewFileAssociation.dfNbNewName;i++)
                  {
                      *(EditLinkRenamedParam.pNewNameToOldNameLink+i)=
                          (OldToNewFileAssociation.pNewItem+i)->dfLinkedOldItem ;
                  }

                  EndDialog(hDlg,TRUE);
                  break;
              }

              case IDCANCEL:
                  EndDialog(hDlg,FALSE);
                  break;

              case IDC_BUTTON_ADDLINK:
                  if ((ListBox_GetCurSel(hLbOldName) != LB_ERR) &&
                      (ListBox_GetCount(hLbOldName)>0) &&
                      (ListBox_GetCurSel(hLbNewName) != LB_ERR) &&
                      (ListBox_GetCount(hLbNewName)>0))
                  {
                      dfuLong32 dfOldPos = (dfuLong32)ListBox_GetItemData(hLbOldName,ListBox_GetCurSel(hLbOldName));
                      dfuLong32 dfNewPos = (dfuLong32)ListBox_GetItemData(hLbNewName,ListBox_GetCurSel(hLbNewName));
                      if (LinkOldAndNewFileName(dfOldPos,dfNewPos))
                      {
                          ListBox_DeleteString(hLbOldName,ListBox_GetCurSel(hLbOldName));
                          ListBox_DeleteString(hLbNewName,ListBox_GetCurSel(hLbNewName));
                          ListBoxAddLinkFileName(hDlg,OldToNewFileAssociation,dfOldPos);
                      }
                  }
                  break;

              case IDC_BUTTON_REMOVELINK:
                  /*
                  if ((ListBox_GetCurSel(hLbLinkName) != LB_ERR) &&
                      (ListBox_GetCount(hLbLinkName)>0))
                  {
                      dfuLong32 dfNewPos;
                      dfuLong32 dfOldPos = ListBox_GetItemData(hLbLinkName,ListBox_GetCurSel(hLbLinkName));
                      if (DeleteLinkOldAndNewFileName(dfOldPos,dfNewPos))
                      {
                          ListBox_DeleteString(hLbLinkName,ListBox_GetCurSel(hLbLinkName));
                          ListBoxAddOldFileName(hDlg,OldToNewFileAssociation,dfOldPos);
                          ListBoxAddNewFileName(hDlg,OldToNewFileAssociation,dfNewPos);
                      }
                  }*/
                  if (ListBox_GetSelCount(hLbLinkName)>0)
                  {
                      dfuLong32 dfNewPos;
                      dfuLong32 dfOldPos = (dfuLong32)ListBox_GetItemData(hLbLinkName,ListBox_GetCurSel(hLbLinkName));
                      if (DeleteLinkOldAndNewFileName(dfOldPos,dfNewPos))
                      {
                          ListBox_DeleteString(hLbLinkName,ListBox_GetCurSel(hLbLinkName));
                          ListBoxAddOldFileName(hDlg,OldToNewFileAssociation,dfOldPos);
                          ListBoxAddNewFileName(hDlg,OldToNewFileAssociation,dfNewPos);
                      }
                  }
                  break;
              };

              CheckButtonEnablingStattus();
          }
          return TRUE;
    }
    return FALSE;
}

BOOL EditLinkRenamedFile(HWND hWnd,
                         dfwcharpc* dfOldNameArray,dfuLong32 dfNbOldName,
                         dfwcharpc* dfNewNameArray,dfuLong32 dfNbNewName,
                         dfuLong32 * pNewNameToOldNameLink)
{
    BOOL fRet=FALSE;

    EDITLINKRENAMEDPARAM EditLinkRenamedParam;

    EditLinkRenamedParam.dfOldNameArray = dfOldNameArray;
    EditLinkRenamedParam.dfNbOldName = dfNbOldName;
    EditLinkRenamedParam.dfNewNameArray = dfNewNameArray;
    EditLinkRenamedParam.dfNbNewName = dfNbNewName;
    EditLinkRenamedParam.pNewNameToOldNameLink = pNewNameToOldNameLink;

    EditLinkRenamedObject* pEditLinkRenamedObject = new EditLinkRenamedObject (EditLinkRenamedParam);

    fRet=(BOOL)DialogBoxParam(ghInstRes, MAKEINTRESOURCE(IDD_LINKRENAMED), hWnd, /*guiItem.GetHwndMain() */
                   (DLGPROC)DialogObjectDlgProc,(LPARAM)pEditLinkRenamedObject);

    delete(pEditLinkRenamedObject);

    return fRet;
}


