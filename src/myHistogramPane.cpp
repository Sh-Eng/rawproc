
#include "myHistogramPane.h"
#include <math.h>
#include <algorithm>
#include <wx/clipbrd.h>
#include "myConfig.h"
#include "util.h"


myHistogramPane::myHistogramPane(wxWindow* parent, const wxPoint &pos, const wxSize &size) :
 wxWindow(parent, wxID_ANY, pos, size, wxBORDER_SUNKEN)
{
	int fr=0, fg=0, fb=0;
	blankpic = true;
	SetDoubleBuffered(true);
	//t = new wxTimer(this);
	unsigned hm = 0;
	wscale = 1.0;
	xorigin = 0;
	yorigin = 0;
	hmax = 0;
	hscale = 0;
	ord = 1;
	EVaxis = false;
	Unbounded = false;
	db = NULL;
	
	r = NULL; g = NULL; b = NULL;
	rlen=0; glen=0; blen=0;
	display_channels = CHANNEL_RGB;
		
	MouseX = 0; MouseY=0;
	pressedDown = false;
	inwindow = false;

	Bind(wxEVT_MOTION, &myHistogramPane::mouseMoved, this);
	Bind(wxEVT_LEFT_DOWN, &myHistogramPane::mouseDown, this);
	Bind(wxEVT_LEFT_UP, &myHistogramPane::mouseReleased, this);
	Bind(wxEVT_LEFT_DCLICK, &myHistogramPane::mouseDoubleClicked, this);
	//Bind(wxEVT_RIGHT_DOWN, &myHistogramPane::rightClick, this);
	Bind(wxEVT_ENTER_WINDOW, &myHistogramPane::mouseEnterWindow, this);
	Bind(wxEVT_LEAVE_WINDOW, &myHistogramPane::mouseLeftWindow, this);
	//EVT_CHAR(myHistogramPane::keyPressed)
	Bind(wxEVT_KEY_DOWN, &myHistogramPane::keyPressed, this);
	//EVT_KEY_UP(myHistogramPane::keyReleased)
	Bind(wxEVT_MOUSEWHEEL, &myHistogramPane::mouseWheelMoved, this);
	Bind(wxEVT_SIZE, &myHistogramPane::OnSize, this);
	Bind(wxEVT_PAINT, &myHistogramPane::paintEvent, this);
}

/*
myHistogramPane::myHistogramPane(wxWindow* parent, gImage &dib, const wxPoint &pos, const wxSize &size) :
 wxWindow(parent, wxID_ANY, pos, size, wxBORDER_SUNKEN)
{
	blankpic = false;
	//SetDoubleBuffered(true);
	//t = new wxTimer(this);
	unsigned hm = 0;
	wscale = 1.0;
	xorigin = 0;
	yorigin = 0;
	hmax = 0;
	hscale = 0;
	ord = 1;
	EVaxis = false;
	Unbounded = false;
	display_channels = CHANNEL_RGB;
	db = dib;

	//not needed, for now; renders 'c' key command inop
	//smalldata = dib.Histogram();
	
	rdata = dib.Histogram(CHANNEL_RED, hm);
	if (hmax < hm) hmax = hm;
	if (hscale < rdata.size()) hscale = rdata.size();
	rlen=rdata.size();
	r = new wxPoint[rlen];
	for (unsigned i=0; i<rlen; i++) r[i] = wxPoint(i,rdata[i]);

	gdata = dib.Histogram(CHANNEL_GREEN, hm);
	if (hmax < hm) hmax = hm;
	if (hscale < gdata.size()) hscale = gdata.size();
	glen=gdata.size();
	g = new wxPoint[glen];
	for (unsigned i=0; i<rlen; i++) g[i] = wxPoint(i,gdata[i]);

	bdata = dib.Histogram(CHANNEL_BLUE, hm);
	if (hmax < hm) hmax = hm;
	if (hscale < bdata.size()) hscale = bdata.size();
	blen=bdata.size();
	b = new wxPoint[blen];
	for (unsigned i=0; i<blen; i++) b[i] = wxPoint(i,bdata[i]);

	MouseX = 0; MouseY=0;
	SetInitialSize(wxSize(500,400));
	pressedDown = false;
	inwindow = false;


	Bind(wxEVT_MOTION, &myHistogramPane::mouseMoved, this);
	Bind(wxEVT_LEFT_DOWN, &myHistogramPane::mouseDown, this);
	Bind(wxEVT_LEFT_UP, &myHistogramPane::mouseReleased, this);
	Bind(wxEVT_LEFT_DCLICK, &myHistogramPane::mouseDoubleClicked, this);
	//Bind(wxEVT_RIGHT_DOWN, &myHistogramPane::rightClick, this);
	Bind(wxEVT_ENTER_WINDOW, &myHistogramPane::mouseEnterWindow, this);
	Bind(wxEVT_LEAVE_WINDOW, &myHistogramPane::mouseLeftWindow, this);
        //EVT_CHAR(myHistogramPane::keyPressed)
        Bind(wxEVT_KEY_DOWN, &myHistogramPane::keyPressed, this);
        //EVT_KEY_UP(myHistogramPane::keyReleased)
        Bind(wxEVT_MOUSEWHEEL, &myHistogramPane::mouseWheelMoved, this);
        Bind(wxEVT_SIZE, &myHistogramPane::OnSize, this);

    	Bind(wxEVT_PAINT, &myHistogramPane::paintEvent, this);
	
	//Refresh();
}
*/

