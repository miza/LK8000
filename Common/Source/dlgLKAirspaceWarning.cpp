/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "StdAfx.h"
#include <aygshell.h>

#include "InfoBoxLayout.h"

#include "externs.h"
#include "Units.h"
#include "MapWindow.h"

#ifdef LKAIRSPACE

#include "dlgTools.h"
#include "criticalsection.h"
#include "LKAirspace.h"

extern HWND   hWndMainWindow;
extern HWND   hWndMapWindow;
CAirspace airspace_copy;
AirspaceWarningMessage msg;
int timer_counter;

WndForm *dlg=NULL;

void dlgLKAirspaceFill();

static void OnAckForTimeClicked(WindowControl * Sender)
{
  (void)Sender;
  if (dlg == NULL) return;
  if (msg.originator == NULL) return;
  CAirspaceManager::Instance().AirspaceSetAckLevel(*msg.originator, msg.warnlevel);
  dlg->SetModalResult(mrOK);
}

static void OnCloseClicked(WindowControl * Sender)
{
  (void)Sender;
  if (dlg==NULL) return;
  dlg->SetModalResult(mrOK);
}

static int OnTimer(WindowControl * Sender){
  (void)Sender;
  
  // Timer events comes at 500ms, we need every second
  static bool timer_divider = false;
  timer_divider = !timer_divider;
  if (timer_divider) return 0;
  
  // Auto close dialog after some time
  if (!(--timer_counter)) {
	dlg->SetModalResult(mrOK);
	return 0;
  }
  
  //Get a new copy with current values from airspacemanager
  if (msg.originator == NULL) return 0;
  airspace_copy = CAirspaceManager::Instance().GetAirspaceCopy(msg.originator);
  dlgLKAirspaceFill();
  return(0);
}

static int OnKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam)
{
  (void)lParam;
	switch(wParam){
    case VK_RETURN:
      OnAckForTimeClicked(Sender);
      return(0);
    case VK_ESCAPE:
      OnCloseClicked(Sender);
      return(0);
#ifdef GNAV
    case VK_APP1:
    case '6':
      OnAckClicked(Sender);
      return(0);
#endif
  }

  return(1);
  
}



