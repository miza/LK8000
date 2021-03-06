/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id: LKMapWindow.cpp,v 1.6 2010/12/11 19:20:55 root Exp root $
*/


#include "StdAfx.h"
#include "compatibility.h"
#include "options.h"
#include "Defines.h"

#include "MapWindow.h"
#ifndef NEW_OLC
#include "OnLineContest.h"
#endif /* NEW_OLC */
#include "Utils.h"
#include "XCSoar.h"
#include "LKUtils.h"
#include "Utils2.h"
#include "Units.h"
#include "Logger.h"
#include "McReady.h"
#include "Airspace.h"
#include "Waypointparser.h"
#include "Dialogs.h"
#include "externs.h"
#include "InputEvents.h"
#include <windows.h>
#include <math.h>

#include <tchar.h>

#include "Task.h"

#include "Terrain.h"
#include "RasterTerrain.h"

#include "GaugeCDI.h"
#include "GaugeFLARM.h"
#include "InfoBoxLayout.h"
#include "LKMapWindow.h"
#if LKOBJ
#include "LKObjects.h"
#endif

#if (WINDOWSPC>0)
#include <wingdi.h>
#endif

#include "utils/heapcheck.h"

#if defined(LKAIRSPACE) || defined(NEW_OLC)
using std::min;
using std::max;
#endif

extern HWND hWndCDIWindow;
extern HFONT MapLabelFont;
extern HFONT  MapWindowBoldFont;

#define ISPARAGLIDER (AircraftCategory == (AircraftCategory_t)umParaglider)
#define INVERTCOLORS  (Appearance.InverseInfoBox)