myHistogramPane::~myHistogramPane()
{
	if (r) delete[] r;
	if (g) delete[] g;
	if (b) delete[] b;
}

void myHistogramPane::OnSize(wxSizeEvent& event) 
{
	event.Skip();
	Refresh();
}

 
void myHistogramPane::paintEvent(wxPaintEvent & evt)
{
	wxPaintDC dc(this);
	render(dc);
}
 
void myHistogramPane::paintNow()
{
	wxClientDC dc(this);
	render(dc);
}

void myHistogramPane::BlankPic()
{
	blankpic = true;
	Refresh();
	
}

void myHistogramPane::RecalcHistogram()
{
	blankpic = false;
	hmax = 0;
	rlen=hscale; glen=hscale; blen=hscale;

	if (Unbounded)
		histogram = db->Histogram(hscale, zerobucket, onebucket);
	else
		histogram = db->Histogram(hscale);
	
	bool rmax=false, gmax=false, bmax=false;
	for (unsigned i=hscale-1; i>0; i--) {
		if (!rmax) {if (histogram[i].r == 0) rlen--; else rmax = true;}
		if (!gmax) {if (histogram[i].g == 0) glen--; else gmax = true;}
		if (!bmax) {if (histogram[i].b == 0) blen--; else bmax = true;}
	}
	
	if (r) delete[] r;
	if (g) delete[] g;
	if (b) delete[] b;
	r = new wxPoint[hscale];
	g = new wxPoint[hscale];
	b = new wxPoint[hscale];
	

	//parm histogram.clipbuckets: number of buckets to eliminate on both ends of the histogram in calculating the max height.  Default=0
	unsigned clipbuckets = atoi(myConfig::getConfig().getValueOrDefault("histogram.clipbuckets","0").c_str()); 
	unsigned lower = clipbuckets;
	unsigned upper = (hscale-1)-clipbuckets;
	
	for (unsigned i=0; i<hscale; i++) {
		r[i] = wxPoint(i, histogram[i].r);
		g[i] = wxPoint(i, histogram[i].g);
		b[i] = wxPoint(i, histogram[i].b);
		if (i >= lower & i <= upper) {
			if (hmax < histogram[i].r) hmax = histogram[i].r;
			if (hmax < histogram[i].g) hmax = histogram[i].g;
			if (hmax < histogram[i].b) hmax = histogram[i].b;
		}
	}
	Refresh();

}

void myHistogramPane::SetPic(gImage *dib, unsigned scale)
{	
	db = dib;
	hscale = scale;
	//rlen=scale; glen=scale; blen=scale;
	RecalcHistogram();	
}

void myHistogramPane::SetChannel(GIMAGE_CHANNEL channel)
{
	display_channels = channel;
	Refresh();
}


