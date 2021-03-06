/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id: MapWindow.h,v 8.4 2010/12/15 12:32:26 root Exp root $
*/

#if !defined(AFX_MAPWINDOW_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_MAPWINDOW_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "Sizes.h"
#include "Utils.h"
#include "XCSoar.h"
#include "Airspace.h"
#include "Parser.h"
#include "Calculations.h"

#define NORTHSMART 5
#define NORTHTRACK 4
#define TRACKCIRCLE 3
#define NORTHCIRCLE 2
#define NORTHUP 1
#define TRACKUP 0

#define DISPLAYNAME 0
#define DISPLAYNUMBER 1
#define DISPLAYNAMEIFINTASK 2
#define DISPLAYFIRSTTHREE 3
#define DISPLAYFIRSTFIVE 4
#define DISPLAYFIRST8 5
#define DISPLAYFIRST10 6
#define DISPLAYFIRST12 7
#define DISPLAYNONE 8

#define AIRPORT				0x01
#define TURNPOINT			0x02
#define LANDPOINT			0x04
#define HOME					0x08
#define START					0x10
#define FINISH				0x20
#define RESTRICTED		0x40
#define WAYPOINTFLAG	0x80

// Used by MapWindow::TextColor 
// 5 bits (0-30) . Some colors unused
#define TEXTBLACK 0
#define TEXTWHITE 1
#define TEXTGREEN 2
#define TEXTRED 3
#define TEXTBLUE 4
#define TEXTYELLOW 5
#define TEXTCYAN 6
#define TEXTMAGENTA 7
#define TEXTGREY 8
#define TEXTORANGE 9
#define TEXTLIGHTGREEN 10
#define TEXTLIGHTRED 11
#define TEXTLIGHTBLUE 12
#define TEXTLIGHTYELLOW 13
#define TEXTLIGHTCYAN 14
#define TEXTLIGHTGREY 15
#define TEXTLIGHTORANGE 16

#ifndef MAP_ZOOM
typedef enum {dmNone, dmCircling, dmCruise, dmFinalGlide} DisplayMode_t;

extern DisplayMode_t UserForceDisplayMode;
extern DisplayMode_t DisplayMode;


#endif /* ! MAP_ZOOM */
// VENTA3 note> probably it would be a good idea to separate static WP data to dynamic values,
// by moving things like Reachable, AltArival , etc to WPCALC
// Currently at 5.2.2 the whole structure is saved into the task file, so everytime we 
// change the struct all old taks files become invalid... (there's a bug, btw, in this case)

typedef struct _WAYPOINT_INFO
{
  // Number is used as an alternate short name, and can also be duplicated
  int Number;
  double Latitude;
  double Longitude;
  double Altitude;
  int Flags;
  TCHAR Name[NAME_SIZE + 1];
  TCHAR *Comment;
  POINT	Screen;
  int Zoom;
  BOOL Reachable;
  double AltArivalAGL;
  BOOL Visible;
  bool InTask;
  TCHAR *Details;
  bool FarVisible;
  int FileNum; // which file it is in, or -1 to delete
  // waypoint original format, LKW_DAT CUP etc.
  short Format;
  TCHAR Code[CUPSIZE_CODE+1];
  TCHAR Freq[CUPSIZE_FREQ+1];
  int   RunwayLen;
  int   RunwayDir;
  TCHAR Country[CUPSIZE_COUNTRY+1];
  short Style;
} WAYPOINT;

// This struct is separated from _WAYPOINT_INFO and will not be used in task files.  
// It is managed by the same functions that manage WayPointList, only add variables here
// and use them like  WayPointCalc[n].Distance  for example. 
// THIS STRUCTURE MUST BE INITIALIZED inside waypointparser function!!
typedef struct _WAYPOINT_CALCULATED
{
//  long timeslot;
  double GR;       // GR from current position 
  short VGR;       // Visual GR
  double Distance; // distance from current position 
  double Bearing;  // used for radial
  double AltArriv[ALTA_SIZE]; 
  double NextETE;
  bool Preferred;  // Flag to be used by Preferred quick selection WP page (TODO) and by BestAlternate
  double AltReqd[ALTA_SIZE];
  short WpType;
  // quick flags. if not landable, it's a turnpoint
  bool IsLandable;
  // if on, then they are also landable for sure
  bool IsAirport;
  bool IsOutlanding;
} WPCALC;