// This is painting traffic icons on the screen.
void MapWindow::LKDrawFLARMTraffic(HDC hDC, RECT rc, POINT Orig_Aircraft) {

  if (!EnableFLARMMap) return;

  if (!DrawInfo.FLARM_Available) return;

  // init scaled coords for traffic icon
  static bool doinit=true;
  static short scaler[5];
  static short tscaler=0;
  if (doinit) {

	switch (ScreenSize) {
		case ss480x640:
		case ss480x800:
		case ss896x672:
		case ss800x480:
		case ss640x480:
			scaler[0]=-1*(NIBLSCALE(4)-2);
			scaler[1]=NIBLSCALE(5)-2;
			scaler[2]=-1*(NIBLSCALE(6)-2);
			scaler[3]=NIBLSCALE(4)-2;
			scaler[4]=NIBLSCALE(2)-2;
			tscaler=NIBLSCALE(7)-2;
			break;
		case ss240x320:
		case ss272x480:
		case ss320x240:
		case ss480x272:
		case ss720x408:
		case ss480x234:
		case ss400x240:
			scaler[0]=-1*(NIBLSCALE(8)-2);
			scaler[1]=NIBLSCALE(10)-2;
			scaler[2]=-1*(NIBLSCALE(12)-2);
			scaler[3]=NIBLSCALE(8)-2;
			scaler[4]=NIBLSCALE(4)-2;
			tscaler=NIBLSCALE(13)-2;
			break;
		default:
			scaler[0]=-1*NIBLSCALE(4);
			scaler[1]=NIBLSCALE(5);
			scaler[2]=-1*NIBLSCALE(6);
			scaler[3]=NIBLSCALE(4);
			scaler[4]=NIBLSCALE(2);
			tscaler=NIBLSCALE(7);
			break;
	}


	doinit=false;
  }

  HPEN hpold;
  #if LKOBJ
  HPEN thinBlackPen = LKPen_Black_N1;
  #else
  HPEN thinBlackPen = CreatePen(PS_SOLID, NIBLSCALE(1), RGB(0,0,0));
  #endif
  POINT Arrow[5];

  //TCHAR buffer[50]; REMOVE
  TCHAR lbuffer[50];

  hpold = (HPEN)SelectObject(hDC, thinBlackPen);

  int i;
  int painted=0;

//  double dX, dY;

  TextInBoxMode_t displaymode;
  displaymode.AsInt = 0;
  displaymode.AsFlag.NoSetFont = 1;

  double screenrange = GetApproxScreenRange();
  double scalefact = screenrange/6000.0; // FIX 100820

  #if LKOBJ
  HBRUSH redBrush = LKBrush_Red;
  HBRUSH yellowBrush = LKBrush_Yellow;
  HBRUSH greenBrush = LKBrush_Green;
  #else
  HBRUSH redBrush = CreateSolidBrush(RGB_RED);
  HBRUSH yellowBrush = CreateSolidBrush(RGB_YELLOW);
  HBRUSH greenBrush = CreateSolidBrush(RGB_GREEN);
  #endif
  HFONT  oldfont = (HFONT)SelectObject(hDC, LK8MapFont);

  for (i=0,painted=0; i<FLARM_MAX_TRAFFIC; i++) {

	// limit to 10 icons map traffic
	if (painted>=10) {
		i=FLARM_MAX_TRAFFIC;
		continue;
	}

	if ( (DrawInfo.FLARM_Traffic[i].ID !=0) && (DrawInfo.FLARM_Traffic[i].Status != LKT_ZOMBIE) ) {

		painted++;

		double target_lon;
		double target_lat;

		target_lon = DrawInfo.FLARM_Traffic[i].Longitude;
		target_lat = DrawInfo.FLARM_Traffic[i].Latitude;

		if ((EnableFLARMMap==2)&&(scalefact>1.0)) {

			double distance;
			double bearing;

			DistanceBearing(DrawInfo.Latitude, DrawInfo.Longitude, target_lat, target_lon, &distance, &bearing);
			FindLatitudeLongitude(DrawInfo.Latitude, DrawInfo.Longitude, bearing, distance*scalefact, &target_lat, &target_lon);
		}
      
		POINT sc, sc_name, sc_av;
		LatLon2Screen(target_lon, target_lat, sc);

		sc_name = sc;

		sc_name.y -= NIBLSCALE(16);
		sc_av = sc_name;

		wsprintf(lbuffer,_T(""));
		if (DrawInfo.FLARM_Traffic[i].Cn && DrawInfo.FLARM_Traffic[i].Cn[0]!=_T('?')) { // 100322
			_tcscat(lbuffer,DrawInfo.FLARM_Traffic[i].Cn);
		}
		if (DrawInfo.FLARM_Traffic[i].Average30s>=0.1) {
			if (_tcslen(lbuffer) >0)
				_stprintf(lbuffer,_T("%s:%.1f"),lbuffer,LIFTMODIFY*DrawInfo.FLARM_Traffic[i].Average30s);
			else
				_stprintf(lbuffer,_T("%.1f"),LIFTMODIFY*DrawInfo.FLARM_Traffic[i].Average30s);
		}

		displaymode.AsFlag.Border=1;

		if (_tcslen(lbuffer)>0)
		TextInBox(hDC, lbuffer, sc.x+tscaler, sc.y+tscaler, 0, displaymode, false);

		// red circle
		if ((DrawInfo.FLARM_Traffic[i].AlarmLevel>0) && (DrawInfo.FLARM_Traffic[i].AlarmLevel<4)) {
			DrawBitmapIn(hDC, sc, hFLARMTraffic);
		}
#if 0
		Arrow[0].x = -4;
		Arrow[0].y = 5;
		Arrow[1].x = 0;
		Arrow[1].y = -6;
		Arrow[2].x = 4;
		Arrow[2].y = 5;
		Arrow[3].x = 0;
		Arrow[3].y = 2;
		Arrow[4].x = -4;
		Arrow[4].y = 5;
#endif

		Arrow[0].x = scaler[0];
		Arrow[0].y = scaler[1];
		Arrow[1].x = 0;
		Arrow[1].y = scaler[2];
		Arrow[2].x = scaler[3];
		Arrow[2].y = scaler[1];
		Arrow[3].x = 0;
		Arrow[3].y = scaler[4];
		Arrow[4].x = scaler[0];
		Arrow[4].y = scaler[1];

		switch (DrawInfo.FLARM_Traffic[i].Status) { // 100321
			case LKT_GHOST:
				SelectObject(hDC, yellowBrush);
				break;
			case LKT_ZOMBIE:
				SelectObject(hDC, redBrush);
				break;
			default:
				SelectObject(hDC, greenBrush);
				break;
		}

		PolygonRotateShift(Arrow, 5, sc.x, sc.y, DrawInfo.FLARM_Traffic[i].TrackBearing - DisplayAngle);
		Polygon(hDC,Arrow,5);

	}
  }

  SelectObject(hDC, oldfont);
  SelectObject(hDC, hpold);
  #ifndef LKOBJ
  DeleteObject((HPEN)thinBlackPen);
  DeleteObject(greenBrush);
  DeleteObject(yellowBrush);
  DeleteObject(redBrush);
  #endif

}

// MUST be an even number
#define NUMVBRICKS 32
#define BOXTHICK   2	// caution, not used as deemed
#define PIXELSEPARATE 1
#define POSCOLOR 6

