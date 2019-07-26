/***************************************************************
 * Name:      toolbarplugin
 * Purpose:   Code::Blocks plugin for simulavr integration
 * Author:    Doug Sandy (dlsandy@asu.edu)
 * Created:   2019-07-16
 * Copyright: Doug Sandy, Arizona State University
 * License:   GPL
 **************************************************************/

#include <sdk.h> // Code::Blocks SDK

#include <logmanager.h>
#include <configmanager.h>
#include <string>
#include "toolbarplugin.h"
#include <wx/fs_zip.h>

#define TEMP_UP_ID   5000
#define TEMP_DOWN_ID 5001

// Register the plugin with Code::Blocks.
// We are using an anonymous namespace so we don't litter the global one.
namespace
{
    PluginRegistrant<SimulavrToolbarPlugin> reg(_T("simulavr-plugin"));
}


// events handling
BEGIN_EVENT_TABLE(SimulavrToolbarPlugin, cbPlugin)
    EVT_SOCKET(SOCKET_ID, SimulavrToolbarPlugin::OnSocketEvent)
END_EVENT_TABLE()

/********************************************************************
* constructor
*/
SimulavrToolbarPlugin::SimulavrToolbarPlugin() :m_led(NULL)
{
    if(!Manager::LoadResource(_T("simulavr.zip")))
    {
        NotifyMissingFile(_T("simulavr.zip"));
    }

}

/********************************************************************
* destructor
*/
SimulavrToolbarPlugin::~SimulavrToolbarPlugin()
{
}

/********************************************************************
* LoadBitmapFromResourceFile
* Load a bitmap image from the plugin's resource file.
*
* Params:
*    imagename - the filename of the image to load.  This assumes that
*       the image is within the images folder within the resoure archive.
* Returns:
*    a pointer to the image on success, otherwise a pointer to a
*    default blank image.
*/
wxBitmap * SimulavrToolbarPlugin::LoadBitmapFromResourceFile(wxString imagename)
{
    wxFileSystem filesystem;
    wxImage image;

    // add handlers if they have not been added already
    wxImage::AddHandler(new wxPNGHandler);
    wxFileSystem::AddHandler(new wxArchiveFSHandler);

    // get the location of the resource file
    wxString file = "simulavr.zip";
    wxString resourceFile = ConfigManager::LocateDataFile(file, sdDataGlobal | sdDataUser);

    // make sure that we can read the resource file
    if (wxFile::Access(resourceFile, wxFile::read) == false)
    {
        Manager::Get()->GetLogManager()->LogError(_("Unable to read resource file '") + resourceFile + _("'."));
        return &m_dummyBmp;
    }

    // open the image file from within the archive
    wxFSFile *fsfile = filesystem.OpenFile(_T("file:")+resourceFile+_T("#zip:images/")+imagename);
    if (fsfile==nullptr) {
        Manager::Get()->GetLogManager()->LogError("Could not find resource in archive : "+imagename);
        return &m_dummyBmp;
    }

    // load the image
    if (!image.LoadFile(*fsfile->GetStream(), wxBITMAP_TYPE_PNG)) {
        Manager::Get()->GetLogManager()->LogError("Unable to load resource:" +imagename);
        return &m_dummyBmp;
    }
    return new wxBitmap(image);
}

/********************************************************************
* OnDebugStart
* when the Code::blocks debugger starts, set the led to tristate to
* match the state of the simulated device.
*/
void SimulavrToolbarPlugin::OnDebugStart(CodeBlocksEvent &event)
{
    m_led->SetBitmap(*m_ledtri);
    m_toolbar->Realize();
}

/********************************************************************
* OnSocketEvent
* event handler for receiving data from simulavr (UDP port 8877).
*
*/
void SimulavrToolbarPlugin::OnSocketEvent(wxSocketEvent& event)
{
	wxIPV4address addr;
	addr.Service(8877);
	char buf[1024];
	size_t n;

	switch(event.GetSocketEvent())
	{
	case wxSOCKET_INPUT:
		// stop further notifications while processing this event
		m_sock->Notify(false);
		// receive the data into the buffer.
		n = m_sock->RecvFrom(addr, buf, sizeof(buf)).LastCount();
		if (!n)	{
			// failed to receive data - just return.
            m_sock->Notify(true);
			return;
		}

		// update something in the toolbar
		if (m_led) {
            switch (buf[0]) {
            case 'H':
            case 'h':
                m_led->SetBitmap(*m_ledon);
                m_toolbar->Realize();
                break;
            case 'L':
            case 'l':
                m_led->SetBitmap(*m_ledoff);
                m_toolbar->Realize();
                break;
            case 'Z':
                m_led->SetBitmap(*m_ledtri);
                m_toolbar->Realize();
                break;
            }
		}

		m_sock->Notify(true);
		break;
	}
}

