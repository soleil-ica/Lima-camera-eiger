//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------
USEFORM("HTTPView.cpp", Form_HTTPView);
USEFORM("Main_Client_UDP.cpp", Form_Main);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
        try
        {
           Application->Initialize();
           Application->CreateForm(__classid(TForm_Main), &Form_Main);
       Application->CreateForm(__classid(TForm_HTTPView), &Form_HTTPView);
       Application->Run();
        }
        catch (Exception &exception)
        {
                 Application->ShowException(&exception);
        }
        catch (...)
        {
                 try
                 {
                         throw Exception("");
                 }
                 catch (Exception &exception)
                 {
                         Application->ShowException(&exception);
                 }
        }
        return 0;
}
//---------------------------------------------------------------------------