// Available only in fullscreen landscape mode
void MapWindow::LKDrawVario(HDC hDC, RECT rc) {

  HPEN		oldPen;
  HBRUSH	oldBrush;

  static RECT	vrc, mrc, hrc, htrc, hbrc;
  static RECT	brc[NUMVBRICKS];
  static HPEN	blackThickPen, whiteThickPen;
  static HPEN	blackThinPen, whiteThinPen;
  static HBRUSH blackBrush, whiteBrush;
  static HBRUSH greenBrush, darkyellowBrush, orangeBrush, redBrush;
  static HBRUSH lakeBrush, blueBrush, indigoBrush;
  static HBRUSH *positiveBrush[NUMVBRICKS/2];
  static HBRUSH *negativeBrush[NUMVBRICKS/2];
  static bool	doinit=true;

  static short startInitCounter=0;
  static bool dogaugeinit=true;

  if (doinit) {

  int boxthick;
  int hpixelseparate;
  int vpixelseparate;

  // A dirty hack for an impossible division solution
  // lowres devices should have 8 bricks, and not 16 as asked by users!
  switch (ScreenSize) {
	case ss320x240:
	case ss400x240:
		hpixelseparate=0;
		vpixelseparate=-1;
		boxthick=0;
		break;
	case ss480x272:
	case ss480x234:
		hpixelseparate=0;
		vpixelseparate=-2;
		boxthick=0;
		break;
	default:
		hpixelseparate=PIXELSEPARATE;
		vpixelseparate=PIXELSEPARATE;
		boxthick=IBLSCALE(BOXTHICK);
		break;
  }

  int variowidth=LKVarioSize;


  #if LKOBJ
  whiteThickPen =  LKPen_White_N2;	// BOXTHICK
  blackThickPen =  LKPen_Black_N2;	// BOXTHICK
  whiteThinPen =   LKPen_White_N0;
  blackThinPen =   LKPen_Black_N0;
  blackBrush = LKBrush_Black;
  whiteBrush = LKBrush_White;
  greenBrush = LKBrush_Green;
  darkyellowBrush = LKBrush_DarkYellow2;
  orangeBrush = LKBrush_Orange;
  redBrush = LKBrush_Red;
  lakeBrush = LKBrush_Lake;
  blueBrush = LKBrush_Blue;
  indigoBrush = LKBrush_Indigo;
  #else
  whiteThickPen =   (HPEN)CreatePen(PS_SOLID,IBLSCALE(BOXTHICK),RGB_WHITE);
  blackThickPen =   (HPEN)CreatePen(PS_SOLID,IBLSCALE(BOXTHICK),RGB_BLACK);
  whiteThinPen =   (HPEN)CreatePen(PS_SOLID,0,RGB_WHITE);
  blackThinPen =   (HPEN)CreatePen(PS_SOLID,0,RGB_BLACK);
  blackBrush = (HBRUSH)CreateSolidBrush(RGB_BLACK);
  whiteBrush = (HBRUSH)CreateSolidBrush(RGB_WHITE);
  greenBrush = (HBRUSH)CreateSolidBrush(RGB_GREEN);
  darkyellowBrush = (HBRUSH)CreateSolidBrush(RGB_DARKYELLOW2);
  orangeBrush = (HBRUSH)CreateSolidBrush(RGB_ORANGE);
  redBrush = (HBRUSH)CreateSolidBrush(RGB_RED);
  lakeBrush = (HBRUSH)CreateSolidBrush(RGB_LAKE);
  blueBrush = (HBRUSH)CreateSolidBrush(RGB_BLUE);
  indigoBrush = (HBRUSH)CreateSolidBrush(RGB_INDIGO);
  #endif

  // set default background in case of missing values
  for (int i=0; i<(NUMVBRICKS/2); i++ )
	positiveBrush[i]= &blackBrush;
  for (int i=0; i<(NUMVBRICKS/2); i++ )
	negativeBrush[i]= &blackBrush;

  positiveBrush[15]=&greenBrush;
  positiveBrush[14]=&greenBrush;
  positiveBrush[13]=&greenBrush;
  positiveBrush[12]=&greenBrush;
  positiveBrush[11]=&darkyellowBrush;
  positiveBrush[10]=&darkyellowBrush;
  positiveBrush[9]=&darkyellowBrush;
  positiveBrush[8]=&darkyellowBrush;
  positiveBrush[7]=&orangeBrush;
  positiveBrush[6]=&orangeBrush;
  positiveBrush[5]=&orangeBrush;
  positiveBrush[4]=&orangeBrush;
  positiveBrush[3]=&redBrush;
  positiveBrush[2]=&redBrush;
  positiveBrush[1]=&redBrush;
  positiveBrush[0]=&redBrush;

  negativeBrush[0]=&lakeBrush;
  negativeBrush[1]=&lakeBrush;
  negativeBrush[2]=&lakeBrush;
  negativeBrush[3]=&lakeBrush;
  negativeBrush[4]=&blueBrush;
  negativeBrush[5]=&blueBrush;
  negativeBrush[6]=&blueBrush;
  negativeBrush[7]=&blueBrush;
  negativeBrush[8]=&indigoBrush;
  negativeBrush[9]=&indigoBrush;
  negativeBrush[10]=&indigoBrush;
  negativeBrush[11]=&indigoBrush;
  negativeBrush[12]=&blackBrush;
  negativeBrush[13]=&blackBrush;
  negativeBrush[14]=&blackBrush;
  negativeBrush[15]=&blackBrush;


  // vario paint area
  vrc.left=rc.left+NIBLSCALE(1);
  vrc.top=rc.top+NIBLSCALE(1);
  vrc.right=vrc.left+variowidth;
  vrc.bottom=rc.bottom - BottomSize - NIBLSCALE(1);;

  // meter area
  mrc.left=vrc.left+boxthick-hpixelseparate;
  mrc.top=vrc.top+boxthick-vpixelseparate;;
  mrc.right=vrc.right-boxthick;
  mrc.bottom=vrc.bottom-boxthick;

  int vmiddle=((mrc.bottom-mrc.top)/2)+mrc.top;

  // half vario separator for positive and negative values
  hrc.top = vrc.top+ vmiddle-NIBLSCALE(1);
  hrc.bottom = vrc.top+ vmiddle+NIBLSCALE(1);
  hrc.left= vrc.left;
  // MUST MATCH MapWindow DrawLook8000 leftmargin!!
  hrc.right=vrc.right+NIBLSCALE(2);

  // half top meter area
  htrc.left=mrc.left;
  htrc.right=mrc.right;
  htrc.top=mrc.top;

  switch (ScreenSize) {
	case ss320x240:
	case ss480x234:
	case ss480x272:
		htrc.bottom=hrc.top -vpixelseparate;
		hbrc.top=hrc.bottom+vpixelseparate;
		break;
	default:
		htrc.bottom=hrc.top -vpixelseparate-1;
		hbrc.top=hrc.bottom+vpixelseparate+1;
		break;
  }

  // half bottom meter area
  hbrc.left=mrc.left;
  hbrc.right=mrc.right;
  hbrc.bottom=mrc.bottom;

  // pixel height of each brick
  int bricksize=(htrc.bottom - htrc.top - ((vpixelseparate) * ((NUMVBRICKS/2)-1)  )) / (NUMVBRICKS/2);
#if (WINDOWSPC>0)
  if (ScreenSize==ss720x408) bricksize=13;
#endif
  if (ScreenSize==ss480x272) bricksize=9;
  if (ScreenSize==ss480x234) bricksize=8;
 
  // Pre-calculate brick positions for half top
  for (int i=0; i<(NUMVBRICKS/2); i++) {
	brc[i].top= htrc.top + (bricksize*i)+(i*(vpixelseparate));
	// make the last one rounded since bricksize could be slighlty smaller due to division round
	if (i==((NUMVBRICKS/2)-1))
		brc[i].bottom= htrc.bottom;
	else
		brc[i].bottom= brc[i].top+bricksize;
	brc[i].left= htrc.left;
	brc[i].right= htrc.right;
  }
  // Pre-calculate brick positions for half bottom
  for (int i=((NUMVBRICKS/2)-1); i>=0; i--) {
	brc[ (NUMVBRICKS/2)+ i].bottom= hbrc.bottom - (bricksize*(  ((NUMVBRICKS/2)-1)-i)  ) - 
		(  (((NUMVBRICKS/2)-1)-i) * vpixelseparate   );
	if ( i == 0 )
		brc[ (NUMVBRICKS/2)+ i].top = hbrc.top;
	else
		brc[ (NUMVBRICKS/2)+ i].top = brc[ (NUMVBRICKS/2)+i].bottom - bricksize;
	brc[ (NUMVBRICKS/2)+ i].left = hbrc.left;
	brc[ (NUMVBRICKS/2)+ i].right = hbrc.right;
  }


	doinit=false;
  } // END of INIT




  // draw external box
  if (BgMapColor>POSCOLOR) 
	oldPen=(HPEN)SelectObject(hDC,whiteThinPen);
  else
	oldPen=(HPEN)SelectObject(hDC,blackThickPen);

  if (LKVarioBar>vBarVarioGR) {
	oldBrush=(HBRUSH)SelectObject(hDC,GetStockObject(NULL_BRUSH));
  } else {
	oldBrush=(HBRUSH)SelectObject(hDC,hInvBackgroundBrush[BgMapColor]);
  }
  Rectangle(hDC,vrc.left, vrc.top, vrc.right, vrc.bottom);


  // draw middle separator for 0 scale indicator
  if (BgMapColor>POSCOLOR) 
  	FillRect(hDC,&hrc, whiteBrush);
  else
  	FillRect(hDC,&hrc, blackBrush);

  if (BgMapColor>POSCOLOR)
	  SelectObject(hDC,whiteThinPen);
  else
	  SelectObject(hDC,blackThinPen);

  double value;

  if (MapWindow::mode.Is(MapWindow::Mode::MODE_CIRCLING) || LKVarioVal==vValVarioVario) {
	if (DrawInfo.VarioAvailable) {
		// UHM. I think we are not painting values correctly for knots &c.
		//value = LIFTMODIFY*DrawInfo.Vario;
		value = DrawInfo.Vario;
	} else {
		value = DerivedDrawInfo.Vario;
	}
  } else {
	switch(LKVarioVal) {
		case vValVarioNetto:
			value = DerivedDrawInfo.NettoVario;
			break;
		case vValVarioSoll:
			double ias;
			if (DrawInfo.AirspeedAvailable && DrawInfo.VarioAvailable)
				ias=DrawInfo.IndicatedAirspeed;
			else
				ias=DerivedDrawInfo.IndicatedAirspeedEstimated;

			value = DerivedDrawInfo.VOpt - ias;
			// m/s 0-nnn autolimit to 20m/s full scale (72kmh diff)
			if (value>20) value=20;
			if (value<-20) value=-20;
			value/=3.3333;	// 0-20  -> 0-6
			value *= -1; // if up, push down
			break;

		default:
			value = DerivedDrawInfo.NettoVario;
			break;
	}
  }


  if (dogaugeinit) {

	// this is causing problems on emulators and condor and most of the times when the gps has no valid date
	// so we don't use seconds, but loop counter
	if ( startInitCounter++ >2) {
		dogaugeinit=false;
	} else {
		short j=NUMVBRICKS/2;
		// Demo show all bricks
		for (int i=0; i<j; i++)  {
			SelectObject(hDC,*positiveBrush[i]);
			Rectangle(hDC,brc[i].left, brc[i].top, brc[i].right, brc[i].bottom);
		}
		for (int i=0; i<j; i++)  {
			SelectObject(hDC,*negativeBrush[i]);
			Rectangle(hDC,brc[i+(NUMVBRICKS/2)].left, brc[i+(NUMVBRICKS/2)].top, brc[i+(NUMVBRICKS/2)].right, brc[i+(NUMVBRICKS/2)].bottom);
		}
		
		SelectObject(hDC,oldPen);
		SelectObject(hDC,oldBrush);
		return;

	}
  }

  short lkvariobar=LKVarioBar;
  if (lkvariobar>vBarVarioGR) lkvariobar-=vBarVarioGR;
  short meter=-1;
  if (value>0) {

	if (value>=0.05) meter=15;
	if (value>=0.25) meter=14;
	if (value>=0.50) meter=13;
	if (value>=0.75) meter=12;
	if (value>=1.00) meter=11;
	if (value>=1.25) meter=10;
	if (value>=1.50) meter=9;
	if (value>=1.75) meter=8;
	if (value>=2.00) meter=7;
	if (value>=2.50) meter=6;
	if (value>=3.00) meter=5;
	if (value>=3.50) meter=4;
	if (value>=4.00) meter=3;
	if (value>=4.50) meter=2;
	if (value>=5.00) meter=1;
	if (value>=6.00) meter=0;

	if (meter>=0) {
		for (short i=15; i>=meter; i--) {
			switch (lkvariobar) {
				case vBarVarioColor:
					SelectObject(hDC,*positiveBrush[i]);
					break;
				case vBarVarioMono:
					if (BgMapColor>POSCOLOR)
						SelectObject(hDC,whiteBrush);
					else
						SelectObject(hDC,blackBrush);
					break;
				case vBarVarioRB:
					SelectObject(hDC,redBrush);
					break;
				case vBarVarioGR:
				default:
					SelectObject(hDC,greenBrush);
					break;
			}
/*
			if (LKVarioBar == vBarVarioColor) 
				SelectObject(hDC,*positiveBrush[i]);
			else {
				if (BgMapColor>POSCOLOR)
					SelectObject(hDC,whiteBrush);
				else
					SelectObject(hDC,blackBrush);
			}
*/
			Rectangle(hDC,brc[i].left, brc[i].top, brc[i].right, brc[i].bottom);
		}
	}
  } else if (value <0) {
	value*=-1;
	if (value>=0.05) meter=0;
	if (value>=0.25) meter=1;
	if (value>=0.50) meter=2;
	if (value>=0.75) meter=3;
	if (value>=1.00) meter=4;
	if (value>=1.25) meter=5;
	if (value>=1.50) meter=6;
	if (value>=1.75) meter=7;
	if (value>=2.00) meter=8;
	if (value>=2.50) meter=9;
	if (value>=3.00) meter=10;
	if (value>=3.50) meter=11;
	if (value>=4.00) meter=12;
	if (value>=4.50) meter=13;
	if (value>=5.00) meter=14;
	if (value>=6.00) meter=15;

	if (meter>=0) {
		for (short i=0; i<=meter; i++) {
			switch (lkvariobar) {
				case vBarVarioColor:
					SelectObject(hDC,*negativeBrush[i]);
					break;
				case vBarVarioMono:
					if (BgMapColor>POSCOLOR)
						SelectObject(hDC,whiteBrush);
					else
						SelectObject(hDC,blackBrush);
					break;
				case vBarVarioRB:
					SelectObject(hDC,blueBrush);
					break;
				case vBarVarioGR:
				default:
					SelectObject(hDC,redBrush);
					break;
			}
/*
			if (LKVarioBar == vBarVarioColor) 
				SelectObject(hDC,*negativeBrush[i]);
			else {
				if (BgMapColor>POSCOLOR)
					SelectObject(hDC,whiteBrush);
				else
					SelectObject(hDC,blackBrush);
			}
*/
			Rectangle(hDC,brc[i+(NUMVBRICKS/2)].left, brc[i+(NUMVBRICKS/2)].top, brc[i+(NUMVBRICKS/2)].right, brc[i+(NUMVBRICKS/2)].bottom);
		}
	}

  }
	
  // cleanup and return 
  SelectObject(hDC,oldPen);
  SelectObject(hDC,oldBrush);
  return;

}