void myHistogramPane::render(wxDC&  dc)
{
	int w, h;
	int hx;
	GetSize(&w, &h);
	w-=6;
	h-=6;

	dc.Clear();
	if (blankpic) return;

	//set up colors:
	int fontsize = atoi(myConfig::getConfig().getValueOrDefault("app.parameters.fontsize","10").c_str());
	dc.SetTextForeground(wxString2wxColour(wxString(myConfig::getConfig().getValueOrDefault("app.parameters.fontcolor","0"))));
	wxFont font(wxFontInfo(fontsize).Family(wxFONTFAMILY_SWISS));
	dc.SetFont(font);

	//parm app.parameters.linecolor: integer RGB triplet or single of 0-255 (e.g., 128,128,128 or just 128 for gray) to specify default  color to be used to draw lines in the parameter pane. Some tools override this, look for "linecolor" in the Properties filter. Default=0
	wxString applinecolorstring = wxString(myConfig::getConfig().getValueOrDefault("app.parameters.linecolor","0"));

	//parm histogram.ev.linecolor: integer RGB triplet or single of 0-255 (e.g., 128,128,128 or just 128 for gray) to specify default  color to be used to draw EV lines in the parameter pane.  Default=app.parameters.linecolor
	wxColour linecolor = wxString2wxColour(wxString(myConfig::getConfig().getValueOrDefault("histogram.ev.linecolor",applinecolorstring.ToStdString())));

	//parm histogram.cursor.linecolor: integer RGB triplet or single of 0-255 (e.g., 128,128,128 or just 128 for gray) to specify default  color to be used to draw the histogram cursor. Some tools override this, look for "linecolor" in the Properties filter. Default=app.parameters.linecolor
	wxColour cursorcolor = wxString2wxColour(wxString(myConfig::getConfig().getValueOrDefault("histogram.cursor.linecolor",applinecolorstring.ToStdString())));


	int lineheight = wxSize(dc.GetTextExtent("l")).GetHeight();


	//go to histogram coordinates:
	if (EVaxis) {
		dc.SetLogicalScale(((double) w / (double) hscale)* wscale, ((double) (h-lineheight)/ (double) hmax));// * wscale);
		dc.SetDeviceOrigin (xorigin, h-yorigin-lineheight);
	}
	else {
		dc.SetLogicalScale(((double) w / (double) hscale)* wscale, ((double) h/ (double) hmax)); // * wscale);
		dc.SetDeviceOrigin (xorigin, h-yorigin);
	}
	dc.SetAxisOrientation(true,true);

//	if something I haven't defined yet {
		wxColour boundcolor = wxString2wxColour(wxString(myConfig::getConfig().getValueOrDefault("histogram.bound.color","128")));
		wxPen origpen = dc.GetPen();
		wxBrush origbrush = dc.GetBrush();
		dc.SetPen(wxPen(boundcolor));
		dc.SetBrush(wxBrush(boundcolor));

		if (Unbounded) 
			dc.DrawRectangle(zerobucket,0,onebucket,hmax);
		else
			dc.DrawRectangle(0,0,hscale,hmax);

		dc.SetPen(origpen);
		dc.SetBrush(origbrush);
//	}

	wxPoint * frontcolor;
	
	unsigned order = ord;
	for (unsigned i=0; i<3; i++) {
		if (order == 1) {
			if (r) {
				if ((display_channels == CHANNEL_RGB) | (display_channels == CHANNEL_RED)) {
					dc.SetPen(wxPen(wxColour(255,0,0),1));
					dc.DrawLines(rlen,r,0,0);
					if (i==2) frontcolor = r;
				}
			}
		}
		if (order == 2) {
			if (g) {
				if ((display_channels == CHANNEL_RGB) | (display_channels == CHANNEL_GREEN)) {
					dc.SetPen(wxPen(wxColour(0,255,0),1));
					dc.DrawLines(glen,g,0,0);
					if (i==2) frontcolor = g;
				}
			}
		}
		if (order == 3) {
			if (b) {
				if ((display_channels == CHANNEL_RGB) | (display_channels == CHANNEL_BLUE)) {
					dc.SetPen(wxPen(wxColour(0,0,255),1));
					dc.DrawLines(blen,b,0,0);
					if (i==2) frontcolor = b;
				}
			}
		}
		order++;
		if (order>3) order=1;
	}

	//parm histogram.ev.zero: Tone in the 0.0-1.0 scale to plot as EV0.  Default: 0.18
	EV0 = atof(myConfig::getConfig().getValueOrDefault("histogram.ev.zero","0.18").c_str());
	//parm histogram.ev.range: +/- range to plot EV.  Default: 3.0 (-/+ 3 stops)
	float EVrange = atof(myConfig::getConfig().getValueOrDefault("histogram.ev.range","3.0").c_str());
	//parm histogram.ev.increment: Step increment for EV plot.  Default: 1.0
	float EVinc = atof(myConfig::getConfig().getValueOrDefault("histogram.ev.increment","1.0").c_str());

/*
	if (EVaxis) {
		dc.SetPen(wxPen(linecolor, 1, wxPENSTYLE_SOLID ));
		dc.DrawLine(hscale * EV0, 0, hscale * EV0, hmax);
		dc.SetPen(wxPen(linecolor, 1, wxPENSTYLE_LONG_DASH ));
		for (float ev=-EVrange; ev<=EVrange; ev+=EVinc) {
			dc.DrawLine(hscale * EV0*pow(2.0, ev), 0, hscale * EV0*pow(2.0, ev), hmax);

		}

	}
*/




	//marker lines:
	dc.SetPen(wxPen(cursorcolor));
	int mlx = dc.DeviceToLogicalX(wxCoord(MouseX));
	float mlx_ev = hscale * mlx*pow(2.9, 1.0/mlx);
	
	unsigned mly=0;
	//if (mlx > 0 & mlx < hscale) { 
	//	mly = wxCoord(frontcolor[mlx].y);
	//	dc.DrawLine(0,mly,hscale,mly);
	//}

	//return to window coords:
	dc.SetLogicalScale(1.0, 1.0);
	dc.SetDeviceOrigin (0, 0);
	dc.SetAxisOrientation(true,false);

	dc.SetTextBackground(wxColour(255,255,255));

	dc.SetPen(wxPen(cursorcolor));
	if (inwindow & !pressedDown & mlx > 0 & mlx < hscale) 
		dc.DrawLine(MouseX,0,MouseX,h);
	if (mlx < 0) mlx = 0;
	if (mlx > hscale) mlx = hscale;
	//wxString str = wxString::Format("x: %d y: %d    hscale=%d",mlx,mly,hscale);
	wxString str, str1;
	if (inwindow)
		str = wxString::Format("x: %d    hscale: %d",mlx,hscale);
	else
		str = wxString::Format("hscale: %d",hscale);

	wxSize sz = dc.GetTextExtent(str);
	dc.DrawText(str,w-sz.GetWidth()-3,2);   //h-20);
	if (EVaxis) {
		str1 = wxString::Format("ev0: %0.2f",EV0);
		wxSize sz1 = dc.GetTextExtent(str1);
		dc.DrawText(str1,w-sz1.GetWidth()-3,lineheight+4);
		for (float ev=-EVrange; ev<=EVrange; ev+=EVinc) {
			wxString e; float fpart, ipart;
			fpart = modf(ev, &ipart);
			if (abs(fpart) > 0.0) 
				e = wxString::Format("%1.1f", abs(ev));
			else
				e = wxString::Format("%1.0f", abs(ev));
			int ew = wxSize(dc.GetTextExtent(e)).GetWidth();
			if (ev < 0.0) e.Prepend("-");
			if (ev > 0.0) e.Prepend("+");
			ew = wxSize(dc.GetTextExtent(e)).GetWidth() - ew;
			//dc.DrawText(e,((int) ((w*EV0*pow(2.0, ev)*wscale)-ew))+xorigin,(h-(lineheight-1))-yorigin);
			dc.DrawText(e,((int) ((w*EV0*pow(2.0, ev))-ew))+xorigin,(h-(lineheight-1))-yorigin);
		}
	}

	if (EVaxis) {
		dc.SetPen(wxPen(linecolor, 1, wxPENSTYLE_SOLID ));
		dc.DrawLine(w * EV0, 0, w * EV0, hmax);
		dc.SetPen(wxPen(linecolor, 1, wxPENSTYLE_LONG_DASH ));
		for (float ev=-EVrange; ev<=EVrange; ev+=EVinc) {
			dc.DrawLine(w * EV0*pow(2.0, ev), 0, w * EV0*pow(2.0, ev), hmax);

		}

	}


/*
	if (Unbounded) {
		dc.SetPen(wxPen(wxColour(192,192,0),1));
		dc.DrawLine(zerobucket, 0, zerobucket, hmax);
		dc.SetPen(wxPen(wxColour(0,192,192),1));
		dc.DrawLine(onebucket,  0, onebucket,  hmax);
	}
*/
}