typedef struct _SNAIL_POINT
{
  float Latitude;
  float Longitude;
  float Vario;
  double Time;
  POINT Screen;
  short Colour;
  BOOL Circling;
  bool FarVisible;
  double DriftFactor;
} SNAIL_POINT;


typedef union{
  unsigned int AsInt;
  struct{
    unsigned Border:1;
    unsigned FillBackground:1;
    unsigned AlligneRight:1;
    unsigned Reachable:1;
    unsigned AlligneCenter:1;
    unsigned WhiteBorder:1;
    unsigned WhiteBold:1;
    unsigned NoSetFont:1;  // VENTA5
    unsigned Color:5;
#ifdef LKAIRSPACE
	unsigned SetTextColor:1;	// Set text color in border mode
#endif
  }AsFlag;
}TextInBoxMode_t;
  // mode are flags
  // bit 0 == fill background add border / 1
  // bit 1 == fill background            / 2
  // bit 2 == right alligned             / 4
  // bit 3 == landable TP label          / 8
  // bit 4 == center alligned

class MapWindow {
 public:
#ifdef MAP_ZOOM
  /** 
   * @brief Class responsible for handling all Map Zoom activities
   * 
   * Zoom class is responsible for:
   *  - storing Map Zoom state (global and for each of the modes)
   *  - provide Zoom related calculations
   */
  class Zoom {
  private:
    // initial fixed zoom factors
    static const double SCALE_CRUISE_INIT      = 4.0;
    static const double SCALE_CIRCLING_INIT    = 4.0 / 30;
    static const double SCALE_PANORAMA_INIT    = 7.0;
    static const double SCALE_PG_PANORAMA_INIT = 5.0;
    static const double SCALE_INVALID_INIT     = 50.0;
    
    enum TMapScaleType {
      SCALE_CRUISE,             /**< @brief Basic zoom for flight mode used for:
                                   - cruise mode when AutoZoom is disabled
                                   - thermalling mode when CirclingZoom and AutoZoom is disabled
                                   - restore user zoom when next waypoint with AutoZoom enabled
                                */
      SCALE_CIRCLING,           /**< @brief Zoom for thermalling mode when CirclingZoom is enabled */
      SCALE_PANORAMA,           /**< @brief Panorama 20 seconds zoom */
      SCALE_AUTO_ZOOM,          /**< @brief AutoZoom zoom */
      SCALE_PAN,                /**< @brief PAN zoom */
      SCALE_TARGET_PAN,         /**< @brief TARGET_PAN zoom */
      
      SCALE_NUM                 /**< @brief DO NOT USE THAT */
    };
    
    friend class MapWindow;
    
    bool _inited;                                 /**< @brief Object inited flag */
    bool _autoZoom;                               /**< @brief Stores information if AutoZoom is enabled */
    bool _circleZoom;                             /**< @brief Stores information if CirclingZoom is enabled */
    bool _bigZoom;                                /**< @brief Stores information if BigZoom was done and special refresh is needed */
    double _scale;                                /**< @brief Current map scale */
    double _modeScale[SCALE_NUM];                 /**< @brief Requested scale for each of scale types */
    double *_requestedScale;                      /**< @brief Requested scale for current scale type */
    
    // performance related members
    double _scaleOverDistanceModify;
    double _resScaleOverDistanceModify;
    double _drawScale;
    double _invDrawScale;
    
    double RequestedScale() const        { return *_requestedScale; }
    void RequestedScale(double value)    { *_requestedScale = value; }
    void CalculateTargetPanZoom();
    void CalculateAutoZoom();
    double ResScaleOverDistanceModify() const { return _resScaleOverDistanceModify; }
    double DrawScale() const             { return _drawScale; }
    double InvDrawScale() const          { return _invDrawScale; }
    
  public:
    Zoom();
    void Reset();
    
    void AutoZoom(bool enable)      { _autoZoom = enable; SwitchMode(); }
    bool AutoZoom() const           { return _autoZoom; }
    
