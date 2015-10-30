//---------------------------------------------------------------------------

#ifndef HTTPViewH
#define HTTPViewH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
//---------------------------------------------------------------------------
class TForm_HTTPView : public TForm
{
__published:	// Composants gérés par l'EDI
   TRichEdit *RichEdit;
private:	// Déclarations utilisateur
public:		// Déclarations utilisateur
   __fastcall TForm_HTTPView(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm_HTTPView *Form_HTTPView;
//---------------------------------------------------------------------------
#endif