void myHistogramPane::mouseWheelMoved(wxMouseEvent& event) 
{
	wxSize s = GetSize();
	xcenter = event.m_x; ycenter = event.m_y;
	double inc = 0.1;
	if (event.ShiftDown()) inc = 1.0;
	if (event.ControlDown()) inc = 10.0;
	if (event.GetWheelRotation() > 0) { 
		wscale += inc;
//		xorigin -= (inc*event.m_x); 
//		yorigin -= inc*(s.GetHeight()-event.m_y);
	}
	else {
		wscale -= inc;
//		xorigin += (inc*event.m_x); 
//		yorigin += inc*(s.GetHeight()-event.m_y);
	}
	if (wscale < 1.0) {
		wscale = 1.0;
		xorigin = 0;
		yorigin = 0;
	}


	Refresh();
	
	event.Skip();
}

void myHistogramPane::keyPressed(wxKeyEvent& event) 
{
	//wxMessageBox(wxString::Format("keycode: %d", event.GetKeyCode()));
	switch (event.GetKeyCode()) {
		case 66: //b - bounded/unbounded histogram
			if (Unbounded)
				Unbounded = false;
			else
				Unbounded = true;
			RecalcHistogram();
			break;
		case 116: //T
		case 84: //t
			wscale = 1.0;
			xorigin = 0;
			yorigin = 0;
			break;
		case WXK_SPACE : // s?
			if (ord == 1) ord = 2;
			else if(ord == 2) ord = 3;
			else if(ord == 3) ord = 1;
			break;
		case 82: //r - pan right
			if (event.ShiftDown()) xorigin -= 10;
			else if (event.ControlDown()) xorigin -= 100;
			else xorigin -= 1;
			break;
		case 88: //x - toggle x axis linear-EV
			if (EVaxis)
				EVaxis = false;
			else
				EVaxis = true;
			break;
		case 76: //l - pan left
			if (event.ShiftDown()) xorigin += 10;
			else if (event.ControlDown()) xorigin += 100;
			else xorigin += 1;
			break;
		case 67: // c - with Ctrl, copy 256-scale histogram to clipboard
			wxString hist;
			hist.Append(wxString::Format("red:%d", (int) histogram[0].r));
			for (int i = 1; i< histogram.size(); i++) 
				hist.Append(wxString::Format(",%d", (int) histogram[i].r));
			hist.Append("\n");
			hist.Append(wxString::Format("green:%d", (int) histogram[0].g));
			for (int i = 1; i< histogram.size(); i++) 
				hist.Append(wxString::Format(",%d", (int) histogram[i].g));
			hist.Append("\n");
			hist.Append(wxString::Format("blue:%d", (int) histogram[0].b));
			for (int i = 1; i< histogram.size(); i++) 
				hist.Append(wxString::Format(",%d", (int) histogram[i].b));
			hist.Append("\n");
			if (wxTheClipboard->Open())
			{
				wxTheClipboard->SetData( new wxTextDataObject(hist) );
				wxTheClipboard->Close();
			}
			break;
	}

	Refresh();
	
	event.Skip();
}
 