    void CircleZoom(bool enable)    { _circleZoom = enable; SwitchMode(); }
    bool CircleZoom() const         { return _circleZoom; }
    
    void BigZoom(bool enable)       { _bigZoom = enable; }
    bool BigZoom() const            { return _bigZoom; }
    
    void SwitchMode();
    
    double Scale() const { return _scale; }
    
    void EventAutoZoom(int vswitch);
    void EventSetZoom(double value);
    void EventScaleZoom(int vswitch);
    
    void UpdateMapScale();
    void ModifyMapScale();
  };
  

  /** 
   * @brief Class responsible for handling Map Display Mode data
   * 
   * There are 2 types of map display modes:
   *  - fly
   *     - modes representing current flight mode (ie. cruise, circle, final glide)
   *     - only one of them can be active at a time
   *  - special
   *     - modes representing special actions like (PANORAMA, PAN, etc.)
   *     - none or more than one special state can be enabled at the same time
   */
  class Mode {
  public:
    /** 
     * @brief Fly Modes
     */
    enum TModeFly {
      MODE_FLY_NONE        = 0x0000,
      MODE_FLY_CRUISE      = 0x0001,
      MODE_FLY_CIRCLING    = 0x0002,
      MODE_FLY_FINAL_GLIDE = 0x0004
    };
    static const unsigned FLY_MASK = 0x00FF;
    
    /** 
     * @brief Special Modes
     */
    enum TModeSpecial {
      MODE_SPECIAL_NONE        = 0x0000,
      MODE_SPECIAL_PAN         = 0x0100,
      MODE_SPECIAL_TARGET_PAN  = 0x0200,
      MODE_SPECIAL_PANORAMA    = 0x0400
    };
    static const unsigned SPECIAL_MASK = 0xFF00;
    
    /** 
     * @brief All Display Modes
     */
    enum TMode {
      MODE_NONE        = MODE_FLY_NONE,
      
      MODE_CRUISE      = MODE_FLY_CRUISE,
      MODE_CIRCLING    = MODE_FLY_CIRCLING,
      MODE_FINAL_GLIDE = MODE_FLY_FINAL_GLIDE,
      
      MODE_PAN         = MODE_SPECIAL_PAN,
      MODE_TARGET_PAN  = MODE_SPECIAL_TARGET_PAN,
      MODE_PANORAMA    = MODE_SPECIAL_PANORAMA
    };
    
  private:
    friend class Zoom;
    unsigned _mode;                    /**< @brief Current Map Display Mode */
    unsigned _lastMode;                /**< @brief Previous Map Display Mode */
    TModeFly _userForcedFlyMode;       /**< @brief Fly Mode forced by a user */
    
  public:
    Mode();
    
    TModeFly UserForcedMode() const    { return _userForcedFlyMode; }
    void UserForcedMode(TModeFly mode) { _userForcedFlyMode = mode; }
    
    bool Is(TMode mode) const   { return _mode & mode; }
    void Fly(TModeFly flyMode);
    TModeFly Fly() const        { return static_cast<TModeFly>(_mode & FLY_MASK); }
    
    void Special(TModeSpecial specialMode, bool enable);
    TModeSpecial Special() const { return static_cast<TModeSpecial>(_mode & SPECIAL_MASK); }
    
    bool AnyPan() const { return _mode & (MODE_SPECIAL_PAN | MODE_SPECIAL_TARGET_PAN); }
  };
  
#endif /* MAP_ZOOM */

  static bool IsDisplayRunning();
  static int iAirspaceMode[AIRSPACECLASSCOUNT];
  static int iAirspaceBrush[AIRSPACECLASSCOUNT]; 
  static int iAirspaceColour[AIRSPACECLASSCOUNT];
  static bool bAirspaceBlackOutline;
  static BOOL CLOSETHREAD;

  static COLORREF GetAirspaceColour(int i) {
    return Colours[i];
  }
  static HBRUSH GetAirspaceBrush(int i) {
    return hAirspaceBrushes[i];
  }
  static COLORREF GetAirspaceColourByClass(int i) {
    return Colours[iAirspaceColour[i]];
  }
  static HBRUSH GetAirspaceBrushByClass(int i) {
    return hAirspaceBrushes[iAirspaceBrush[i]];
  }

