/***    driveway - draw 1807 94th Ave NE driveway layout
 *
 *      Author:
 *      (c) 1997, Benjamin W. Slivka
 *
 *      History:
 *      19-Jul-1997 bens    Initial version (from stock.exe ancestry)
 *      22-Jul-1997 bens    Draw the driveway
 *      23-Jul-1997 bens    Compute paver needs
 *      24-Jul-1997 bens    Print out dimensions
 */

#ifndef WIN32
typedef unsigned int UINT;
#endif

#include <windows.h>
#include <commdlg.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "ids.h"

/**************
 *** Macros *****************************************************************
 **************/

#define MIR(x)  MAKEINTRESOURCE(x)  // Make DialogBox lines shorter

#define Message(psz,hwnd)                                \
MessageBox(hwnd,psz,"Internal Error",MB_APPLMODAL|MB_OK|MB_ICONEXCLAMATION)


#define	sqr(x)	((x)*(x))


/*****************
 *** Constants **************************************************************
 *****************/

#define cchMax     80                   // Maximum line length

#define CCHMAXPATH  256


/************************
 *** Type Definitions *******************************************************
 ************************/

/***    GLOBAL - global data
 *
 */
typedef struct {    /* g */
    HWND    hwnd;               // Client window
    int     cxMain;             // X width of main window
    int     cyMain;             // Y width of main window
    int     cxClient;           // X width of client area of window
    int     cyClient;           // Y width of client area of window
    HANDLE  hInstance;          // App instance handle
    FARPROC lpfnAboutDlgProc;   // About DlgProc instance function pointer
    char    achMisc[CCHMAXPATH];// Temp buffer for formating
    char    achTitle[CCHMAXPATH]; // Stock data title
} GLOBAL;


/*****************
 *** Variables **************************************************************
 *****************/

GLOBAL g;	// Globals
FILE  *fout;	// Details file
float   totalLinealInches=0;
long	nPavers=0;
BOOL	fCountedPavers=0;


/***************************
 *** Function Prototypes ****************************************************
 ***************************/

int PASCAL WinMain(HANDLE hInstance,
                   HANDLE hPrevInstance,
                   LPSTR lpszCmdLine,
                   int nCmdShow);

BOOL FAR PASCAL AboutDlgProc(HWND hDlg,UINT msg,UINT wParam,LONG lParam);
long FAR PASCAL WndProc(HWND hDlg,UINT msg,UINT wParam,LONG lParam);

BOOL   BeginApp(HANDLE hInstance,HANDLE hPrevInstance);
VOID   CopyToClipboard(HWND hwnd);
VOID   DoCommand(HWND hwnd,UINT wParam,LONG lParam);
VOID   EndApp(VOID);
VOID   NotImplemented(HWND hwnd, char *psz);
VOID   PrintView(HWND hwnd);
VOID   DrawDriveway(HDC hdc,RECT *prect);
void   tesselate(
    HDC	    hdc,	    // dc
    float   iRadiusOuter,   // outer radius
    float   iRadiusInner,   // inner radius
    float   iChord,	    // length of chord at outer radius
    float   xiStart,	    // left-most (northern most) chord start
    float   xiEnd,	    // right-most (southern most) chord end
    float   yiEnd,      // Lowest chord start
    int     sign        // 1 => Draw clockwise; -1 => counter clockwise
    );
void   addPavers(char *sz, float li);
void   countArcPavers(char *sz,
		      float radiusi,
		      float xiStartArc,
		      float yiStartArc,
		      float xiEndArc,
		      float yiEndArc);


/***************
 *** WinMain ****************************************************************
 ***************/

int PASCAL WinMain(HANDLE hInstance,
                   HANDLE hPrevInstance,
                   LPSTR lpszCmdLine,
                   int nCmdShow)
{
    MSG     msg;
    struct  tm *newtime;
    time_t  aclock;

    fout = fopen("driveway.txt","w"); //* detailed measurements
    fprintf(fout, "1807 94th Ave NE: Driveway & paver measurements\n\n");
    fprintf(fout, "Notes:\n");
    fprintf(fout, "(1) all dimensions given as x,y\n");
    fprintf(fout, "(2) 0,0 is NORTH edge of driveway at circle center\n");
    fprintf(fout, "(3) All radial line coordinates are *NORTH* edge of 6\" paver line\n");
    time( &aclock );                 /* Get time in seconds */
    newtime = localtime( &aclock );  /* Convert time to struct tm form */
    fprintf(fout, "%s\n",asctime(newtime));
    fprintf(fout,"%61s, %9s, %5s\n","paver area","LinFeet","nTile");
    fprintf(fout,"-----------------------------------------------------------------------------\n");
    if (!BeginApp(hInstance,hPrevInstance))
        exit(1);

    //  Show window
    ShowWindow(g.hwnd,nCmdShow);
    UpdateWindow(g.hwnd);

    while(GetMessage(&msg,NULL,0,0))
        {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }

    EndApp();
    fprintf(fout,"-----------------------------------------------------------------------------\n");
    fprintf(fout,"%61s, %7.2f, %5ld\n","TOTAL",totalLinealInches,nPavers);
    fclose(fout);

    return msg.wParam;
}