// try not to use colors when over a useless mapscale
double MapWindow::LKDrawTrail( HDC hdc, const POINT Orig, const RECT rc)
{
  int i, snail_index;
  SNAIL_POINT P1;
  #if 100303
  int  nearby;
  bool usecolors=false;
  #endif

  double TrailFirstTime = -1;

  if(!TrailActive)
    return -1;

  #if 0	// 100303
  if ((DisplayMode == dmCircling) != last_circling) {
    need_colour = true;
  }
  last_circling = (DisplayMode == dmCircling);
  #endif

  #if 100303
#ifndef MAP_ZOOM
  if (MapWindow::MapScale <2.34) { // <3km map zoom
#else /* MAP_ZOOM */
  if (MapWindow::zoom.Scale() <2.34) { // <3km map zoom
#endif /* MAP_ZOOM */
	usecolors=true;
  }
  #endif

  // Trail drift calculations

  double traildrift_lat = 0.0;
  double traildrift_lon = 0.0;
  
#ifndef MAP_ZOOM
  if (EnableTrailDrift && (DisplayMode == dmCircling)) {
#else /* MAP_ZOOM */
  if (EnableTrailDrift && MapWindow::mode.Is(MapWindow::Mode::MODE_CIRCLING)) {
#endif /* MAP_ZOOM */
    double tlat1, tlon1;
    
    FindLatitudeLongitude(DrawInfo.Latitude, 
                          DrawInfo.Longitude, 
                          DerivedDrawInfo.WindBearing, 
                          DerivedDrawInfo.WindSpeed,
                          &tlat1, &tlon1);
    traildrift_lat = (DrawInfo.Latitude-tlat1);
    traildrift_lon = (DrawInfo.Longitude-tlon1);
  } else {
    traildrift_lat = 0.0;
    traildrift_lon = 0.0;
  }
  
  // JMW don't draw first bit from home airport

  //  Trail size

  int num_trail_max;
  if (TrailActive!=2) {
	// scan entire trail for sink magnitude
	num_trail_max = TRAILSIZE;
  } else {
	// scan only recently for lift magnitude
	num_trail_max = TRAILSIZE/TRAILSHRINK;
  }
#ifndef MAP_ZOOM
  if ((DisplayMode == dmCircling)) {
#else /* MAP_ZOOM */
  if (MapWindow::mode.Is(MapWindow::Mode::MODE_CIRCLING)) {
#endif /* MAP_ZOOM */
	num_trail_max /= TRAILSHRINK;
  }

  // Snail skipping 

  const int skip_divisor = num_trail_max/5;
  int skip_border = skip_divisor;
  int skip_level= 3; // TODO code: try lower level?

  int snail_offset = TRAILSIZE+iSnailNext-num_trail_max;
  while (snail_offset>= TRAILSIZE) {
    snail_offset -= TRAILSIZE;
  }
  while (snail_offset< 0) {
    snail_offset += TRAILSIZE;
  }
  const int zero_offset = (TRAILSIZE-snail_offset);
  skip_border += zero_offset % skip_level;

  int index_skip = ((int)DrawInfo.Time)%skip_level;

  // TODO code: Divide by time step cruise/circling for zero_offset

  // Keep track of what's drawn

  bool this_visible = true;
  bool last_visible = false;
  POINT point_lastdrawn;
  point_lastdrawn.x = 0;
  point_lastdrawn.y = 0;

  // Average colour display for skipped points
  float vario_av = 0;
  int vario_av_num = 0;

  // Constants for speedups

#ifndef MAP_ZOOM
  const bool display_circling = DisplayMode == dmCircling;
#else /* MAP_ZOOM */
  const bool display_circling = MapWindow::mode.Is(MapWindow::Mode::MODE_CIRCLING);
#endif /* MAP_ZOOM */
  const double display_time = DrawInfo.Time;

  // expand bounds so in strong winds the appropriate snail points are
  // still visible (since they are being tested before drift is applied)
  // this expands them by one minute

  // NOT a good idea, other functions will assume to be within screen boundaries..
  rectObj bounds_thermal = screenbounds_latlon;
  screenbounds_latlon.minx -= fabs(60.0*traildrift_lon);
  screenbounds_latlon.maxx += fabs(60.0*traildrift_lon);
  screenbounds_latlon.miny -= fabs(60.0*traildrift_lat);
  screenbounds_latlon.maxy += fabs(60.0*traildrift_lat);

  const rectObj bounds = bounds_thermal;

  const int deg = DEG_TO_INT(AngleLimit360(DisplayAngle));
  const int cost = ICOSTABLE[deg];
  const int sint = ISINETABLE[deg];
  const int xxs = Orig_Screen.x*1024-512;
  const int yys = Orig_Screen.y*1024+512;
#ifndef MAP_ZOOM
  const double mDrawScale = DrawScale;
#else /* MAP_ZOOM */
  const double mDrawScale = zoom.DrawScale();
#endif /* MAP_ZOOM */
  const double mPanLongitude = PanLongitude;
  const double mPanLatitude = PanLatitude;

  // Main loop

#if 100303
  if (display_circling) {
	nearby=NIBLSCALE(1);
  } else {
#ifndef MAP_ZOOM
  	if (MapWindow::MapScale <=1)
#else /* MAP_ZOOM */
  	if (MapWindow::zoom.Scale() <=1)
#endif /* MAP_ZOOM */
		nearby=NIBLSCALE(1); 
	else
		nearby=NIBLSCALE(2);
  }
#endif
 
  for(i=1;i< num_trail_max; ++i) 
  {
    ///// Handle skipping

    if (i>=skip_border) {
      skip_level= max(1,skip_level-1);
      skip_border= i+2*(zero_offset % skip_level)+skip_divisor;
      index_skip = skip_level;
    }

    index_skip++;
    if ((i<num_trail_max-10) && (index_skip < skip_level)) {
      continue;
    } else {
      index_skip=0;
    }

    ////// Find the snail point

    snail_index = snail_offset+i;
    while (snail_index>=TRAILSIZE) {
      snail_index-= TRAILSIZE;
    }

    P1 = SnailTrail[snail_index];

    /////// Mark first time of display point

    if (((TrailFirstTime<0) || (P1.Time<TrailFirstTime)) && (P1.Time>=0)) {
      TrailFirstTime = P1.Time;
    }

    //////// Ignoring display elements for modes

    if (display_circling) {
      if ((!P1.Circling)&&( i<num_trail_max-60 )) {
        // ignore cruise mode lines unless very recent
	last_visible = false;
        continue;
      }
    } else {
      //  if ((P1.Circling)&&( snail_index % 5 != 0 )) {
        // JMW TODO code: This won't work properly!
        // draw only every 5 points from circling when in cruise mode
	//        continue;
      //      }
    }

    ///////// Filter if far visible

    if (!P1.FarVisible) {
      last_visible = false;
      continue;
    }

    ///////// Determine if this is visible

    this_visible =   ((P1.Longitude> bounds.minx) &&
		     (P1.Longitude< bounds.maxx) &&
		     (P1.Latitude> bounds.miny) &&
		     (P1.Latitude< bounds.maxy)) ;

    if (!this_visible && !last_visible) {
      last_visible = false;
      continue;
    }

    ////////// Find coordinates on screen after applying trail drift

    // now we know either point is visible, better get screen coords
    // if we don't already.

    double dt = max(0.0,(display_time-P1.Time)*P1.DriftFactor);
    double this_lon = P1.Longitude+traildrift_lon*dt;
    double this_lat = P1.Latitude+traildrift_lat*dt;

#if 1
    // this is faster since many parameters are const
    int Y = Real2Int((mPanLatitude-this_lat)*mDrawScale);
    int X = Real2Int((mPanLongitude-this_lon)*fastcosine(this_lat)*mDrawScale);
    P1.Screen.x = (xxs-X*cost + Y*sint)/1024;
    P1.Screen.y = (Y*cost + X*sint + yys)/1024;
#else
    LatLon2Screen(this_lon, 
		  this_lat, 
		  P1.Screen);
#endif

    ////////// Determine if we should skip if close to previous point

    if (last_visible && this_visible) {
	// only average what's visible
	if ( (abs(P1.Screen.y-point_lastdrawn.y) + abs(P1.Screen.x-point_lastdrawn.x)) <nearby) { // 100303 
		if (usecolors) {
			vario_av += P1.Vario;
			vario_av_num ++;
		}
		continue;
		// don't draw if very short line
	}
    }

    if (usecolors) {
	float useval;
	float offval=1.0;
	int usecol;

	if ( vario_av_num ) useval=vario_av/(vario_av_num+1); else useval=P1.Vario; // 091202 avnum

	if (useval<0) offval=-1;
	useval=fabs(useval);
	
	if ( useval <0.1 ) {
		P1.Colour=7;
		goto go_selcolor;
	}
	if (useval <=0.5 ) {; usecol=1; goto go_setcolor; }
	if (useval <=1.0 ) {; usecol=2; goto go_setcolor; }
	if (useval <=1.5 ) {; usecol=3; goto go_setcolor; }
	if (useval <=2.0 ) {; usecol=4; goto go_setcolor; }
	if (useval <=3.0 ) {; usecol=5; goto go_setcolor; }
	if (useval <=4.0 ) {; usecol=6; goto go_setcolor; }
	usecol=7; // 7th : 4ms and up

go_setcolor:
	P1.Colour = 7+(short int)(usecol*offval);
    } else {
	P1.Colour = 3; // blue
    }

go_selcolor:
    SelectObject(hdc, hSnailPens[P1.Colour]);

    if (!last_visible) { // draw set cursor at P1
#ifndef NOLINETO
      MoveToEx(hdc, P1.Screen.x, P1.Screen.y, NULL);
#endif
    } else {
#ifndef NOLINETO
      LineTo(hdc, P1.Screen.x, P1.Screen.y);
#else
      DrawSolidLine(hdc, P1.Screen, point_lastdrawn, rc);
#endif
    }
    point_lastdrawn = P1.Screen;
    last_visible = this_visible;
  }

  // draw final point to glider
  if (last_visible) {
#ifndef NOLINETO 
    LineTo(hdc, Orig.x, Orig.y);
#else
    DrawSolidLine(hdc, Orig, point_lastdrawn, rc);
#endif
  }

  return TrailFirstTime;
}


// change dynamically the map orientation mode
// set true flag for resetting DisplayOrientation mode and return
void MapWindow::SetAutoOrientation(bool doreset) {

  static bool doinit=true;

  if (doinit) {
	OldDisplayOrientation=DisplayOrientation;
	doinit=false;
  }

  if (doreset) {
	OldDisplayOrientation=DisplayOrientation;
	return;
  }

  // 1.4 because of correction if mapscale reported on screen in MapWindow2
#ifndef MAP_ZOOM
  if ((MapScale*1.4) >= AutoOrientScale) {
#else /* MAP_ZOOM */
  if (MapWindow::zoom.Scale() * 1.4 >= AutoOrientScale) {
#endif /* MAP_ZOOM */
	// DisplayOrientation=NORTHSMART; // better to keep the glider centered on low zoom levels
	DisplayOrientation=NORTHUP;
  } else {
	DisplayOrientation=OldDisplayOrientation;
  }
}