/********************************************************************
* SendByte
* send a single byte of information to simulavr on UDP port 7777
*
*/
bool SimulavrToolbarPlugin::SendByte(char ch)
{
	wxIPV4address addrLocal;
	addrLocal.AnyAddress();
    wxDatagramSocket sock2tx(addrLocal);
    if ( !sock2tx.IsOk() )
    {
        return false;
    }

	wxIPV4address raddr;
	raddr.Hostname("localhost");
    raddr.Service(7777);
	if (sock2tx.SendTo(raddr, &ch, 1).LastCount() != 1)
	{
		return false;
	}
}

/********************************************************************
* OnAttach
*
* initialize the plugin.
* NOTE: after this function, the inherited member variable m_IsAttached
* will be TRUE.
*/
void SimulavrToolbarPlugin::OnAttach()
{
    // register an event handler for the start of the Code::blocks debugger
    Manager::Get()->RegisterEventSink(cbEVT_DEBUGGER_STARTED, new cbEventFunctor<SimulavrToolbarPlugin, CodeBlocksEvent>(this, &SimulavrToolbarPlugin::OnDebugStart));
}


/*****************************************************************************
* OnRelease
* Perform any necessary de-initialization. This method is called by
* Code::Blocks PluginManager when the plugin needs to be de-attached from
* Code::Blocks.
*
* params:
* appShutDown (Input) If true, the application is shutting down.
*/
void SimulavrToolbarPlugin::OnRelease(bool appShutDown)
{
    if ((m_ledon  != &m_dummyBmp)&&(m_ledon  !=nullptr)) delete m_ledon;
    if ((m_ledoff != &m_dummyBmp)&&(m_ledoff !=nullptr)) delete m_ledoff;
    if ((m_ledtri != &m_dummyBmp)&&(m_ledtri !=nullptr)) delete m_ledtri;
    if ((m_temp_up!= &m_dummyBmp)&&(m_temp_up!=nullptr)) delete m_temp_up;
    if ((m_temp_dn!= &m_dummyBmp)&&(m_temp_dn!=nullptr)) delete m_temp_dn;
    if (m_sock!=nullptr) delete m_sock;
}

/********************************************************************
* OnTempUpClicked
* event handler for temperature increase button.  Send the event onward
* to Simulavr through the socket interface.
*/
void SimulavrToolbarPlugin::OnTempUpClicked(wxCommandEvent& WXUNUSED(event))
{
    SendByte('>');
}

/********************************************************************
* OnTempDnClicked
* event handler for temperature decrease button.  Send the event onward
* to Simulavr through the socket interface.
*/
void SimulavrToolbarPlugin::OnTempDnClicked(wxCommandEvent& WXUNUSED(event))
{
    SendByte('<');
}

/********************************************************************
* BuildToolBar()
*
* Code::blocks has created a toolbar and offered it for use by the
* plugin.  This method adds toolbar controls and tools to the
* empty toolbar.
*
* Params:
* toolbar (input) a pointer to the wxWidgets toolbar associated with this
*         plugin.
*/
bool SimulavrToolbarPlugin::BuildToolBar(wxToolBar* toolBar)
{
    // save the pointer to the toolbar in case it is needed later
    m_toolbar = toolBar;

    // load the toolbar images
    m_ledon  = LoadBitmapFromResourceFile("ledon.png");
    m_ledoff = LoadBitmapFromResourceFile("ledoff.png");
    m_ledtri = LoadBitmapFromResourceFile("ledtri.png");
    m_temp_up = LoadBitmapFromResourceFile("temp_up.png");
    m_temp_dn = LoadBitmapFromResourceFile("temp_dn.png");

    // add toolbar controls and tools
    m_led = new wxStaticBitmap(m_toolbar,m_toolbar->GetId(),*m_ledtri);
    toolBar->AddControl(new wxStaticText(m_toolbar,m_toolbar->GetId()," LED State: "));
    toolBar->AddControl(m_led);
    toolBar->AddControl(new wxStaticText(m_toolbar,m_toolbar->GetId(),"       Temperature: "));
    toolBar->AddTool(TEMP_UP_ID,"Raise Device Temperature", *m_temp_up);
    toolBar->AddTool(TEMP_DOWN_ID,"Lower Device Temperature", *m_temp_dn);

    // Create the communications socket
    wxIPV4address addr;
    addr.AnyAddress();
    addr.Service(8877);
    m_sock = new wxDatagramSocket(addr);

    // check to make sure the socket is okay before proceeding
    if (m_sock==NULL) return true;
    if (!m_sock->IsOk()){
        delete m_sock;
        m_sock = NULL;
        return true;
    }

    wxIPV4address addrReal;
    if (!m_sock->GetLocal(addrReal)){
        delete m_sock;
        m_sock = NULL;
        return true;
    }

    // Setup the event handler
    m_sock->SetEventHandler( *this, SOCKET_ID);
    m_sock->SetNotify(wxSOCKET_INPUT_FLAG);
    m_sock->Notify(true);

    // connect the event handlers for the toolbar buttons
    Connect(TEMP_UP_ID, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(SimulavrToolbarPlugin::OnTempUpClicked));
    Connect(TEMP_DOWN_ID, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(SimulavrToolbarPlugin::OnTempDnClicked));

    // return true if you add toolbar items
    return true;
}
