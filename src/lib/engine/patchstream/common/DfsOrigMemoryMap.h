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

/* DfsOrigMemoryMap.h */
/* define macro to get access on the original version of a file */

#ifndef DFS_ORIG_MEMORY_MAP
#define DFS_ORIG_MEMORY_MAP

#if (!(defined(MEMORY_MAP_FULLFILE))) && ((defined(WIN64) || defined(_WIN64)))
#define MEMORY_MAP_FULLFILE
#endif


#ifdef MEMORY_MAP_FULLFILE
typedef struct
{
  dfvoidp   pData;
  dfuLong64 size;
} ORIGDATA;
typedef ORIGDATA *ORIGDATAPTR;


#define GetOrigDataPtrpDataBySizeView(porgdata,beginPosNeed,sizeNeed) ((porgdata)->pData)
#define GetOrigDataPtrpDataByEndView(porgdata,beginPosNeed,LimitPosNeed) ((porgdata)->pData)

#define NeedManualSetDataPtrDirty(porgdata) (FALSE)

#define SetDataPtrDirtyByPosSize(porgdata,beginPosNeed,sizeNeed) { }

#define FillOrigDataForFullMemoryOrg(porgdata,pDataMem,sizeMem) \
     { ((porgdata)->pData) = (pDataMem); ((porgdata)->size) = (sizeMem); }

#define IsFullOrigDataMapped(porgdata) (TRUE)
#define GetMaxOrigDataExigibleSizeView(porgdata) ((dfuIntPtr)((porgdata)->size))
#define GetMaxOrigDataExigibleSizeViewMask(porgdata) ((dfuIntPtr)-1)
#else

typedef dfvoidp(* tAdaptDataMapView)(void* pOrgData,dfuLong64 dfCurrentViewBegin,dfuLong64 dfCurrentViewEndOrSize,BOOL fBySize,BOOL*pfDoneOk);
typedef void(* tSetDirtyMapView)(void* pOrgData,dfuLong64 dfCurrentViewBegin,dfuLong64 dfSize);

typedef struct
{
  dfvoidp   pCurrentView;
  dfuLong64 size;
  dfuLong64 dfCurrentViewBegin;
  dfuLong64 dfCurrentViewLimitEnd;
  dfuIntPtr dfMaxOrigDataExigibleSizeView;
  dfuIntPtr dfMaxOrigDataExigibleSizeViewMask;
//  dfuLong64 dfCurrentViewSize;
  tAdaptDataMapView fncAdaptDataMapView;
  tSetDirtyMapView fncSetDirtyMapView;
  dfvoidp   pInternalfncAdaptData;
} ORIGDATA;
typedef ORIGDATA *ORIGDATAPTR;

#define GetOrigDataPtrpDataBySizeView(porgdata,beginPosNeed,sizeNeed) \
    ((((porgdata)->fncAdaptDataMapView) == NULL) ? ((porgdata)->pCurrentView) :  \
      ((((porgdata)->dfCurrentViewBegin) <= (beginPosNeed)) && (((porgdata)->dfCurrentViewLimitEnd) >= ((beginPosNeed)+(sizeNeed)))) ? \
             ((porgdata)->pCurrentView) : \
          ((*((porgdata)->fncAdaptDataMapView))(porgdata,beginPosNeed,sizeNeed,TRUE,NULL)))

#define GetOrigDataPtrpDataByEndView(porgdata,beginPosNeed,LimitPosNeed) \
    ((((porgdata)->fncAdaptDataMapView) == NULL) ? ((porgdata)->pCurrentView) :  \
      ((((porgdata)->dfCurrentViewBegin) <= (beginPosNeed)) && (((porgdata)->dfCurrentViewLimitEnd) >= (LimitPosNeed))) ? \
             ((porgdata)->pCurrentView) : \
          ((*((porgdata)->fncAdaptDataMapView))((porgdata),(beginPosNeed),(LimitPosNeed),FALSE,NULL)))


#define FillOrigDataForFullMemoryOrg(porgdata,pDataMem,sizeMem) \
     { ((porgdata)->pCurrentView) = (pDataMem); \
       ((porgdata)->dfCurrentViewLimitEnd) = (sizeMem); \
       ((porgdata)->size) = (sizeMem); \
       ((porgdata)->dfMaxOrigDataExigibleSizeView) = ((dfuIntPtr)(sizeMem)); \
       ((porgdata)->fncAdaptDataMapView) = NULL; \
       ((porgdata)->pInternalfncAdaptData) = NULL; \
       ((porgdata)->dfCurrentViewBegin) = 0; \
       ((porgdata)->dfMaxOrigDataExigibleSizeViewMask) = ((dfuIntPtr)-1);\
     }

#define NeedManualSetDataPtrDirty(porgdata) \
	((((porgdata)->fncSetDirtyMapView) != NULL) ? TRUE : FALSE)

#define SetDataPtrDirtyByPosSize(porgdata,beginPosNeed,sizeNeed) \
 { \
   if (((porgdata)->fncSetDirtyMapView) != NULL) \
     { \
	    ((*((porgdata)->fncSetDirtyMapView))((porgdata),(beginPosNeed),(sizeNeed))); \
   	 } \
   \
   \
 }

#define IsFullOrigDataMapped(porgdata) (((porgdata)->fncAdaptDataMapView) == NULL)
#define GetMaxOrigDataExigibleSizeView(porgdata) ((dfuIntPtr)((porgdata)->dfMaxOrigDataExigibleSizeView))
#define GetMaxOrigDataExigibleSizeViewMask(porgdata) ((porgdata)->dfMaxOrigDataExigibleSizeViewMask)

#endif

#define GetOrigDataPtrpData(porgdata) (GetOrigDataPtrpDataBySizeView((porgdata),0,(porgdata)->size))
#define GetMaxOrigDataSize(porgdata) ((dfuIntPtr)((porgdata)->size))
#endif
