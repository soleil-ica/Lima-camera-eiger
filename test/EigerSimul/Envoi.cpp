// ---------------------------------------------------------------------------

#pragma hdrstop

#include "Envoi.h"
#include "Main_Client_UDP.h"
#include <vcl.h>

#pragma package(smart_init)
// ---------------------------------------------------------------------------

// Important : les méthodes et les propriétés des objets de la VCL ne peuvent être
// utilisées que dans une méthode appelée en utilisant Synchronize, comme :
//
// Synchronize(UpdateCaption);
//
// où UpdateCaption serait de la forme :
//
// void __fastcall Envoi::UpdateCaption()
// {
// Form1->Caption = "Mise à jour dans un thread";
// }
// ---------------------------------------------------------------------------

// RSSI value cosine table
const int Envoi::m_RSSIValues[21] =
{
   100, 98, 91, 81, 68, 54, 39, 25, 13, 5, 1, 1, 5, 14, 25, 39, 54, 69, 82, 92, 98
};

__fastcall Envoi::Envoi(bool CreateSuspended) : TThread(CreateSuspended)
{
   Priority = tpTimeCritical;
   m_iNombreEnCours = 0;
   m_iGlobalFrameCounter = 0;
   m_iLastProgressPercent = 0;
   QueryPerformanceFrequency((LARGE_INTEGER*) & lpFrequency); // ticks per second
   lpFrequency /= 1000; // ticks per ms
   m_iRSSIValuesMax = 20;
}

// ---------------------------------------------------------------------------
// TODO: Gérer le stop/start
// actuellement le stop "suspend" le thread, mais le start suivant reprendra l'opération où elle en était ...
void __fastcall Envoi::Execute()
{
   int iTemp, iRSSIvalueindex = 0;
   AnsiString sTemp;
   AnsiString strMsgToSend;

   while (!Terminated)
   {
      while (m_iNombreEnCours != m_iNombre)
      {
         m_iNombreEnCours++;

         // Wait the delay
         QueryPerformanceCounter((LARGE_INTEGER*) & lpPerformanceCount1);
         __int64 iTicksCount;
         do
         {
            QueryPerformanceCounter((LARGE_INTEGER*) & lpPerformanceCount2);
            iTicksCount = lpPerformanceCount2 - lpPerformanceCount1;
         }
         while (iTicksCount < m_iDelais * lpFrequency);

         // Update frame parameters
         strMsgToSend.sprintf(m_sMessage.t_str(), m_iGlobalFrameCounter, m_RSSIValues[iRSSIvalueindex++]);
         if (iRSSIvalueindex > m_iRSSIValuesMax)
         {
            iRSSIvalueindex = 0;
         }

         m_iGlobalFrameCounter += m_iDelais;

         // Send the frame
         Form_Main->UDPClient->Send(strMsgToSend);

         // Progress bar update
         if (-1 != m_iNombre)
         {
            int ProgressPercent = 1.0 * m_iNombreEnCours / m_iNombre * 100;
            if (ProgressPercent != m_iLastProgressPercent)
            {
               Form_Main->ProgressBar1->StepIt();
               m_iLastProgressPercent = ProgressPercent;
            }
         }
      }

      // Reset counters
      Reset();

      Form_Main->B_StopClick(); // job done
   }
}

// ---------------------------------------------------------------------------
void __fastcall Envoi::fctSetNombre(int iTemp)
{
   m_iNombre = iTemp;
}

// ---------------------------------------------------------------------------
void __fastcall Envoi::fctSetDelais(int iTemp)
{
   m_iDelais = iTemp;
}

// ---------------------------------------------------------------------------
void __fastcall Envoi::fctSetMessage(String sTemp)
{
   m_sMessage = sTemp;
}

// ---------------------------------------------------------------------------
void __fastcall Envoi::Reset()
{
   m_iNombreEnCours = 0;
   m_iGlobalFrameCounter = 0;
   m_iLastProgressPercent = 0;
   Form_Main->ProgressBar1->Position = 0;
}
