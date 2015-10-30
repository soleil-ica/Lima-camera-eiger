// ---------------------------------------------------------------------------

#pragma hdrstop

#include "Main_Client_UDP.h"
#include "HTTPView.h"
#include <vcl.h>
#include <memory>

// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "IdCustomHTTPServer"
#pragma link "IdCustomTCPServer"
#pragma link "IdHTTPServer"
#pragma resource "*.dfm"
TForm_Main *Form_Main;

// ---------------------------------------------------------------------------
__fastcall TForm_Main::TForm_Main(TComponent* Owner) : TForm(Owner)
{
   m_hDll = LoadLibrary(C_NAME_NETWORK_DLL);


   E_Host->Clear();

   m_bRun = true;

   char pcMachineName[256];
   gethostname(pcMachineName, 255);
   hostent* host = gethostbyname(pcMachineName);
   AnsiString strAddr;
   if (NULL!= host->h_addr_list)
   {
    //Obtain the computer's IP
    unsigned char b1 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b1;
    unsigned char b2 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b2;
    unsigned char b3 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b3;
    unsigned char b4 = ((struct in_addr *)(host->h_addr))->S_un.S_un_b.s_b4;

    AnsiString strAddr;
    strAddr.printf("%d.%d.%d.%d", b1, b2, b3, b4);
    E_Host->Text = strAddr;
   }

   m_strResponse["/detector/api/version/"] = " { \"value_type\": \"string\", \"value\": \"SIMAPI1.0\"}";
   m_strResponse["detector_readout_time"]  = " { \"value_type\": \"float\",  \"value\": 0.001 }";
   m_strResponse["count_time"]             = " { \"value_type\": \"float\",  \"value\": 0.5 }";
   m_strResponse["compression_enabled"]    = " { \"value_type\": \"uint\",   \"value\": 1 }";
   m_strResponse["x_pixel_size"]           = " { \"value_type\": \"float\",  \"value\": 0.000075 }";
   m_strResponse["y_pixel_size"]           = " { \"value_type\": \"float\",  \"value\": 0.000075 }";
   m_strResponse["x_pixels_in_detector"]   = " { \"value_type\": \"uint\",   \"value\": 1030 }";
   m_strResponse["y_pixels_in_detector"]   = " { \"value_type\": \"uint\",   \"value\": 1065 }";
   m_strResponse["bit_depth_readout"]      = " { \"value_type\": \"uint\",   \"value\": 12 }";
   m_strResponse["description "]           = " { \"value_type\": \"string\", \"value\": \"Eiger1M Simulator\" }";
   m_strResponse["detector_number"]        = " { \"value_type\": \"string\", \"value\": \"00001\" }";

   AnsiString strBooltrue  = " { \"value_type\": \"uint\",   \"value\": 1 }";
   AnsiString strBoolfalse = " { \"value_type\": \"uint\",   \"value\": 0 }";
   m_strResponse["countrate_correction_applied"]     = strBoolfalse;
   m_strResponse["flatfield_correction_applied"]     = strBooltrue;
   m_strResponse["pixel_mask_applied"]               = strBoolfalse;
   m_strResponse["virtual_pixel_correction_applied"] = strBooltrue;
   m_strResponse["efficiency_correction_applied"]    = strBooltrue;
   m_strResponse["threshold_energy"]                 = " { \"value_type\": \"float\",  \"value\": 4000.0 }";
   m_strResponse["photon_energy"]                    = " { \"value_type\": \"float\",  \"value\": 8000.0 }";
   m_strResponse["th0_temp"]                         = " { \"value_type\": \"float\",  \"value\": 26.0 }";
   m_strResponse["th0_humidity"]                     = " { \"value_type\": \"float\",  \"value\": 60.0 }";

   m_strResponse["/filewriter/api/SIMAPI1.0/status/state"] = " { \"value_type\": \"string\", \"value\": \"%s\" }";
   m_strResponse["/detector/api/SIMAPI1.0/status/state"]   = " { \"value_type\": \"string\", \"value\": \"%s\" }";
   m_strResponse["/data/lima_data_000001.h5"] = "/data/lima_data_000001.h5";
}



// ---------------------------------------------------------------------------
void __fastcall TForm_Main::FormClose(TObject* /* Sender */ , TCloseAction& /* Action */ )
{
   if (NULL != m_hDll) FreeLibrary(m_hDll);
}


// ---------------------------------------------------------------------------
void __fastcall TForm_Main::HTTPServerCommandGet(TIdContext *AContext,
                                             TIdHTTPRequestInfo *ARequestInfo,
                                             TIdHTTPResponseInfo *AResponseInfo)