 private:
  // 12 is number of airspace types
  static HPEN hAirspacePens[AIRSPACECLASSCOUNT];
  static HPEN hSnailPens[NUMSNAILCOLORS];
  static COLORREF hSnailColours[NUMSNAILCOLORS];
  static HBRUSH hAirspaceBrushes[NUMAIRSPACEBRUSHES];
  static HBITMAP hAirspaceBitmap[NUMAIRSPACEBRUSHES];
  static HBITMAP hAboveTerrainBitmap;
  static HBRUSH hAboveTerrainBrush;
  static COLORREF Colours[NUMAIRSPACECOLORS];

  static BOOL Initialised;
  static bool GliderCenter;
  static DWORD timestamp_newdata;
  static bool RequestFullScreen;
  static bool LandableReachable;
#ifndef MAP_ZOOM
  static void ModifyMapScale();
  static double ResMapScaleOverDistanceModify; // speedup
#endif /* ! MAP_ZOOM */

 public:
#ifndef MAP_ZOOM

  // These two values were private.. moved to public VNT 090621
  static double MapScaleOverDistanceModify; // speedup
  static double RequestMapScale;
#else /* MAP_ZOOM */
  static Zoom zoom;
  static Mode mode;
#endif /* MAP_ZOOM */

  static RECT MapRect;
  static RECT MapRectBig;
#ifndef MAP_ZOOM
  static double MapScale;
#endif /* ! MAP_ZOOM */
  static bool ForceVisibilityScan;

  static bool MapDirty;

  static unsigned char DeclutterLabels;
  static bool EnableTrailDrift;
  static int GliderScreenPosition;
  static int GliderScreenPositionX;
  static int GliderScreenPositionY;
  static void RequestFastRefresh();

  static void UpdateTimeStats(bool start);

#ifndef MAP_ZOOM
  static bool isAutoZoom();
  static bool isPan();

#endif /* ! MAP_ZOOM */
  // Drawing primitives
  static void DrawDashLine(HDC , const int , const POINT , const POINT , 
			   const COLORREF , 
			   const RECT rc);
  /* Not used
  static void DrawDotLine(HDC, const POINT , const POINT , const COLORREF , 
			  const RECT rc);
  */

  static void _DrawLine(HDC hdc, const int PenStyle, const int width, 
	       const POINT ptStart, const POINT ptEnd, 
	       const COLORREF cr, const RECT rc);
  static void _Polyline(HDC hdc, POINT* pt, const int npoints, const RECT rc);
  static void DrawBitmapIn(const HDC hdc, const POINT &sc, const HBITMAP h);
  static void DrawBitmapX(HDC hdc, int top, int right,
		     int sizex, int sizey,
		     HDC source,
		     int offsetx, int offsety,
		     DWORD mode);

  // ...
  static void RequestToggleFullScreen();
  static void RequestOnFullScreen();
  static void RequestOffFullScreen();

  static void OrigScreen2LatLon(const int &x, const int &y, 
                                double &X, double &Y);
  static void Screen2LatLon(const int &x, const int &y, double &X, double &Y);

  static void LatLon2Screen(const double &lon, const double &lat, POINT &sc);
  static void LatLon2Screen(pointObj *ptin, POINT *ptout, const int n,
			    const int skip);

  static void CloseDrawingThread(void);
  static void CreateDrawingThread(void);
  static void SuspendDrawingThread(void);
  static void ResumeDrawingThread(void);
  static void LKWriteText(HDC hDC, const TCHAR* wText, int x, int y, int maxsize, const bool mode, const short align, COLORREF rgb_tex, bool invertable);

  static LRESULT CALLBACK MapWndProc (HWND hWnd, UINT uMsg, WPARAM wParam,LPARAM lParam);

  static bool IsMapFullScreen();

