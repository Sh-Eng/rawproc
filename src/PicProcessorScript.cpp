#include "PicProcessorScript.h"
#include "PicProcPanel.h"
#include "myConfig.h"
#include "myRowSizer.h"
#include "util.h"
#include "gimage_cmd.h"
#include "gimage/strutil.h"
#include "gimage_parse.h"
#include "gimage_process.h"
#include <wx/textfile.h>
#include <wx/datetime.h>

#include <fstream>
#include <sstream>
#include <string>

#define SCRIPTENABLE 8700
#define SCRIPTFILESELECT 8701
#define SCRIPTFILESAVE 8702
#define SCRIPTUPDATE 8703
#define SCRIPTAUTOUPDATE 8607

class ScriptPanel: public PicProcPanel
{

	public:
		ScriptPanel(wxWindow *parent, PicProcessor *proc, wxString params): PicProcPanel(parent, proc, params)
		{
			Freeze();
			wxSizerFlags flags = wxSizerFlags().Left().Border(wxLEFT|wxRIGHT|wxTOP);

			enablebox = new wxCheckBox(this, SCRIPTENABLE, _("gmic:"));
			enablebox->SetValue(true);

			//edit = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(200,200), wxTE_MULTILINE);
			//setEdit(params);
			//wxString editstring = params;
			//editstring.Replace(";","\n");
			//edit->SetValue(editstring);
			
			file = new wxStaticText(this, wxID_ANY, "(no file)");
			autobox = new wxCheckBox(this, SCRIPTAUTOUPDATE, _("auto update"));
			
			myRowSizer *m = new myRowSizer(wxSizerFlags().Expand());
			m->AddRowItem(enablebox, wxSizerFlags(1).Left().Border(wxLEFT|wxTOP));

			m->NextRow(wxSizerFlags().Expand());
			m->AddRowItem(new wxStaticLine(this, wxID_ANY), wxSizerFlags(1).Left().Border(wxLEFT|wxRIGHT|wxTOP|wxBOTTOM));

			m->NextRow();
			m->AddRowItem(new wxButton(this, SCRIPTFILESELECT, _("Select File...")), flags);
			m->AddRowItem(file, flags);
			//m->AddRowItem(new wxButton(this, SCRIPTFILESAVE, _("Save File...")), flags);
			m->NextRow();
			//m->AddRowItem(edit, flags);
			
			//m->NextRow();
			m->AddRowItem(new wxButton(this, SCRIPTUPDATE, _("Run Script")), flags);
			m->AddRowItem(autobox, flags);
			m->End();

			SetSizerAndFit(m);

			if (!params.IsEmpty()) {
				scriptfile = wxFileName(params);
				file->SetLabel(scriptfile.GetFullName());
			}

			t.SetOwner(this);

			Bind(wxEVT_CHECKBOX, &ScriptPanel::onEnable, this, SCRIPTENABLE);
			Bind(wxEVT_BUTTON, &ScriptPanel::selectFile, this, SCRIPTFILESELECT);
			//Bind(wxEVT_BUTTON, &ScriptPanel::saveFile, this, SCRIPTFILESAVE);
			Bind(wxEVT_BUTTON, &ScriptPanel::updateScript, this, SCRIPTUPDATE);
			Bind(wxEVT_CHAR_HOOK, &ScriptPanel::OnKey,  this);
			Bind(wxEVT_TIMER, &ScriptPanel::OnTimer,  this);
			Bind(wxEVT_CHECKBOX, &ScriptPanel::onAuto, this, SCRIPTAUTOUPDATE);
			Thaw();
		}
		

		void onEnable(wxCommandEvent& event)
		{
			if (enablebox->GetValue()) {
				q->enableProcessing(true);
				q->processPic();
			}
			else {
				q->enableProcessing(false);
				q->processPic();
			}
		}

		void updateScript(wxCommandEvent& event)
		{
			q->processPic();
		}

		void setEdit(wxString commandstring)
		{
			//commandstring.Replace(";","\n");
			//edit->SetValue(commandstring);
		}

		void selectFile(wxCommandEvent& event)
		{
			selectFile();
		}

		void selectFile()
		{
			wxString commandstring;
			//wxFileName toollistpath;
			//toollistpath.AssignDir(wxString(myConfig::getConfig().getValueOrDefault("app.toollistpath","")));

			wxString fname = wxFileSelector(_("Open GMIC Script..."), scriptfile.GetPath());  //, toollistpath.GetPath());
			if (fname == "") return;
			wxFileName filepath(fname);

			if (filepath.FileExists()) {
				filepath.MakeAbsolute();
				scriptfile = filepath;
				modtime = filepath.GetModificationTime();
				file->SetLabel(filepath.GetFullName());
				((PicProcessorScript *) q)->setSource(filepath.GetFullPath());
				if (filepath.GetPath() == wxFileName::GetCwd())
					q->setParams(filepath.GetFullName());
				else
					q->setParams(filepath.GetFullPath());
				q->processPic();
				
			}
			else wxMessageBox(_("Error: script file not found."));
		}
		
