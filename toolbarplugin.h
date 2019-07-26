/***************************************************************
 * Name:      toolbarplugin
 * Purpose:   Code::Blocks plugin for simulavr integration
 * Author:    Doug Sandy (dlsandy@asu.edu)
 * Created:   2019-07-16
 * Copyright: Doug Sandy, Arizona State University
 * License:   GPL
 **************************************************************/

#ifndef TOOLBARPLUGIN_H_INCLUDED
#define TOOLBARPLUGIN_H_INCLUDED

// For compilers that support precompilation, includes <wx/wx.h>
#include <wx/wxprec.h>
#include <wx/socket.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <cbplugin.h> // for "class cbPlugin"

#define SOCKET_ID 5555

class SimulavrToolbarPlugin : public cbPlugin
{
    public:
        /// Constructor / Destructor
        SimulavrToolbarPlugin();
        virtual ~SimulavrToolbarPlugin();

        virtual bool BuildToolBar(wxToolBar* toolBar);

        void OnSocketEvent(wxSocketEvent& event);
        void OnTempUpClicked(wxCommandEvent& event);
        void OnTempDnClicked(wxCommandEvent& event);

        bool SendByte(char ch);

    protected:
        /// helper functions
        wxBitmap * LoadBitmapFromResourceFile(wxString);

        /// event handleers
        virtual void OnAttach();
        virtual void OnRelease(bool appShutDown);
        void OnDebugStart(CodeBlocksEvent &event);

    private:
        /// member data
        wxToolBar *m_toolbar; /// pointer to the toolbar object
        wxBitmap *m_ledon;    /// pointer to the led "on" bitmap
        wxBitmap *m_ledoff;   /// pointer to the led "off" bitmap
        wxBitmap *m_ledtri;   /// pointer to the led "tristate" bitmap
        wxBitmap *m_temp_up;  /// pointer to the temperature increase button's bitmap
        wxBitmap *m_temp_dn;  /// pointer to the temperature decrease button's bitmap
        wxBitmap m_dummyBmp;  /// a dummy (empty) bitmap to be used if other resources cant be loaded
        wxDatagramSocket * m_sock;   /// pointer to the UDP communications socket
        wxStaticBitmap *m_led;       /// led control on the toolbar
        DECLARE_EVENT_TABLE();
};

#endif // TOOLBARPLUGIN_H_INCLUDED