void myHistogramPane::mouseDown(wxMouseEvent& event) 
{
	pressedDown = true;
	MouseX = event.m_x;
	MouseY = event.m_y;
	Refresh();
	
	event.Skip();
}

void myHistogramPane::mouseMoved(wxMouseEvent& event)  
{
	unsigned x, y, dx, dy;
	x=event.m_x; y=event.m_y;
	if (pressedDown) {
		dx = MouseX-x;
		dy = MouseY-y;
		if (event.ShiftDown())   if (dx > dy) dx *= 10;  else dy *= 10;
		if (event.ControlDown()) if (dx > dy) dx *= 100; else dy *= 100;
		xorigin -= dx; yorigin += dy;
	}
	MouseX = x;
	MouseY = y;
	Refresh();
	
	event.Skip();
}

void myHistogramPane::mouseReleased(wxMouseEvent& event) 
{
	pressedDown = false;
	Refresh();
	
	event.Skip();
}

void myHistogramPane::mouseDoubleClicked(wxMouseEvent& event)
{
	wscale = 1.0;
	xorigin = 0;
	yorigin = 0;
	Refresh();
	
	event.Skip();
}


void myHistogramPane::mouseEnterWindow(wxMouseEvent& event) 
{
	inwindow = true;
	SetFocus();
	Refresh();
	
}

void myHistogramPane::mouseLeftWindow(wxMouseEvent& event) 
{
	inwindow =  false;
	Refresh();
	
}

//void myHistogramPane::rightClick(wxMouseEvent& event) {}
//void myHistogramPane::keyReleased(wxKeyEvent& event) {}
 
 