static CallBackTableEntry_t CallBackTable[]={
  DeclareCallBackEntry(OnAckForTimeClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};


void dlgLKAirspaceFill()
{
  if (msg.warnlevel != airspace_copy.WarningLevel()) {
	// we can automatically close the dialog when the warning level changes, probably new msg waiting in the queue
	dlg->SetModalResult(mrOK);
  }
  
	//Fill up dialog data
	WndProperty* wp;	
	WndButton* wb;	
	
	wp = (WndProperty*)dlg->FindByName(TEXT("prpReason"));
	if (wp) {
	  switch (msg.event) {
		default:
		  wp->SetText(TEXT("Unknown"));
		  break;
		  
		case aweNone:
  		  wp->SetText(TEXT("None"));
		  break;

		case aweMovingInsideFly:
			wp->SetText(TEXT("aweMovingInsideFly"));
		  break;
		
		case awePredictedLeavingFly:
			wp->SetText(TEXT("awePredictedLeavingFly"));
		  break;
		
		case aweNearOutsideFly:
			wp->SetText(TEXT("aweNearOutsideFly"));
		  break;
		  
		case aweLeavingFly:
			wp->SetText(TEXT("aweLeavingFly"));
		  break;

		case awePredictedEnteringFly:
			wp->SetText(TEXT("awePredictedEnteringFly"));
		  break;
		  
		case aweEnteringFly:
			wp->SetText(TEXT("aweEnteringFly"));
		  break;
		  
		case aweMovingOutsideFly:
			wp->SetText(TEXT("aweMovingOutsideFly"));
		  break;
		  
				
		// Events for NON-FLY zones
		case aweMovingOutsideNonfly:
			wp->SetText(TEXT("aweMovingOutsideNonfly"));
		  break;
		  
		case awePredictedEnteringNonfly:
			wp->SetText(TEXT("awePredictedEnteringNonfly"));
		  break;

		case aweNearInsideNonfly:
			wp->SetText(TEXT("aweNearInsideNonfly"));
		  break;

		case aweEnteringNonfly:
			wp->SetText(TEXT("aweEnteringNonfly"));
		  break;

		case aweMovingInsideNonfly:
			wp->SetText(TEXT("aweMovingInsideNonfly"));
		  break;

		case aweLeavingNonFly:
			wp->SetText(TEXT("aweLeavingNonFly"));
		  break;

	  }//sw
	  wp->RefreshDisplay();
	}

	wp = (WndProperty*)dlg->FindByName(TEXT("prpState"));
	if (wp) {
	  switch (airspace_copy.WarningLevel()) {
		default:
		  wp->SetText(TEXT("Unknown"));
		  break;
		  
		case awNone:
  		  wp->SetText(TEXT("None"));
		  break;

		case awYellow:
			wp->SetText(TEXT("awYellow (YELLOW)"));
		  break;
		
		case awRed:
			wp->SetText(TEXT("awRed (RED)"));
		  break;
	  }//sw
	  wp->RefreshDisplay();
	}
	  
	wp = (WndProperty*)dlg->FindByName(TEXT("prpName"));
	if (wp) {
	  wp->SetText(airspace_copy.Name());
	  wp->RefreshDisplay();
	}	

	int hdist;
	int vdist;
	int bearing;
	bool inside;
	TCHAR stmp[21];
	
	// Unfortunatelly virtual methods don't work on copied instances
	// we have to ask airspacemanager to perform the required calculations
	//inside = airspace_copy.CalculateDistance(&hdist, &bearing, &vdist);
	//inside = CAirspaceManager::Instance().AirspaceCalculateDistance(msg.originator, &hdist, &bearing, &vdist);
	bool distances_ready = airspace_copy.GetDistanceInfo(inside, hdist, bearing, vdist);

	wp = (WndProperty*)dlg->FindByName(TEXT("prpHDist"));
	if (wp) {
	  TCHAR stmp2[40];
	  if (distances_ready) {
		Units::FormatUserDistance((double)abs(hdist),stmp, 10);
		if (hdist<0) {
		  wsprintf(stmp2,TEXT("%s to leave"), stmp);
		} else {
		  wsprintf(stmp2,TEXT("%s to enter"), stmp);
		}
	  } else {
		// no distance info calculated
		wsprintf(stmp2,TEXT("Not calculated"));
	  }
	  wp->SetText(stmp2);
	  wp->RefreshDisplay();
	}	

	wp = (WndProperty*)dlg->FindByName(TEXT("prpVDist"));
	if (wp) {
		TCHAR stmp2[40];
	  if (distances_ready) {
		Units::FormatUserAltitude((double)abs(vdist),stmp, 10);
		if (vdist<0) {
		  wsprintf(stmp2,TEXT("below %s"), stmp);
		} else {
		  wsprintf(stmp2,TEXT("above %s"), stmp);
		}
	  } else {
		// no distance info calculated
		wsprintf(stmp2,TEXT("Not calculated"));
	  }
	  wp->SetText(stmp2);
	  wp->RefreshDisplay();
	}	

	wp = (WndProperty*)dlg->FindByName(TEXT("prpTopAlt"));
	if (wp) {
	  TCHAR stmp2[40];
	  CAirspaceManager::Instance().GetAirspaceAltText(stmp2, 40, airspace_copy.Top());
	  wp->SetText(stmp2);
	  wp->RefreshDisplay();
	}	

	wp = (WndProperty*)dlg->FindByName(TEXT("prpBaseAlt"));
	if (wp) {
	  TCHAR stmp2[40];
	  CAirspaceManager::Instance().GetAirspaceAltText(stmp2, 40, airspace_copy.Base());
	  wp->SetText(stmp2);
	  wp->RefreshDisplay();
	}	

	wb = (WndButton*)dlg->FindByName(TEXT("cmdClose"));
	if (wb) {
	  TCHAR stmp2[40];
	  wsprintf(stmp2,TEXT("%s (%d)"), gettext(TEXT("_@M186_")), timer_counter);
	  wb->SetCaption(stmp2);
	}	

}

// Called periodically to show new airspace warning messages to user
void ShowAirspaceWarningsToUser()
{
  if (msg.originator != NULL) return;		// Dialog already open

  bool there_is_message = CAirspaceManager::Instance().PopWarningMessage(&msg);
  if (!there_is_message) return;		// no message to display

  airspace_copy = CAirspaceManager::Instance().GetAirspaceCopy(msg.originator);

  bool ackdialog_required = false;
  TCHAR msgbuf[128];

  // which message we need to show?
  switch (msg.event) {
	default:
	  DoStatusMessage(TEXT("Unknown airspace warning message"));
	  break;	//Unknown msg type

	case aweNone:
	  break;
	  
	case awePredictedLeavingFly:
	case aweNearOutsideFly:
	case aweLeavingFly:
	case awePredictedEnteringNonfly:
	case aweNearInsideNonfly:
	case aweEnteringNonfly:
	case aweMovingInsideNonfly:
	  ackdialog_required = true;
	  break;
	  
	case aweEnteringFly:
	  wsprintf(msgbuf, TEXT("Entering %s"), airspace_copy.Name());
	  DoStatusMessage(msgbuf);
	  break;

	case aweLeavingNonFly:
	  wsprintf(msgbuf, TEXT("Leaving %s"), airspace_copy.Name());
	  DoStatusMessage(msgbuf);
	  break;
	  
  }

  // show dialog to user if needed
  if (ackdialog_required && (airspace_copy.WarningLevel() == msg.warnlevel)) {
	if (!InfoBoxLayout::landscape)
	  dlg = dlgLoadFromXML(CallBackTable, NULL, hWndMainWindow, TEXT("IDR_XML_LKAIRSPACEWARNING_L"));
	else
	  dlg = dlgLoadFromXML(CallBackTable, NULL, hWndMainWindow, TEXT("IDR_XML_LKAIRSPACEWARNING"));

	if (dlg==NULL) {
	  StartupStore(_T("------ LKAirspaceWarning setup FAILED!%s"),NEWLINE); //@ 101027
	  return;
	}
	
	dlg->SetKeyDownNotify(OnKeyDown);
    dlg->SetTimerNotify(OnTimer);
	timer_counter = 10;					// Auto closing dialog in x secs

	WndButton *wb = (WndButton*)dlg->FindByName(TEXT("cmdAckForTime"));
	if (wb) {
	  TCHAR stmp2[40];
	  wsprintf(stmp2,TEXT("%s (%ds)"), gettext(TEXT("_@M46_")), AcknowledgementTime);
	  wb->SetCaption(stmp2);
	}	

	dlgLKAirspaceFill();

	#ifndef DISABLEAUDIO
	if (EnableSoundModes) LKSound(_T("LK_AIRSPACE.WAV")); // 100819
	#endif

	dlg->ShowModal();

	delete dlg;
	dlg = NULL;
  }
  
  msg.originator = NULL;
  return;
}


#endif //LKAIRSPACE
