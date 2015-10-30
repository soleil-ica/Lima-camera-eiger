// ---------------------------------------------------------------------------

#ifndef Main_Client_UDPH
#define Main_Client_UDPH
// ---------------------------------------------------------------------------
#include "Envoi.h"

#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <IdUDPBase.hpp>
#include <IdUDPClient.hpp>
#include <Buttons.hpp>
#include <ExtCtrls.hpp>
#include <IdSNTP.hpp>
#include <ComCtrls.hpp>
#include <IdUDPBase.hpp>
#include <IdUDPServer.hpp>
#include "IdCustomHTTPServer.hpp"
#include "IdCustomTCPServer.hpp"
#include "IdHTTPServer.hpp"

#include <map>
using namespace std;


/// For SendAPR DLL function call
#define C_FUNC_SEND_ARP                 "SendARP"
#define C_NAME_NETWORK_DLL              "IPHLPAPI.DLL"
typedef DWORD(__stdcall*DLLFUNC)(unsigned long, unsigned long, ULONG*, ULONG*);

// ---------------------------------------------------------------------------
class TForm_Main : public TForm
{
__published: // Composants gérés par l'EDI

   TLabel *L_Host;
   TEdit *E_Host;
   TIdHTTPServer *HTTPServer;
   TButton *Button1;
   TLabel *LabelFWstatus;
   TLabel *LabelDETstatus;
   TComboBox *ComboBoxFWstatus;
   TComboBox *ComboBoxDETstatus;
   TLabel *LabelCMDResponse;
   TEdit *EditCMDRespCode;
   TLabel *LabelCmdDuration_arm;
   TEdit *EditDuration_arm;
   TLabel *LabelCmdDuration_trigger;
   TEdit *EditDuration_trigger;

   void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
   void __fastcall HTTPServerCommandGet(TIdContext *AContext, TIdHTTPRequestInfo *ARequestInfo, TIdHTTPResponseInfo *AResponseInfo);
   void __fastcall FormShow(TObject *Sender);
   void __fastcall Button1Click(TObject *Sender);
   void __fastcall HTTPServerCommandOther(TIdContext *AContext, TIdHTTPRequestInfo *ARequestInfo, TIdHTTPResponseInfo *AResponseInfo);


private: // Déclarations de l'utilisateur
   bool m_bRun;
   AnsiString m_sMessage;
   int m_iNombre;
   int m_iDelais;
   HINSTANCE m_hDll;

   map <AnsiString, AnsiString> m_strResponse;

public: // Déclarations de l'utilisateur
   __fastcall TForm_Main(TComponent* Owner);
};

// ---------------------------------------------------------------------------
extern PACKAGE TForm_Main *Form_Main;
// ---------------------------------------------------------------------------
#endif
