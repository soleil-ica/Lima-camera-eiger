//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "HTTPView.h"
#include "Main_Client_UDP.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm_HTTPView *Form_HTTPView;

//---------------------------------------------------------------------------
__fastcall TForm_HTTPView::TForm_HTTPView(TComponent* Owner)
   : TForm(Owner)
{
   Left = Form_Main->Left + Form_Main->Width;
   Top  = Form_Main->Top;
}
//---------------------------------------------------------------------------
