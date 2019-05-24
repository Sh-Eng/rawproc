
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
	TextVisible = true;
	FloatCursor = true;
	db = NULL;
	
	r = NULL; g = NULL; b = NULL;
	rlen=0; glen=0; blen=0;
	display_channels = CHANNEL_RGB;
		
	MouseX = 0; MouseY=0;
	pressedDown = false;
	inwindow = false;

	if (myConfig::getConfig().getValueOrDefault("app.tooltip","1") == "1")
		SetToolTip("Keyboard Commands:\n   d: histogram display/data\n   e: EV markers\n   f: cursor data/bin\n   l: label visibility   r: reset scale\n   t: toggle tooltip\n   <sp>: channel on-top\n   ctrl-c: copy 256-bin histogram to clipboard\n   left-right arrows: pan histogram");

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

bool myHistogramPane::ToggleToolTip()
{
	if (GetToolTipText() == "") {
		SetToolTip("Histogram Keyboard Commands:\n   d: histogram display/data\n   e: EV markers\n   f: cursor data/bin\n   l: label visibility   r: reset scale\n   t: toggle tooltip\n   <sp>: channel on-top\n   ctrl-c: copy 256-bin histogram to clipboard\n   left-right arrows: pan histogram");
		return true;
	}
	else {
		UnsetToolTip();
		return false;
	}
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

	if (db == NULL) return;

	histogram.clear();
	if (Unbounded) {
		histogram = db->Histogram(hscale, zerobucket, onebucket, dmin, dmax);
	}
	else {
		histogram = db->Histogram(hscale);
		dmin = 0.0;
		dmax = 1.0;
	}
	
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
	

	//parm histogram.clipbins: number of buckets to eliminate on both ends of the histogram in calculating the max height.  Default=0
	unsigned clipbins = atoi(myConfig::getConfig().getValueOrDefault("histogram.clipbins","0").c_str()); 
	unsigned lower = clipbins;
	unsigned upper = (hscale-1)-clipbins;
	
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

	//Get a line height from an arbitrary character:
	int lineheight = wxSize(dc.GetTextExtent("l")).GetHeight();


//go to histogram coordinates:
	if (EVaxis && !Unbounded) {  //EVAxis currently incorrect for Unbounded...
		dc.SetLogicalScale(((double) (w-6) / (double) hscale)* wscale, ((double) (h-lineheight)/ (double) hmax));// * wscale);
		dc.SetDeviceOrigin (xorigin+3, h-yorigin-lineheight);
	}
	else {
		dc.SetLogicalScale(((double) (w-6) / (double) hscale)* wscale, ((double) (h-5)/ (double) hmax)); // * wscale);
		dc.SetDeviceOrigin (xorigin+3, h-yorigin-5);
	}
	dc.SetAxisOrientation(true,true);

	//if (something I haven't defined yet) {
		wxColour boundcolor = wxString2wxColour(wxString(myConfig::getConfig().getValueOrDefault("histogram.bound.color","128")));
		wxPen origpen = dc.GetPen();
		wxBrush origbrush = dc.GetBrush();
		dc.SetPen(wxPen(boundcolor));
		dc.SetBrush(wxBrush(boundcolor));

		if (Unbounded) 
			dc.DrawRectangle(zerobucket,0,onebucket-zerobucket,hmax);
		else
			dc.DrawRectangle(0,0,hscale,hmax);

		dc.SetPen(origpen);
		dc.SetBrush(origbrush);
	//}

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
	//if (EV0 == 0.0) EV0 = 0.18;
	//parm histogram.ev.range: +/- range to plot EV.  Default: 3.0 (-/+ 3 stops)
	float EVrange = atof(myConfig::getConfig().getValueOrDefault("histogram.ev.range","3.0").c_str());
	//if (EVrange == 0.0) EVrange = 3.0;
	//parm histogram.ev.increment: Step increment for EV plot.  Default: 1.0
	float EVinc = atof(myConfig::getConfig().getValueOrDefault("histogram.ev.increment","1.0").c_str());
	//if (EVinc == 0.0) EVinc = 1.0;

	std::map<float,int> evlist;
	if (EVaxis && !Unbounded) {  //EVAxis currently incorrect for Unbounded...
		dc.SetPen(wxPen(linecolor, 1, wxPENSTYLE_SOLID ));
		dc.DrawLine(hscale * EV0, 0, hscale * EV0, hmax);
		dc.SetPen(wxPen(linecolor, 1, wxPENSTYLE_LONG_DASH ));
		float prev_evval = EV0;
		for (float ev=-EVrange; ev<=EVrange; ev+=EVinc) {
			float evval = hscale*EV0*pow(2.0, ev);
			evlist[ev] = dc.LogicalToDeviceX(evval);
			dc.DrawLine(evval, 0, evval, hmax);
		}

	}

	//marker lines:
	dc.SetPen(wxPen(cursorcolor));
	int mlx = dc.DeviceToLogicalX(wxCoord(MouseX));
	float mlx_ev = hscale * mlx*pow(2.9, 1.0/mlx);

//return to window coords:
	dc.SetLogicalScale(1.0, 1.0);
	dc.SetDeviceOrigin (0, 0);
	dc.SetAxisOrientation(true,false);

	//if (mlx < 0) mlx = 0;
	//if (mlx >= hscale) mlx = hscale-1;

	float inc = (dmax - dmin) / (float) hscale;
	float mlf = dmin + (inc * (float) mlx);

	dc.SetTextBackground(wxColour(255,255,255));

	dc.SetPen(wxPen(cursorcolor));
	if (inwindow & !pressedDown) { // & mlx > 0 & mlx < hscale) {
		dc.DrawLine(MouseX,0,MouseX,h);
		wxString cursorstr;
		if (FloatCursor)
			cursorstr = wxString::Format("%0.4f",mlf);
		else
			cursorstr = wxString::Format("%d",mlx);
		if (MouseX < w/2)
			dc.DrawText(cursorstr,MouseX+1, 2);
		else 
			dc.DrawText(cursorstr,MouseX-(dc.GetTextExtent(cursorstr).GetWidth()+1), 2);
		//dc.DrawText(wxString::Format("%d",mlx),MouseX, 2); //ToDo: property to switch between data/bin...
	}

	long mlr, mlg, mlb;
	if (!histogram.empty()  && histogram.size() > mlx) {
		mlr = histogram[mlx].r;
		mlg = histogram[mlx].g;
		mlb = histogram[mlx].b;
	}
	//wxString str = wxString::Format("x: %d   hscale=%d",mlx,hscale);
	wxString str, str1;

	if (TextVisible) {
		wxString bstr = wxString::Format("bins:%d",hscale);
		wxString rstr = (Unbounded) ? "\nextent:data" : "\nextent:display"; 
		int strw = std::min(w-dc.GetTextExtent(bstr).GetWidth(),w-dc.GetTextExtent(rstr).GetWidth());
		bstr.Append(rstr);
		dc.DrawText(bstr,strw, (lineheight*1)+2);
	}

	//dc.DrawText(bstr,w-dc.GetTextExtent(bstr).GetWidth()-2, (lineheight*1)+2);
	//dc.DrawText(rstr,w-dc.GetTextExtent(rstr).GetWidth()-2, (lineheight*2)+2);

	//dc.DrawText(bstr,strw, (lineheight*1)+2);
	//dc.DrawText(rstr,strw, (lineheight*2)+2);


	if (EVaxis && !Unbounded) {  //EVAxis currently incorrect for Unbounded...
		for (std::map<float,int>::iterator it=evlist.begin(); it!=evlist.end(); ++it) {
			wxString evstr;
			if (it->first == 0.0)
				evstr = wxString::Format("%0.f", it->first);
			else
				evstr = wxString::Format("%+0.1f", it->first);
			dc.DrawText(evstr,it->second-dc.GetTextExtent(evstr).GetWidth()/2, h-lineheight);
		}
	}

//	wxString ws = wxString::Format("scale: %0.2f",wscale);
//	int ww = wxSize(dc.GetTextExtent(ws)).GetWidth();
//	dc.DrawText(ws, w-ww, h-lineheight-1);

	if (inwindow) 
		if (mlx >=0 & mlx <hscale) ((wxFrame *) GetParent())->SetStatusText(wxString::Format("bin:%d rgb:%d,%d,%d",mlx,mlr,mlg,mlb));

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
		//xorigin -= (inc*event.m_x); 
		xorigin -= abs(xorigin) * inc;
//		yorigin -= inc*(s.GetHeight()-event.m_y);
	}
	else {
		wscale -= inc;
		//xorigin += (inc*event.m_x); 
		xorigin += abs(xorigin) * inc;
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
		case 68: //d - bounded/unbounded histogram
			if (Unbounded)
				Unbounded = false;
			else
				Unbounded = true;
			RecalcHistogram();
			Refresh();
			break;
		case 116: //T
		case 82: //r - reset
			wscale = 1.0;
			xorigin = 0;
			yorigin = 0;
			Refresh();
			break;
		case WXK_SPACE : // s?
			if (ord == 1) ord = 2;
			else if(ord == 2) ord = 3;
			else if(ord == 3) ord = 1;
			Refresh();
			break;
		case 316: //-> - pan right
			if (event.ShiftDown()) xorigin -= 10;
			else if (event.ControlDown()) xorigin -= 100;
			else xorigin -= 1;
			Refresh();
			break;
		case 84: //t - toggle tooltip
				if (ToggleToolTip())
					((wxFrame *) GetParent())->SetStatusText("Histogram tooltip display: on");
				else
					((wxFrame *) GetParent())->SetStatusText("Histogram tooltip display: off");
			break;

		case 76: // l - toggle lables
			if (TextVisible)
				TextVisible = false;
			else
				TextVisible = true;
			Refresh();
			break;
		case 69: //e - toggle EV stop lines
			if (EVaxis)
				EVaxis = false;
			else
				EVaxis = true;
			Refresh();
			break;
		case 70: //f - toggle float/bin cursor label
			if (FloatCursor)
				FloatCursor = false;
			else
				FloatCursor = true;
			Refresh();
			break;
		case 314: // <- - pan left
			if (event.ShiftDown()) xorigin += 10;
			else if (event.ControlDown()) xorigin += 100;
			else xorigin += 1;
			Refresh();
			break;
		case 67: // c - with Ctrl, copy 256-scale histogram to clipboard
			if (!event.ControlDown()) break;
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
			((wxFrame *) GetParent())->SetStatusText("histogram data copied to clipboard");
			break;
	}

	
	event.Skip();
}
 
void myHistogramPane::mouseDown(wxMouseEvent& event) 
{
	SetFocus();
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
		dy = 0; //dy = MouseY-y;  //panning only left-right...
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
	Refresh();
	
}

void myHistogramPane::mouseLeftWindow(wxMouseEvent& event) 
{
	inwindow =  false;
	((wxFrame *) GetParent())->SetStatusText("");
	Refresh();
	
}

//void myHistogramPane::rightClick(wxMouseEvent& event) {}
//void myHistogramPane::keyReleased(wxKeyEvent& event) {}
 
 