  // input events or reused code
#ifndef MAP_ZOOM
  static void Event_SetZoom(double value);
  static void Event_ScaleZoom(int vswitch);
#endif /* ! MAP_ZOOM */
  static void Event_Pan(int vswitch);
  static void Event_TerrainTopology(int vswitch);
#ifndef MAP_ZOOM
  static void Event_AutoZoom(int vswitch);
#endif /* ! MAP_ZOOM */
  static void Event_PanCursor(int dx, int dy);
  static bool Event_InteriorAirspaceDetails(double lon, double lat);
  static bool Event_NearestWaypointDetails(double lon, double lat, double range, bool pan);

  static void UpdateInfo(NMEA_INFO *nmea_info,
			 DERIVED_INFO *derived_info);
  static rectObj CalculateScreenBounds(double scale);
  static void ScanVisibility(rectObj *bounds_active);

#ifndef MAP_ZOOM
  static void SwitchZoomClimb(void);

#endif /* ! MAP_ZOOM */
 private:
  static void CalculateScreenPositions(POINT Orig, RECT rc, 
                                       POINT *Orig_Aircraft);
  static void CalculateScreenPositionsGroundline();
  static void CalculateScreenPositionsAirspace();
#ifndef LKAIRSPACE
  static void CalculateScreenPositionsAirspaceCircle(AIRSPACE_CIRCLE& circ);
  static void CalculateScreenPositionsAirspaceArea(AIRSPACE_AREA& area);
#endif
  static void CalculateScreenPositionsThermalSources();
  #if MULTICALC
  static void LKCalculateWaypointReachable(short multicalc_slot, short numslots);
  #else
  static void CalculateWaypointReachable(void);
  static void CalculateWaypointReachableNew(void);
  #endif
  
  static bool PointVisible(const POINT &P);
  static bool PointVisible(const double &lon, const double &lat);
  static bool PointInRect(const double &lon, const double &lat,
			  const rectObj &bounds);