{
   Form_HTTPView->RichEdit->Lines->Add(ARequestInfo->RemoteIP+": ["+ARequestInfo->AuthUsername+"/"+ARequestInfo->AuthPassword+"] "+ ARequestInfo->RawHTTPCommand);
   Application->ProcessMessages();

   AResponseInfo->ContentText = "Request not recognized.";

   AnsiString strCommand = ARequestInfo->RawHTTPCommand;

   map <AnsiString, AnsiString>::const_iterator iter = m_strResponse.begin();
   bool found = false;

   while ( (!found) && (m_strResponse.end() != iter) )
   {
      if (strCommand.Pos(iter->first) )
      {
         AnsiString strAnswer = iter->second;
         if (strCommand.Pos("/filewriter/api/SIMAPI1.0/status/state")) //
         {
            strAnswer.sprintf(strAnswer.c_str(), ComboBoxFWstatus->Items->Strings[ComboBoxFWstatus->ItemIndex].t_str());
         }
         else if (strCommand.Pos("/detector/api/SIMAPI1.0/status/state"))
         {
            //Form_HTTPView->RichEdit->Lines->Add( IntToStr(ComboBoxDETstatus->ItemIndex) );
            //Form_HTTPView->RichEdit->Lines->Add(ComboBoxDETstatus->Items->Strings[ComboBoxDETstatus->ItemIndex]);
            strAnswer.sprintf(strAnswer.c_str(), ComboBoxDETstatus->Items->Strings[ComboBoxDETstatus->ItemIndex].t_str());
         }
         else if (strCommand.Pos("/data/lima_data_000001.h5") )
         {
            Form_HTTPView->RichEdit->Lines->Add("Servefile ...");
            AResponseInfo->ServeFile(AContext, "lima_data_000001.h5");
            Form_HTTPView->RichEdit->Lines->Add("Done.");
            return; // avoid further header writing in this case
         }
         else if (strCommand.Pos("/data/lima_master.h5") )
         {
            Form_HTTPView->RichEdit->Lines->Add("Servefile ...");
            AResponseInfo->ServeFile(AContext, "lima_master.h5");
            Form_HTTPView->RichEdit->Lines->Add("Done.");
            return; // avoid further header writing in this case
         }

         AResponseInfo->ContentText = strAnswer;
         Form_HTTPView->RichEdit->Lines->Add(strAnswer);
         found = true;
      }
      ++iter;
   }

   AResponseInfo->WriteHeader();
   AResponseInfo->WriteContent();
}


// ---------------------------------------------------------------------------
void __fastcall TForm_Main::FormShow(TObject*)
{
   Form_HTTPView->Show();
}

void __fastcall TForm_Main::Button1Click(TObject*)
{
 Form_HTTPView->Visible = ! Form_HTTPView->Visible;
}
//---------------------------------------------------------------------------

void __fastcall TForm_Main::HTTPServerCommandOther(TIdContext*,
                                                   TIdHTTPRequestInfo* ARequestInfo,
                                                   TIdHTTPResponseInfo* AResponseInfo)

{
   Form_HTTPView->RichEdit->Lines->Add(ARequestInfo->RemoteIP+": ["+ARequestInfo->AuthUsername+"/"+ARequestInfo->AuthPassword+"] "+ ARequestInfo->RawHTTPCommand);
   if (ARequestInfo && ARequestInfo->PostStream)
   {
      char* buff = new char[ARequestInfo->PostStream->Size + 2];
      ARequestInfo->PostStream->Read(buff, ARequestInfo->PostStream->Size);
      buff[ARequestInfo->PostStream->Size] = '\0';
      Form_HTTPView->RichEdit->Lines->Add( buff );
      delete[] buff;
   }

   AnsiString strCommand = ARequestInfo->RawHTTPCommand;
   if (strCommand.Pos("/command/"))
   {
      AResponseInfo->ResponseNo = StrToInt(EditCMDRespCode->Text);
      Form_HTTPView->RichEdit->Lines->Add(AResponseInfo->ResponseText +" "+ IntToStr(AResponseInfo->ResponseNo));

      if (strCommand.Pos("/arm"))
      {
         Sleep(EditDuration_arm->Text.ToInt());
      }
      else if (strCommand.Pos("/trigger"))
      {
         Sleep(EditDuration_trigger->Text.ToInt());
      }
   }
}
//---------------------------------------------------------------------------