		void onAuto(wxCommandEvent& event)
		{
			if (autobox->IsChecked()) {
				t.Stop();
				modtime = scriptfile.GetModificationTime();
				t.Start(500,wxTIMER_ONE_SHOT);
			}
			else t.Stop();
		}
		
		void OnTimer(wxTimerEvent& event)
		{
			wxDateTime m = scriptfile.GetModificationTime();
			if (!m.IsEqualTo(modtime)) {
				q->processPic();
				modtime = m;
			}
			if (autobox->IsChecked()) t.Start(500,wxTIMER_ONE_SHOT);
			event.Skip();
		}


	private:
		wxCheckBox *enablebox, *autobox;
		wxTextCtrl *edit;
		wxStaticText *file;
		wxFileName scriptfile;
		wxDateTime modtime;
		wxTimer t;

};


PicProcessorScript::PicProcessorScript(wxString name, wxString command, wxTreeCtrl *tree, wxString imagefile, PicPanel *display): PicProcessor(name, command, tree, display)
{
	//showParams();
	//loadCommands(command);
	img = imagefile;
}

void PicProcessorScript::createPanel(wxSimplebook* parent)
{
	toolpanel = new ScriptPanel(parent, this, c);
	parent->ShowNewPage(toolpanel);
	toolpanel->Refresh();
	toolpanel->Update();
}

//void PicProcessorScript::selectFile()
//{
//	((GroupPanel *) toolpanel)->selectFile();
//}

void PicProcessorScript::setSource(wxString src)
{
	source = src;
	//m_tree->SetItemText(id, n+":"+source);
}

wxString PicProcessorScript::getSource()
{
	return source;
}

bool PicProcessorScript::processPicture(gImage *processdib) 
{
	if (!processingenabled) return true;
	
	((wxFrame*) m_display->GetParent())->SetStatusText(_("script..."));
	bool ret = true;
	std::map<std::string,std::string> result;

	std::map<std::string,std::string> params;
	std::string pstr = getParams().ToStdString();

	if (!pstr.empty())
		//params = parse_gmic(std::string(pstr));
	
	if (params.find("error") != params.end()) {
		wxMessageBox(params["error"]);
		ret = false; 
	}
	//else if (params.find("mode") == params.end()) {  //all variants need a mode, now...
	//	wxMessageBox("Error - no mode");
	//	ret = false;
	//}
	else { 
		//create temp file from dib
		wxFileName inimage, outimage;
		inimage = outimage = img;
		inimage.SetName(inimage.GetName()+"-in");
		inimage.SetExt("tif");
		outimage.SetName(outimage.GetName()+"-out");
		outimage.SetExt("tif");
		//printf("Saving temp file: %s...\n", imgname.GetFullPath().ToStdString().c_str()); fflush(stdout);
		dib->saveTIFF(inimage.GetFullName().ToStdString().c_str(), BPP_UFP, "");
		
		//create command string
		//parm script.gmic.command.command: Full path/filename to the gmic.exe program.  Default=(none), won't work without a valid program.
		wxString scriptcommand = wxString(myConfig::getConfig().getValueOrDefault("script.gmic.command",""));
		if (scriptcommand == "") {
			wxMessageBox("No gmic path defined in script.gmic.command");
			return false;
		}
		
		std::string fn = source.ToStdString();
		std::ifstream ifs(fn);
		std::string scr( (std::istreambuf_iterator<char>(ifs) ),(std::istreambuf_iterator<char>()    ) );
		wxString script = wxString(scr);
		
		wxString cmd = wxString::Format("%s %s %s output %s,float", scriptcommand, inimage.GetFullName(), script, outimage.GetFullName() );
		//wxMessageBox(cmd);

		//wxExecute command string, wait to finish
		wxArrayString output, errors;
		wxExecute (cmd, output, errors, wxEXEC_NODISABLE);
		//wxMessageBox(wxString::Format("\nOutput:\n%s\n\nErrors:%s\n",output, errors));
		
		gImage newdib(gImage::loadTIFF(outimage.GetFullName().ToStdString().c_str(), ""));
		dib->setImage(newdib.getImageData(), newdib.getWidth(), newdib.getHeight()); //retain the metadata and profile
		
		//result = process_gmic(*dib, params)
		if (paramexists(result,"treelabel")) m_tree->SetItemText(id, wxString(result["treelabel"]));
		
		//if (result.find("error") != result.end()) {
		//	wxMessageBox(wxString(result["error"]));
		//	ret = false;
		//}
		//else {
			m_display->SetModified(true);
			if ((myConfig::getConfig().getValueOrDefault("tool.all.log","0") == "1") || 
				(myConfig::getConfig().getValueOrDefault("tool.script.log","0") == "1"))
					log(wxString::Format(_("tool=script,%s,imagesize=%dx%d,time=%s"),
						params["mode"].c_str(),
						dib->getWidth(), 
						dib->getHeight(),
						result["duration"].c_str())
					);
		//}
	}

	dirty=false;
	((wxFrame*) m_display->GetParent())->SetStatusText("");
	return ret;
}