  static void DrawAircraft(HDC hdc, const POINT Orig);
  static void DrawCrossHairs(HDC hdc, const POINT Orig, const RECT rc);
  static void DrawGlideCircle(HDC hdc, const POINT Orig, const RECT rc); // VENTA3
  static void DrawHeading(HDC hdc, const POINT Orig, const RECT rc); // VENTA10
  static void DrawBestCruiseTrack(HDC hdc, const POINT Orig);
  static void DrawCompass(HDC hdc, const RECT rc);
  static void DrawTRI(HDC hdc, const RECT rc);
  static void DrawTarget(HDC hdc, const RECT rc,int ttop,int tbottom,int tleft,int tright);
  //  static void DrawHorizon(HDC hdc, const RECT rc);
  //  static void DrawWind(HDC hdc, POINT Orig, RECT rc);
  //  static void DrawWindAtAircraft(HDC hdc, POINT Orig, RECT rc);
  static void DrawWindAtAircraft2(HDC hdc, POINT Orig, RECT rc);
  static void DrawAirSpace(HDC hdc, const RECT rc);
#ifdef LKAIRSPACE
  static void DrawAirspaceLabels(HDC hdc, const RECT rc, const POINT Orig_Aircraft);
#endif
  static void DrawWaypoints(HDC hdc, const RECT rc);
  static void DrawWaypointsNew(HDC hdc, const RECT rc);
  static void DrawLook8000(HDC hdc, const RECT rc);
  static void DrawMapSpace(HDC hdc, const RECT rc);
  static void DrawNearest(HDC hdc, const RECT rc);
  static void DrawNearestTurnpoint(HDC hdc, const RECT rc);
  static void DrawCommon(HDC hdc, const RECT rc);
  static void DrawInfoPage(HDC hdc, const RECT rc, const bool forceinit);
  static void DrawTraffic(HDC hdc, const RECT rc);
  static void WriteInfo(HDC hdc, bool *showunit, TCHAR *BufferValue, TCHAR *BufferUnit, TCHAR *BufferTitle,
                                short *columnvalue, short *columntitle, short *row1, short *row2, short *row3);
  static bool LKFormatValue(const short fvindex, const bool longtitle, TCHAR *BufferValue, TCHAR *BufferUnit, TCHAR *BufferTitle);
  static void LKFormatDist(const int wpindex, const bool wpvirtual, TCHAR *BufferValue, TCHAR *BufferUnit);
  static void LKFormatBrgDiff(const int wpindex, const bool wpvirtual, TCHAR *BufferValue, TCHAR *BufferUnit);
  static void LKFormatGR(const int wpindex, const bool wpvirtual, TCHAR *BufferValue, TCHAR *BufferUnit);
  static void LKFormatAltDiff(const int wpindex, const bool wpvirtual, TCHAR *BufferValue, TCHAR *BufferUnit);
  static void LKUpdateOlc(void);
//  static void LKWriteText(HDC hDC, const TCHAR* wText, int x, int y, int maxsize, const bool mode, const short align, COLORREF rgb_tex, bool invertable);

#ifdef CPUSTATS
  static void DrawCpuStats(HDC hdc, const RECT rc);
#endif
#ifdef DRAWDEBUG
  static void DrawDebug(HDC hdc, const RECT rc);
#endif
  static void DrawWelcome8000(HDC hdc, const RECT rc); 
  static void DrawLKStatus(HDC hdc, const RECT rc);
  static void DrawFlightMode(HDC hdc, const RECT rc);
  static void DrawGPSStatus(HDC hdc, const RECT rc);
  static double DrawTrail(HDC hdc, const POINT Orig, const RECT rc);
  static double LKDrawTrail(HDC hdc, const POINT Orig, const RECT rc);
  static void DrawTeammate(HDC hdc, const RECT rc);
  static void DrawTrailFromTask(HDC hdc, const RECT rc, const double);
  static void DrawOffTrackIndicator(HDC hdc, const RECT rc);
  static void DrawProjectedTrack(HDC hdc, const RECT rc, const POINT Orig);
  static void DrawStartSector(HDC hdc, const RECT rc, POINT &Start,
                              POINT &End, int Index);
  static void DrawTask(HDC hdc, RECT rc, const POINT &Orig_Aircraft);
  static void DrawThermalEstimate(HDC hdc, const RECT rc);
  static void DrawTaskAAT(HDC hdc, const RECT rc);
  static void DrawAbortedTask(HDC hdc, const RECT rc, const POINT Orig);
  static void DrawBearing(HDC hdc, const RECT rc);
  static void DrawGreatCircle(HDC hdc,
                              double lon_start, double lat_start,
                              double lon_end, double lat_end,
			      const RECT rc);
  // static void DrawMapScale(HDC hDC,RECT rc);
  static void DrawMapScale(HDC hDC, const RECT rc, 
			   const bool ScaleChangeFeedback);
  static void DrawMapScale2(HDC hDC, const RECT rc, 
			    const POINT Orig_Aircraft);
  static void DrawFinalGlide(HDC hDC, const RECT rc);
  static void DrawThermalBand(HDC hDC, const RECT rc);
  static void DrawGlideThroughTerrain(HDC hDC, const RECT rc);
  static void DrawTerrainAbove(HDC hDC, const RECT rc);
  static void DrawCDI();
    static void DrawSpeedToFly(HDC hDC, RECT rc); // VNT9
  static void DrawFLARMTraffic(HDC hDC, RECT rc, POINT Orig_Aircraft);
  static void LKDrawFLARMTraffic(HDC hDC, RECT rc, POINT Orig_Aircraft);
  static void LKDrawVario(HDC hDC, RECT rc);
    
  static void DrawSolidLine(const HDC&hdc, 
			    const POINT&start, 
			    const POINT&end ,
			    const RECT rc);
  static bool TextInBox(HDC hDC, TCHAR* Value, int x, int y, int size, TextInBoxMode_t Mode, bool noOverlap=false);
  static void ToggleFullScreenStart();
  static void TextColor(HDC hDC, short colorcode);
  static bool WaypointInTask(int ind);

 private:
  static int iSnailNext;
  static HBITMAP hDrawBitMap;
  static HBITMAP hDrawBitMapTmp;
  static HBITMAP hMaskBitMap;
  static HDC hdcDrawWindow;
  static HDC hdcScreen;
  static HDC hDCTemp;
  static HDC hDCMask;
  static double PanLatitude;
  static double PanLongitude;
  static DWORD  dwDrawThreadID;
  static HANDLE hDrawThread;
  static double DisplayAngle;
  static double DisplayAircraftAngle;
#ifndef MAP_ZOOM
  static double DrawScale;
  static double InvDrawScale;
#endif /* ! MAP_ZOOM */
  
