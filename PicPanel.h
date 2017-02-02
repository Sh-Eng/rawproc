#ifndef __PICPANEL_H__
#define __PICPANEL_H__

#ifndef WX_PRECOMP
	#include <wx/wx.h>
	#include <wx/frame.h>
#else
	#include <wx/wxprec.h>
#endif

#include <wx/image.h>
#include <wx/dcbuffer.h>
#include <wx/string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <lcms2.h>
//#include "FreeImage.h"
#include <gimage.h>
//#include "PicProcessorCrop.h"


class PicPanel: public wxPanel
{
    public:
        PicPanel(wxFrame *parent);
	~PicPanel();
        void OnEraseBackground(wxEraseEvent& event);
        void OnSize(wxSizeEvent& event);
	void PaintNow();
        void OnPaint(wxPaintEvent& event);
	void render(wxDC &dc);
        void drawBox(wxDC &dc, int x, int y, int w,int h);
	void SetPic(gImage * dib);
	void SetColorManagement(bool b);
	bool GetColorManagement();
	void SetImageProfile(cmsHPROFILE hImgProf);
	void SetThumbMode(int mode);
	void ToggleThumb();
	//void ToggleCropMode();
	//wxString GetCropParams();
	//void SetCropParams(wxString params);
	//void SetCropRatio(int r);
	wxString getHistogramString();
	double GetScale();
	void SetScale(double s);
	void SetScaleToWidth();
	void SetScaleToWidth(double percentofwidth);
	void FitMode(bool f);
	//void CropMode(bool c);
        void OnMouseWheel(wxMouseEvent& event);
        void OnMouseMove(wxMouseEvent& event);
        void OnLeftUp(wxMouseEvent& event);
        void OnRightDown(wxMouseEvent& event);
        void OnLeftDown(wxMouseEvent& event);
	void OnLeftDoubleClicked(wxMouseEvent& event);
	void OnKey(wxKeyEvent& event);
        
    private:

	gImage * d;
        wxFrame *parentframe;
        wxImage img;
        wxImage *thumbimg, *scaledimg;
        wxBitmap *pic, *thumb, *scaledpic, *histogram;
	wxBitmap hsgram;
    
        int MouseX, MouseY;
        int picX, picY;
        int thumbW, thumbH;
        float scale, aspectW, aspectH;
        
        bool moving, thumbmoving;
        
        bool showDebug;
        bool scaleWindow;
	bool fitmode;

	bool colormgt;
	cmsHPROFILE hImgProfile;
	cmsHTRANSFORM hTransform;

        wxColor histogramcolor;
        int toggleThumb;
        int keyCode;

	unsigned int hgram[256];
	wxString histstr;

        DECLARE_EVENT_TABLE()
    
};



#endif
