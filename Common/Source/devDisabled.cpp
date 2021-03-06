/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "StdAfx.h"


#include "externs.h"
#include "Utils.h"
#include "Parser.h"
#include "Port.h"

#include "devDisabled.h"

#include "utils/heapcheck.h"


BOOL disInstall(PDeviceDescriptor_t d){
  // _tcscpy(d->Name, gettext(_T("_@M1600_"))); causing problems with language change
  _tcscpy(d->Name, _T("DISABLED"));
  d->ParseNMEA = NULL;
  d->PutMacCready = NULL;
  d->PutBugs = NULL;
  d->PutBallast = NULL;
  d->Open = NULL;
  d->Close = NULL;
  d->Init = NULL;
  d->LinkTimeout = NULL;
  d->Declare = NULL;
  d->IsLogger = NULL;
  d->IsGPSSource = NULL;
  d->IsBaroSource = NULL;
  d->PutQNH = NULL;
  d->OnSysTicker = NULL;

  return(TRUE);

}


BOOL disRegister(void){
  return(devRegister(
    // gettext(_T("_@M1600_")), 
    _T("DISABLED"),
    (1l << dfGPS)
   ,
    disInstall
  ));
}