 public:
  static void RefreshMap(); // set public VENTA
  static HANDLE hRenderEvent;
#ifndef MAP_ZOOM
  static bool EnablePan; // sorry need this public for virtual keys
#endif /* ! MAP_ZOOM */

  static rectObj screenbounds_latlon;
  
  static BOOL THREADRUNNING;
  static BOOL THREADEXIT;
  
  static double LimitMapScale(double value);

  static bool WaypointInRange(int i);

#ifndef MAP_ZOOM
  static bool SetTargetPan(bool dopan, int task_index);
#else /* MAP_ZOOM */
  static void SetTargetPan(bool dopan, int task_index);
#endif /* MAP_ZOOM */

  static double GetPanLatitude() { return PanLatitude; }
  static double GetPanLongitude() { return PanLongitude; }
#ifndef MAP_ZOOM
  static double GetInvDrawScale() { return InvDrawScale; }
#else /* MAP_ZOOM */
  static double GetInvDrawScale() { return zoom.InvDrawScale(); }
#endif /* MAP_ZOOM */
  static double GetDisplayAngle() { return DisplayAngle; }
  static void SetAutoOrientation(bool doreset);

 private:
  static HBITMAP 
    hTurnPoint, hSmall, hInvTurnPoint, hInvSmall, hCruise, hClimb,
    hFinalGlide, hAutoMacCready, hTerrainWarning, hGPSStatus1, hGPSStatus2,
    hAbort, hLogger, hLoggerOff, hFLARMTraffic;
#ifdef LKAIRSPACE
	static HBITMAP hAirspaceWarning;
#endif
  static HBRUSH   hBackgroundBrush;
  static COLORREF BackgroundColor;

  static HBRUSH   hInvBackgroundBrush[LKMAXBACKGROUNDS]; // 091110 fixed number of backgrounds in MapWindow


  static      HPEN hpAircraft;
  static      HPEN hpAircraftBorder;
  static      HPEN hpWind;
  static      HPEN hpWindThick;
  static      HPEN hpBearing;
  static      HPEN hpBestCruiseTrack;
  static      HPEN hpCompass;
  static	HPEN hpThermalCircle;
  static      HPEN hpOvertarget;
  static      HPEN hpThermalBand;
  static      HPEN hpThermalBandGlider;
  static      HPEN hpFinalGlideAbove;
  static      HPEN hpFinalGlideBelow;
  static      HPEN hpFinalGlideBelowLandable;
  static      HPEN hpMapScale;
  static      HPEN hpMapScale2;
  static      HPEN hpTerrainLine;
  static      HPEN hpTerrainLineBg;
  static      HPEN hpVisualGlideLightRed; // VENTA3
  static      HPEN hpVisualGlideHeavyRed; // 
  static      HPEN hpVisualGlideLightBlack; // VENTA3
  static      HPEN hpVisualGlideHeavyBlack; // 
  static      HPEN hpVisualGlideExtra; // future use
  static      HPEN hpSpeedFast;
  static      HPEN hpSpeedSlow;
  static      HPEN hpStartFinishThick;
  static      HPEN hpStartFinishThin;
  
  static      HBRUSH hbCompass;
  static      HBRUSH hbThermalBand;
  static      HBRUSH hbBestCruiseTrack;
  static      HBRUSH hbFinalGlideBelow;
  static      HBRUSH hbFinalGlideBelowLandable;
  static      HBRUSH hbFinalGlideAbove;
  static      HBRUSH hbWind;

  static RECT MapRectSmall;
  static bool MapFullScreen;

  static DWORD fpsTime0;

  // static void DisplayAirspaceWarning(int Type, TCHAR *Name , AIRSPACE_ALT Base, AIRSPACE_ALT Top ); REMOVE 110102

#ifndef MAP_ZOOM
  static void UpdateMapScale();
#endif /* ! MAP_ZOOM */
  static void CalculateOrigin(const RECT rc, POINT *Orig);