/************************************************
 *** Functions - Listed in Alphabetical Order *******************************
 ************************************************/

/***    AboutDlgProc - About Dialog Box Procedure
 *
 */
BOOL FAR PASCAL AboutDlgProc(HWND hdlg,UINT msg,UINT wParam,LONG lParam)
{
    switch (msg)
        {
        case WM_INITDIALOG:
            return TRUE;

        case WM_COMMAND:
            switch (wParam)
                {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hdlg, 0);
                    return TRUE;
                }
            break;
        }
    return FALSE;
}


/***	BeginApp - Initialize application
 *
 */
BOOL BeginApp(HANDLE hInstance,HANDLE hPrevInstance)
{
    long        lStyle;
    static char szAppName [] = "Driveway";
    WNDCLASS    wndclass;

    // Save hInstance
    g.hInstance = hInstance;

    // Register window class, if necessary
    if (!hPrevInstance)
        {
        wndclass.style         = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc   = WndProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 0;
        wndclass.hInstance     = g.hInstance;
        wndclass.hIcon         = LoadIcon(hInstance, szAppName);
        wndclass.hCursor       = LoadCursor(NULL,IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wndclass.lpszMenuName  = szAppName;
        wndclass.lpszClassName = szAppName;

        RegisterClass(&wndclass);
        }

    // Create main window

    LoadString(g.hInstance,IDS_APP_TITLE,g.achMisc,sizeof(g.achMisc));
    lStyle = WS_OVERLAPPEDWINDOW;
    g.hwnd = CreateWindow(
                szAppName,          // Window class
                g.achMisc,          // Window Title
                lStyle,             // Window Style
                0,                  // Left edge of window
                0,                  // Top edge of window
                500,                // Width of window
                400,                // Height of window
                NULL,               // Parent window
                NULL,               // Control ID
                hInstance,          // App instance
                NULL                // lpCreateStruct
             );

    // Create dialog procedure instance(s)
    g.lpfnAboutDlgProc = MakeProcInstance(AboutDlgProc,g.hInstance);

    return TRUE;
}


/***    CopyToClipboard - Copy graph to clipboard
 *
 */
VOID   CopyToClipboard(HWND hwnd)
{
    HDC             hdc;
    GLOBALHANDLE    hgmem;
    HANDLE          hmf;
    LPMETAFILEPICT  lpMFP;
    RECT            rect;

    // Create Metafile

    hdc = CreateMetaFile(NULL);     // Memory metafile

    rect.top    = 0;                // Arbitrary rectangle
    rect.left   = 0;
    rect.bottom = 1200;
    rect.right  = 1600;
    DrawDriveway(hdc,&rect);           // Draw into metafile

    hmf = CloseMetaFile(hdc);       // Complete metafile

    // Copy to the clipboard

    hgmem = GlobalAlloc(GHND,(DWORD)sizeof(METAFILEPICT));
    lpMFP = (LPMETAFILEPICT) GlobalLock(hgmem);

    lpMFP->mm = MM_HIMETRIC;        // Try this?
    lpMFP->yExt = rect.bottom; 	    // units are 0.01 mm
    lpMFP->xExt = rect.right;
    lpMFP->hMF = hmf;

    GlobalUnlock(hgmem);

    OpenClipboard(hwnd);
    EmptyClipboard();
    SetClipboardData(CF_METAFILEPICT,hgmem);
    CloseClipboard();
}


/***    DoCommand - Handle COMMAND messages
 *
 */
VOID DoCommand(HWND hwnd,UINT wParam,LONG lParam)
{
    switch (wParam) {
        case IDM_ABOUT:
            DialogBox(g.hInstance,MIR(IDD_ABOUT),hwnd,g.lpfnAboutDlgProc);
	    break;

        case IDM_EDIT_COPY:
            CopyToClipboard(hwnd);
	    break;

	case IDM_FILE_PRINT:
	    PrintView(hwnd);
	    break;

	case IDM_FILE_PRINTER_SETUP:
	    NotImplemented(hwnd,"File.Printer Setup...");
	    break;

	case IDM_FILE_EXIT:
	    SendMessage(hwnd, WM_CLOSE, 0, 0L);
	    break;
    }
    return;
}


/***	EndApp - End application
 *
 */
VOID EndApp(VOID)
{
    // Free dialog procedure instances

    FreeProcInstance(g.lpfnAboutDlgProc);
}


/***	NotImplemented - show not-implemented
 *
 *
 */
VOID NotImplemented(HWND hwnd, char *psz)
{
    MessageBox(hwnd, "Not Implemented!", psz,
    	MB_APPLMODAL | MB_OK | MB_ICONEXCLAMATION);
}



/************ GLOBALS for drawing *******************/

    BOOL	f;
    int		i;
    char    ach[1024];

    float       li;     /* lineal inches of pavers */
    long	n;	    /* number of pavers */
    
    HPEN	hpenSave;
    HPEN	hpenGuide;

    float	ihyp;

    float	xi;
    float	yi;
    float	xi2;
    float	yi2;
    float	ri;

    float	sinx;
    float	siny;

    float	radiusi;

    float	xiLeft;
    float	yiTop;
    float	xiRight;
    float	yiBottom;

    float	xiStartArc;
    float	yiStartArc;
	
    float	xiEndArc;
    float	yiEndArc;

    const int 	cxMargin = 8;		// Margin for drawing (pels)
    const int 	cyMargin = 8;		

    int	        cyFlip;
    
    float	ciPaver = 6.0;		// x/y size of border paver
    float	ciDrain = 6.5;		// width of drain

//* Actual driveway measurements

    //* Drain arc
#define 	radiusiDrainInner  (31.0f*12.0f)
    float	radiusiDrainOuter = radiusiDrainInner + 6.5f;
    
    //* Middle arc (between drain and outer arc)
#define		radiusi2Outer	  (radiusiDrainInner + 13.0f*12.0f + 3.0f)
    float	radiusi2Inner    = radiusi2Outer - 6.0f; // 6" paver
    
    //* Outer arc
#define 	radiusi3Outer 	  (radiusi2Outer + 12.0f*12.0f + 3.5f)
    float	radiusi3Inner	 = radiusi3Outer - 6.0f;
    
    //* Edges
#define         cxiWestEdge	   (84.0f*12.0f + 7.5f)
    
    float	cyiSouthEdge	  = 35.0*12.0 + 10.0;
#define		cxiSouthToCenter   (41.0f*12.0f)

    float	cyiNorthEdge      = 24.0*12.0 + 1.5;
#define		cxiNorthToCenter   (cxiWestEdge - cxiSouthToCenter)
    
    float       cxiNorthDrain     = cxiNorthToCenter - 242.0f;
    float	cxiSouthDrain	  = cxiSouthToCenter - 213.0f;

    float	cyiCenterToNorthEdge = 10.0*12.0 + 11.0;
#define		cyiCenterToSouthEdge  (10.0f*12.0f)

    float	cxiESEdge	  =  8.0*12.0 +  2.0;
    float	cxiWSEdge	  = 19.0*12.0 + 10.0;
    float	cxiWNEdge	  = 41.0*12.0 +  6.0;

    //* Basketball
    float	ciBBKeyDiameter	   = 12.0*12.0; //* standard dimension
    float	cxiBBLane	   = 19.0*12.0; //* standard dimension
    float	cyiBBWestToCenter  = 210.0;
    float	cxiBBNorthToCenterBob = 245.0; //* Bob's original placement
    float	cyiBBWestToCenterBob  = 221.5; //* Bob's original placement

    //* Radiating spokes at doorway - x,y coordinates of tips
    typedef struct {
	float	x;
	float	y;
	float   xiDrain;
        float   yiDrain;
	float	ichordOuter;
    } IPT; /* ipt */
    
    IPT	aipt[] = {
    { cxiWestEdge - (43.0f*12.0f + 7.5f), cyiCenterToSouthEdge + 45.5f ,0,0,0},
    { cxiWestEdge - (38.0f*12.0f + 2.5f), cyiCenterToSouthEdge + 83.0f ,0,0,0},
    { cxiWestEdge - (31.0f*12.0f + 1.0f), cyiCenterToSouthEdge + 92.5f ,0,0,0},
    { cxiWestEdge - (23.0f*12.0f + 4.0f), cyiCenterToSouthEdge + 65.5f ,0,0,0},
    };
#define	nipt	(sizeof(aipt)/sizeof(IPT))

//* Compute maximum drawing area
    float	cxiMax = cxiWestEdge + 12.0f;   // maximum x dimension (inches)
    float	cyiMax = radiusi3Outer + 12.0f;	// maximum y dimension (inches)
    
    float	yFactor;
    float	xFactor;
    float	scale;
    
//* Transform inches to pels
#define	sx(xi)	((int)(scale*(xi))+cxMargin)
#define sy(yi)	(cyFlip - (int)(scale*(yi)))

/***    szFromInches - Convert inches to formated string <feet' inches>
 *
 *      Entry:
 *          inches - to convert
 *
 *      Exit:
 *          Returns pointer to formatted string buffer
 *          NOTE: Cycles through 10 internal buffers
 */
char *szFromInches(float inches)
{
    static int  i=0;
#define nStrings    10
#define cchStrings  20
    static char achPrivate[nStrings][cchStrings];

    int     feet;
    float   iRemainder;

    //* Compute feet and inches
    feet = (int)(inches/12.0f);
    iRemainder = inches - feet*12.0f;

    //* Cycle string buffer
    i++;
    if (i >= nStrings)
        i = 0;

    //* Format and return string
    sprintf(achPrivate[i],"%d'%4.1f\"",feet,iRemainder);
    return achPrivate[i];
} /* szFromInches() */


/***    DrawDriveway - plot stock price
 *
 *      Entry:
 *          hdc   - DC to paint
 *          prect - Bounding rectangle to paint
 *
 *      Exit:
 *          Driveway drawn
 */
VOID DrawDriveway(HDC hdc,RECT *prect)
{
//* Compute scale for converting inches to pels
//  Use smaller of two factors to ensure drawing fits and to achieve
//  correct perspective.
    yFactor = (float)(prect->bottom - prect->top - (2*cyMargin)) /
	      (float)(cyiMax);
    xFactor = (float)(prect->right - prect->left - (2*cxMargin)) /
	      (float)(cxiMax);
    scale = (xFactor > yFactor ? yFactor : xFactor);
    cyFlip = prect->bottom - cyMargin;

    if (!fCountedPavers) {
        //* Dump measurements
        fprintf(fout,"Distance from North edge to Center line: %s\n",szFromInches(cxiNorthToCenter));
        fprintf(fout,"Distance from Center line to South edge: %s\n",szFromInches(cxiSouthToCenter));
        fprintf(fout,"\n");
        fprintf(fout,"Perimeter outer radius: %10s\n",szFromInches(radiusi3Outer));
        fprintf(fout,"Perimeter inner radius: %10s\n",szFromInches(radiusi3Inner));
        fprintf(fout,"\n");
        fprintf(fout,"Middle outer radius:    %10s\n",szFromInches(radiusi2Outer));
        fprintf(fout,"Middle inner radius:    %10s\n",szFromInches(radiusi2Inner));
        fprintf(fout,"\n");
        fprintf(fout,"Drain outer radius:     %10s\n",szFromInches(radiusiDrainOuter));
        fprintf(fout,"Drain inner radius:     %10s\n",szFromInches(radiusiDrainInner));
        fprintf(fout,"\n");
    }

    //* Draw center line due East to edge of driveway
    hpenGuide = CreatePen(PS_SOLID,1,RGB(0x00,0x00,0xFF));
    hpenSave = SelectObject(hdc,hpenGuide);
    xi = cxiNorthToCenter;
    yi = cyiCenterToNorthEdge;
    MoveToEx(hdc,sx(xi),sy(yi),NULL);
    yi = radiusi3Outer;
    LineTo(hdc,sx(xi),sy(yi));
    SelectObject(hdc,hpenSave); // Restore original pen

    //* Draw north edge
    xi = 0.0;
    yi = cyiCenterToNorthEdge;
    MoveToEx(hdc,sx(xi),sy(yi),NULL);
    yi += cyiNorthEdge;
    LineTo(hdc,sx(xi),sy(yi));
    LineTo(hdc,sx(xi + ciPaver),sy(yi));  // east paver edge
    LineTo(hdc,sx(xi + ciPaver),sy(cyiCenterToNorthEdge)); // south paver edge
    addPavers("North Edge",cyiNorthEdge); //** COMPUTE PAVERS

    /**********************
     ** Draw outside arc **
     **********************/
    // Bounding box of full circle
    radiusi = radiusi3Outer;
    xiLeft   = cxiNorthToCenter - radiusi;
    yiTop    = radiusi;
    xiRight  = xiLeft + 2*radiusi;
    yiBottom = -radiusi;

    // NOTE: Arc is drawn counterclockwise
    // SE corner
    xiStartArc = cxiNorthToCenter + cxiSouthToCenter - cxiESEdge;
    yiStartArc = cyiCenterToSouthEdge + cyiSouthEdge;

    // NE corner
    xiEndArc = xi;
    yiEndArc = yi;

    f = Arc( 
	hdc,           // handle to device context 
	sx(xiLeft),     // x-coordinate of bounding rectangle’s upper-left corner 
	sy(yiTop),      // y-coordinate of bounding rectangle’s upper-left corner 
	sx(xiRight),    // x-coordinate of bounding rectangle’s lower-right corner 
	sy(yiBottom),   // y-coordinate of bounding rectangle’s lower-right corner 
	
	sx(xiStartArc), // first radial ending point 
	sy(yiStartArc), // first radial ending point 
	
	sx(xiEndArc),   // second radial ending point 
	sy(yiEndArc)    // second radial ending point 
	); 

    countArcPavers("perimeter arc",radiusi,xiStartArc,yiStartArc,xiEndArc,yiEndArc);

    //* Draw inner arc to indicate 6" paver border
    f = Arc( 
	hdc,           // handle to device context 
    sx(xiLeft + ciPaver),     // x-coordinate of bounding rectangle's upper-left corner
    sy(yiTop - ciPaver),      // y-coordinate of bounding rectangle's upper-left corner
    sx(xiRight - ciPaver),    // x-coordinate of bounding rectangle's lower-right corner
    sy(yiBottom + ciPaver),   // y-coordinate of bounding rectangle's lower-right corner
	
	sx(xiStartArc - ciPaver), // first radial ending point 
	sy(yiStartArc - ciPaver), // first radial ending point 
	
	sx(xiEndArc + ciPaver),   // second radial ending point 
	sy(yiEndArc - ciPaver)    // second radial ending point 
	); 

	
    /*********************
     ** Draw more edges **
     *********************/
    
    //* Draw EastSouth edge
    xi = xiStartArc;
    yi = yiStartArc;
    MoveToEx(hdc,sx(xi),sy(yi),NULL);
    xi += cxiESEdge;
    LineTo(hdc,sx(xi),sy(yi));
    LineTo(hdc,sx(xi),sy(yi-ciPaver));
    LineTo(hdc,sx(xi-cxiESEdge),sy(yi-ciPaver));
    LineTo(hdc,sx(xi-cxiESEdge),sy(yi));
    MoveToEx(hdc,sx(xi),sy(yi),NULL);
    addPavers("East Edge adjacent to garage",cxiESEdge); //** COMPUTE PAVERS

    //* Draw South edge
    yi -= cyiSouthEdge;
    LineTo(hdc,sx(xi),sy(yi));

    //* Draw West edges
    xi = cxiWestEdge - cxiWSEdge;   // Draw southern west edge
    LineTo(hdc,sx(xi),sy(yi));

    xi = cxiWNEdge;		    // Draw northern west edge
    yi = cyiCenterToNorthEdge;
    MoveToEx(hdc,sx(xi),sy(yi),NULL);
    xi = 0.0;
    LineTo(hdc,sx(xi),sy(yi));

    //* Draw granite sawtooths radial lines
    for (i=0; i<nipt; i++) {
	xi = cxiNorthToCenter;
	yi = 0.0;
	MoveToEx(hdc,sx(xi),sy(yi),NULL);
	xi = aipt[i].x;
	yi = aipt[i].y;
        hpenGuide = CreatePen(PS_SOLID,1,RGB(0xFF,0x00,0x00));
	hpenSave = SelectObject(hdc,hpenGuide);
        LineTo(hdc,sx(xi),sy(yi));
	SelectObject(hdc,hpenSave); // Restore original pen

	//* Extend line to intersect drain arc (use Law of Sines)
	xi -= cxiNorthToCenter;
	ri = (float)sqrt(xi*xi + yi*yi);    //* radius of sawtooth
	sinx = xi*1.0f/ri;		    //* sin(90 degrees) == 1;
	siny = yi*1.0f/ri;
	xi2 = sinx*radiusiDrainInner + cxiNorthToCenter;
	yi2 = siny*radiusiDrainInner;
	aipt[i].xiDrain = xi2;
	aipt[i].yiDrain = yi2;

	xi = aipt[i].x;
	yi = aipt[i].y;
	MoveToEx(hdc,sx(xi),sy(yi),NULL);
        LineTo(hdc,sx(xi2),sy(yi2));

	li = (float)sqrt(sqr(xi2-xi) + sqr(yi2-yi));
	sprintf(ach,"sawtooth #%d: outer=%8s,%8s  inner=%8s,%8s",
	    i+1,
	    szFromInches(xi2),szFromInches(yi2),
	    szFromInches(xi) ,szFromInches(yi)
	);
        addPavers(ach,li); //** COMPUTE PAVERS
    }

    //* Compute chord lengths at drain of sawtooth extensions
    if (!fCountedPavers) {
	fprintf(fout,"\n");
    }
    for (i=0; i<(nipt-1); i++) {
	li = (float)sqrt(
			 sqr(aipt[i].xiDrain-aipt[i+1].xiDrain) + 
			 sqr(aipt[i].yiDrain-aipt[i+1].yiDrain)
			);
	aipt[i].ichordOuter = li;   //* save chord length
        if (!fCountedPavers) {
	    fprintf(fout,"granite chord %ch: %s\n",'A'+i,szFromInches(li));
	}
    }

    //* Draw two radial lines to north of sawtooth, connecting to drain
    if (!fCountedPavers)
	fprintf(fout,"\nNORTH INNER RADIAL LINES: West edge to Drain arc\n");
    tesselate(
	hdc,
	radiusiDrainInner,		// outer radius
	cyiCenterToNorthEdge,		// inner radius
	121.0f,				// length of chord
	aipt[0].xiDrain,                // left-most (northern most) chord start
	0.0,				// right-most (southern most) chord end
	cyiCenterToNorthEdge,		// y minimum
	-1				// Counter-clockwise
    );

    /*********************
     ** Draw middle arc **
     *********************/
    // Bounding box of full circle
    radiusi  = radiusi2Outer;
    xiLeft   = cxiNorthToCenter - radiusi;
    yiTop    = radiusi;
    xiRight  = xiLeft + 2*radiusi;
    yiBottom = -radiusi;

    // NOTE: Arc is drawn counterclockwise
    // SE corner - compute intersection with South edge
    ihyp = radiusi;
    xi = cxiSouthToCenter;
    if (ihyp > xi) {	// arc hits south edge of driveway
	yiStartArc = (float)sqrt(ihyp*ihyp - xi*xi);
        xiStartArc = cxiNorthToCenter + xi;
    }
    else {		// arc hits west edge of driveway
	yi = cyiCenterToSouthEdge;
	xiStartArc = cxiNorthToCenter + ihyp;
    }

    // NE corner
    ihyp = radiusi;
    xi = cxiNorthToCenter;
    if (ihyp> xi) {    // arc hits north edge of driveway
	yiEndArc = (float)sqrt(ihyp*ihyp - xi*xi);
	if (yiEndArc < cyiCenterToNorthEdge)
	    yiEndArc = cyiCenterToNorthEdge;
        xiEndArc = cxiNorthToCenter - xi;
    }
    else {		    // arc hits west edge of driveway
	yiEndArc = cyiCenterToNorthEdge;
	xiEndArc = cxiNorthToCenter - ihyp;
    }
    
    f = Arc( 
	hdc,           // handle to device context 
	sx(xiLeft),     // x-coordinate of bounding rectangle’s upper-left corner 
	sy(yiTop),      // y-coordinate of bounding rectangle’s upper-left corner 
	sx(xiRight),    // x-coordinate of bounding rectangle’s lower-right corner 
	sy(yiBottom),   // y-coordinate of bounding rectangle’s lower-right corner 
	
	sx(xiStartArc), // first radial ending point 
	sy(yiStartArc), // first radial ending point 
	
	sx(xiEndArc),   // second radial ending point 
	sy(yiEndArc)    // second radial ending point 
	);

    countArcPavers("middle arc",radiusi,xiStartArc,yiStartArc,xiEndArc,yiEndArc);

    //* Draw inner arc to indicate 6" paver border
    f = Arc( 
	hdc,           // handle to device context 
	sx(xiLeft + ciPaver),     // x-coordinate of bounding rectangle’s upper-left corner 
	sy(yiTop - ciPaver),      // y-coordinate of bounding rectangle’s upper-left corner 
	sx(xiRight - ciPaver),    // x-coordinate of bounding rectangle’s lower-right corner 
	sy(yiBottom + ciPaver),   // y-coordinate of bounding rectangle’s lower-right corner 
	
	sx(xiStartArc - ciPaver), // first radial ending point 
	sy(yiStartArc - ciPaver), // first radial ending point 
	
	sx(xiEndArc + ciPaver),   // second radial ending point 
	sy(yiEndArc - ciPaver)    // second radial ending point 
	); 

    /********************
     ** Draw Drain arc **
     ********************/
    // Bounding box of full circle
    radiusi  = radiusiDrainInner;
    xiLeft   = cxiNorthToCenter - radiusi;
    yiTop    = radiusi;
    xiRight  = xiLeft + 2*radiusi;
    yiBottom = -radiusi;

    // NOTE: Arc is drawn counterclockwise
    // SE corner - compute intersection with South edge
    ihyp = radiusi;
    xi = cxiSouthDrain;
    if (ihyp > xi) {	// arc hits south edge of driveway
	yiStartArc = (float)sqrt(ihyp*ihyp - xi*xi);
        xiStartArc = cxiNorthToCenter + xi;
    }
    else {		// arc hits west edge of driveway
	yi = cyiCenterToSouthEdge;
	xiStartArc = cxiNorthToCenter + ihyp;
    }

    // NE corner
    ihyp = radiusi;
    xi = cxiNorthDrain;
    if (ihyp > xi) {    // arc hits north edge of driveway
	yiEndArc = (float)sqrt(ihyp*ihyp - xi*xi);
        xiEndArc = cxiNorthToCenter - xi;
    }
    else {		    // arc hits west edge of driveway
	yiEndArc = cyiCenterToNorthEdge;
	xiEndArc = cxiNorthToCenter - ihyp;
    }
    
    hpenGuide = CreatePen(PS_SOLID,1,RGB(0xFF,0x00,0x00));
    hpenSave = SelectObject(hdc,hpenGuide);
    f = Arc( 
	hdc,           // handle to device context 
	sx(xiLeft),     // x-coordinate of bounding rectangle’s upper-left corner 
	sy(yiTop),      // y-coordinate of bounding rectangle’s upper-left corner 
	sx(xiRight),    // x-coordinate of bounding rectangle’s lower-right corner 
	sy(yiBottom),   // y-coordinate of bounding rectangle’s lower-right corner 
	
	sx(xiStartArc), // first radial ending point 
	sy(yiStartArc), // first radial ending point 
	
	sx(xiEndArc),   // second radial ending point 
	sy(yiEndArc)    // second radial ending point 
	);

    //* Draw outer arc to indicate 6.5" drain width
    f = Arc( 
	hdc,           // handle to device context 
	sx(xiLeft - ciDrain),     // x-coordinate of bounding rectangle’s upper-left corner 
	sy(yiTop + ciDrain),      // y-coordinate of bounding rectangle’s upper-left corner 
	sx(xiRight + ciDrain),    // x-coordinate of bounding rectangle’s lower-right corner 
	sy(yiBottom - ciDrain),   // y-coordinate of bounding rectangle’s lower-right corner 
	
	sx(xiStartArc + ciDrain), // first radial ending point 
	sy(yiStartArc + ciDrain), // first radial ending point 
	
	sx(xiEndArc - ciDrain),   // second radial ending point 
	sy(yiEndArc + ciDrain)    // second radial ending point 
	); 
    SelectObject(hdc,hpenSave); // Restore original pen

    countArcPavers("ESTIMATE drain arc",radiusi,xiStartArc,yiStartArc,xiEndArc,yiEndArc);

    /*************************
     ** Draw basketball key **
     *************************/

    //* Draw the lane
    hpenGuide = CreatePen(PS_SOLID,1,RGB(0xFF,0x00,0x00));
    hpenSave = SelectObject(hdc,hpenGuide);
    xi = 0.0;
    yi = cyiCenterToNorthEdge + cyiBBWestToCenter + ciBBKeyDiameter/2;
    MoveToEx(hdc,sx(xi),sy(yi),NULL);
    xi += cxiBBLane;
    LineTo(hdc,sx(xi),sy(yi));		// East edge of lane
    //* banana: Save this point to start northern most outer radial line
    xi2 = xi;
    yi2 = yi;

    yi -= ciBBKeyDiameter;
    LineTo(hdc,sx(xi),sy(yi));		// South edge of lane

    xi = 0.0;
    LineTo(hdc,sx(xi),sy(yi));		// South edge of lane

    //* Draw the key (a circle)
    radiusi  = ciBBKeyDiameter/2;
    xiLeft   = cxiBBLane - radiusi;
    xiRight  = cxiBBLane + radiusi;
    yiTop    = cyiCenterToNorthEdge + cyiBBWestToCenter + radiusi;
    yiBottom = cyiCenterToNorthEdge + cyiBBWestToCenter - radiusi;
    
    xiStartArc = cxiBBLane;
    yiStartArc = yiBottom;

    xiEndArc = xiStartArc;
    yiEndArc = yiStartArc;

    f = Arc( 
	hdc,           // handle to device context 
	sx(xiLeft),     // x-coordinate of bounding rectangle’s upper-left corner 
	sy(yiTop),      // y-coordinate of bounding rectangle’s upper-left corner 
	sx(xiRight),    // x-coordinate of bounding rectangle’s lower-right corner 
	sy(yiBottom),   // y-coordinate of bounding rectangle’s lower-right corner 
	
	sx(xiStartArc), // first radial ending point 
	sy(yiStartArc), // first radial ending point 
	
	sx(xiEndArc),   // second radial ending point 
	sy(yiEndArc)    // second radial ending point 
	);
    SelectObject(hdc,hpenSave); // Restore original pen

    //* Draw N-S line to center of key per Bob's layout (too far?)
    hpenGuide = CreatePen(PS_SOLID,1,RGB(0x00,0xFF,0x00));
    hpenSave = SelectObject(hdc,hpenGuide);
    xi = 0;
    yi = cyiCenterToNorthEdge + cyiBBWestToCenterBob;
    MoveToEx(hdc,sx(xi),sy(yi),NULL);
    xi = cxiBBNorthToCenterBob;
    LineTo(hdc,sx(xi),sy(yi));
    SelectObject(hdc,hpenSave); // Restore original pen

    /***************************
     ** Tesselate "big" tiles **
     ***************************/

    //* WARNING: Depends upon xi2/yi2 set above at "banana"

    //* Extend corner of key from center point to outer edge (law of sines)
    xi2 -= cxiNorthToCenter;
    ri = (float)sqrt(xi2*xi2 + yi2*yi2);//* radius back to center point
    sinx = xi2*1.0f/ri;		    //* sin(90 degrees) == 1;
    xi = sinx*radiusi3Inner + cxiNorthToCenter;

    if (!fCountedPavers)
        fprintf(fout,"\nPERIMETER RADIAL LINES: Perimeter arc to middle arc\n");
    tesselate(
	hdc,
	radiusi3Inner,	    // outer radius
	radiusi2Outer,	    // inner radius
	138.0	,	    // length of chord at outer radius
	xi,		    // left-most (northern most) chord start
	cxiWestEdge,	    // right-most (southern most) chord end
	0.0,
	1       // Clockwise
    );

    if (!fCountedPavers)
	fprintf(fout,"\nMIDDLE RADIAL LINES: Middle arc to Drain arc\n");
    tesselate(
	hdc,
	radiusi2Inner,	    // outer radius
	radiusiDrainOuter,  // inner radius
	134.0,		    // length of chord at outer radius
	cxiBBLane + ciBBKeyDiameter/4, // left-most (northern most) chord start
	cxiWestEdge,	    // right-most (southern most) chord end
	0.0,
	1       // Clockwise
    );

    //* Don't count twice
    fCountedPavers = 1;

} /* DrawDriveway() */


void tesselate(
    HDC	    hdc,	    // dc
    float   iRadiusOuter,   // outer radius
    float   iRadiusInner,   // inner radius
    float   iChord,	    // length of chord at outer radius
    float   xiStart,	    // left-most (northern most) chord start
    float   xiEnd,	    // right-most (southern most) chord end
    float   yiEnd,      // Lowest chord start
    int     sign        // 1 => Draw clockwise; -1 => counter clockwise
    )
{
    float   angle;
    float   angleInc;
    int	    cLine=0;
    float   xi2Last;
    float   yi2Last;
    float   iInnerChord;    // Length of inner chord

    //* Compute initial angle (due North is 0) and x,y coordinate
    xi = xiStart - cxiNorthToCenter;  //* x coord of triangle
    yi = (float)sqrt(iRadiusOuter*iRadiusOuter - xi*xi); //* cy of triangle
    angle = 3.14159265f - (float)asin(yi/iRadiusOuter);

    //* Compute incremental angle based on outer chord
    angleInc = sign*2.0f*(float)asin((iChord/2)/iRadiusOuter);
    
    //* Draw radial lines
    xi = xiStart;
    while ( (sign>0 ? (xi <= xiEnd) : (xiEnd <= xi)) && (yi >= yiEnd)) {
	MoveToEx(hdc,sx(xi),sy(yi),NULL);
	//* Save last inner arc coordinate to compute inner chord
	xi2Last = xi2;
	yi2Last = yi2;
	//* Computer inner arc coordinate
	xi2 = iRadiusInner*(float)cos(angle) + cxiNorthToCenter;
	yi2 = iRadiusInner*(float)sin(angle);
	if ((sign>0) || (xi != xiStart)) {  // Skip first line on counterclockwise
	    LineTo(hdc,sx(xi2),sy(yi2));

	    cLine++;
	    if (cLine == 2) {
		//* Have two points on inner arc, can computer inner chord
		iInnerChord = (float)sqrt(sqr(xi2-xi2Last) + sqr(yi2-yi2Last));
	    }

	    li = (float)sqrt(sqr(xi2-xi) + sqr(yi2-yi));
	    sprintf(ach,"radial line: outer=%8s,%8s  inner=%8s,%8s",
		szFromInches(xi) ,szFromInches(yi),
		szFromInches(xi2),szFromInches(yi2)
	    );
	    addPavers(ach,li); //** COMPUTE PAVERS
	}
	
	//* Advance position
	angle -= angleInc;
	xi = iRadiusOuter*(float)cos(angle) + cxiNorthToCenter;
	yi = iRadiusOuter*(float)sin(angle);
    }

    if (!fCountedPavers) {	    //* Only count once
        fprintf(fout,"outer chord length: %s\n",szFromInches(iChord));
        fprintf(fout,"inner chord length: %s\n",szFromInches(iInnerChord));
    }
} /* tesselate */


/***	addPavers - account for paver needs
 *
 *
 */
void    addPavers(char *sz, float li)
{
    long    n;
    if (!fCountedPavers) {	    //* Only count once
    n = (long)((li+5.9f)/6.0f);
    totalLinealInches += li;
	nPavers += n;
    fprintf(fout,"%61s, %9s, %5ld\n",sz,szFromInches(li),n);
    }
} /* addPavers() */


/***	countArcPavers - count pavers around an arc
 *
 *
 */
void   countArcPavers(char *sz,
		      float radiusi,
		      float xiStartArc,
		      float yiStartArc,
		      float xiEndArc,
		      float yiEndArc)
{
    float xi;
    float yi;
    float angleR;
    float angleL;
    float angle;
    
    xi = xiStartArc - cxiNorthToCenter;  //* x coord of triangle
    yi = (float)sqrt(radiusi*radiusi - xi*xi); //* y of triangle
    angleR = (float)asin(yi/radiusi);

    xi = xiEndArc - cxiNorthToCenter;  //* x coord of triangle
    yi = (float)sqrt(radiusi*radiusi - xi*xi); //* y of triangle
    angleL = (float)asin(yi/radiusi);

    angle = 3.14159265359f - (angleL + angleR);
    addPavers(sz,angle*radiusi);
} /* countArcPavers() */    

  
  /***	PrintView - Print stock view
 *
 *      Entry:
 *	    hwnd - parent window, for dialogs
 *
 *      Exit:
 */
VOID   PrintView(HWND hwnd)
{
    HDC      hdc;
    PRINTDLG pd;
    RECT     rect;

    // Initialize PRINTDLG structure

    pd.lStructSize = sizeof(PRINTDLG);
    pd.hwndOwner = hwnd;
    pd.hDevMode = (HANDLE) NULL;
    pd.hDevNames = (HANDLE) NULL;
    // Only one page, so no page #'s!
    pd.Flags = PD_RETURNDC |
    	       PD_NOPAGENUMS |
    	       PD_NOSELECTION |
    	       PD_USEDEVMODECOPIES;
    pd.nFromPage = 0;
    pd.nToPage = 0;
    pd.nMinPage = 0;
    pd.nMaxPage = 0;
    pd.nCopies = 0;
    pd.hInstance = (HANDLE) NULL;

    // Prompt for print info, and print if successfull

    if (PrintDlg(&pd) != 0) {

	hdc = pd.hDC;

	rect.left   = 0;
	rect.top    = 0;
	rect.right  = GetDeviceCaps(hdc,HORZRES);
	rect.bottom = GetDeviceCaps(hdc,VERTRES);

	Escape(hdc,STARTDOC,strlen(g.achTitle),g.achTitle,NULL);

	DrawDriveway(hdc,&rect);

	Escape(hdc,NEWFRAME,0,NULL,NULL);
	Escape(hdc,ENDDOC,0,NULL,NULL);
	DeleteDC(hdc);
	if (pd.hDevMode)
       GlobalFree(pd.hDevMode);
	if (pd.hDevNames)
       GlobalFree(pd.hDevNames);
    }
    else {
	// Some kind of failure
	// BUGBUG 10-Feb-1992 bens Handle PrintDlg(...) failure
    }
}


/***    WndProc - Main Window Procedure
 *
 */
long FAR PASCAL WndProc(HWND hwnd,UINT msg,UINT wParam,LONG lParam)
{
    HDC         hdc;
    RECT        rect;
    PAINTSTRUCT ps;


    switch(msg) {
        case WM_CREATE:
            return 0;

        case WM_COMMAND:
            DoCommand(hwnd,wParam,lParam);
            return 0;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);
	    DrawDriveway(hdc,&rect);
            EndPaint(hwnd,&ps);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd,msg,wParam,lParam);
}
