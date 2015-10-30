//---------------------------------------------------------------------------

#ifndef EnvoiH
#define EnvoiH
//---------------------------------------------------------------------------
#include <Classes.hpp>

//---------------------------------------------------------------------------
class Envoi : public TThread
{
private:
	static const int m_RSSIValues[21];
	int m_iRSSIValuesMax;

protected:
   void __fastcall Execute();
   int m_iNombreEnCours;
   int m_iNombre;
	int m_iDelais;
	unsigned long int m_iGlobalFrameCounter;
	int m_iLastProgressPercent;
   String m_sMessage;
/*   LARGE_INTEGER lpFrequency;
	LARGE_INTEGER lpPerformanceCount1 , lpPerformanceCount2 , lpTot;*/

	__int64 lpFrequency;
	__int64 lpPerformanceCount1 , lpPerformanceCount2 , lpTot;

public:
   __fastcall Envoi(bool CreateSuspended);
   void __fastcall fctSetNombre(int);
   void __fastcall fctSetDelais(int);
	void __fastcall fctSetMessage(String);
	void __fastcall Reset();

};
//---------------------------------------------------------------------------
#endif