  static DWORD DrawThread (LPVOID);

  static void RenderMapWindow(  RECT rc);
  static void RenderMapWindowBg(HDC hdc, const RECT rc,
				const POINT &Orig,
				const POINT &Orig_Aircraft);
  static void UpdateCaches(bool force=false);
  static double findMapScaleBarSize(const RECT rc);

  #define SCALELISTSIZE  30
  static int ScaleListCount;
  static int ScaleCurrent;
  static double ScaleList[SCALELISTSIZE];
  static double StepMapScale(int Step);
  static double FindMapScale(double Value);

  static HPEN    hpCompassBorder;
  static HBRUSH  hBrushFlyingModeAbort;

  static HBITMAP hBmpAirportReachable;
  static HBITMAP hBmpAirportUnReachable;
  static HBITMAP hBmpFieldReachable;
  static HBITMAP hBmpFieldUnReachable;
  static HBITMAP hBmpThermalSource;
  static HBITMAP hBmpTarget;
  static HBITMAP hBmpTeammatePosition;


  #if TOPOFASTLABEL
  // How many slots in screen, divided by horizontal blocks on vertical positions
  // How many parts of vertical screens we are using. H=480 mean 48 pixel sized slots
  // Each slot should contain MapWindowLogFont Hsize with some margin.
  #define SCREENVSLOTS	10
  // Max number of labels in each vblock
  #define MAXVLABELBLOCKS 15
  // total labels printed so far
  static int nLabelBlocks;
  static int nVLabelBlocks[SCREENVSLOTS+1];
  static RECT LabelBlockCoords[SCREENVSLOTS+1][MAXVLABELBLOCKS+1];
  static char *slot;
  #else
  #define MAXLABELBLOCKS 100
  static int nLabelBlocks;
  static RECT LabelBlockCoords[MAXLABELBLOCKS];
  #endif

  static void StoreRestoreFullscreen(bool);
 public:

  static double GetApproxScreenRange(void);
  static int GetMapResolutionFactor();

  static POINT GetOrigScreen(void) { return Orig_Screen; }

 private:
  static POINT Orig_Screen;
  static HBITMAP hBmpMapScale;
  static HBITMAP hBmpCompassBg;
  static HBITMAP hBmpClimbeAbort;
  static HBITMAP hBmpUnitKm;
  static HBITMAP hBmpUnitSm;
  static HBITMAP hBmpUnitNm;
  static HBITMAP hBmpUnitM;
  static HBITMAP hBmpUnitFt;
  static HBITMAP hBmpUnitMpS;

#ifndef MAP_ZOOM
  static bool TargetPan;
#endif /* ! MAP_ZOOM */
  static double TargetZoomDistance;
  static int TargetPanIndex; 
  static void ClearAirSpace(bool fill);

 public:
#ifndef MAP_ZOOM
  static bool isTargetPan(void);
  static bool AutoZoom;
#endif /* ! MAP_ZOOM */
  #if TOPOFASTLABEL
  static bool checkLabelBlock(RECT *rc);
  #else
  static bool checkLabelBlock(RECT rc);
  #endif
  static bool RenderTimeAvailable();
#ifndef MAP_ZOOM
  static bool BigZoom;
#endif /* ! MAP_ZOOM */
  static int SnailWidthScale; 
  static int WindArrowStyle;
  static bool TargetDragged(double *longitude, double *latitude);

 private:
  static NMEA_INFO DrawInfo;
  static DERIVED_INFO DerivedDrawInfo;

  static void CalculateOrientationTargetPan(void);
  static void CalculateOrientationNormal(void);

  static POINT Groundline[NUMTERRAINSWEEPS+1];

  static int TargetDrag_State;
  static double TargetDrag_Latitude;
  static double TargetDrag_Longitude;

  // include declaration for alpha blended drawing
  #include "MapWindowA.h"
};

void PolygonRotateShift(POINT* poly, int n, int x, int y, 
                        double angle);

extern void DrawDashLine(HDC , INT ,POINT , POINT , COLORREF );

#endif
