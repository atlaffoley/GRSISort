#ifndef TGRSIDETECTORHIT_H
#define TGRSIDETECTORHIT_H


#include "Globals.h"

#include <cstdio>
#include <utility>

//#include "TChannel.h"
#include <TVector3.h> 
#include <TObject.h> 
#include <Rtypes.h>


class TGRSIDetectorHit : public TObject 	{
	public:
		TGRSIDetectorHit();
		virtual ~TGRSIDetectorHit();

	public:

		virtual void Clear(Option_t* opt = "");	//!
		virtual void Print(Option_t* opt = "");	//!

		//static bool Compare(TGRSIDetectorHit *lhs,TGRSIDetectorHit *rhs); //!

		//virtual TVector3 GetPosition() = 0;	//!
		//virtual void SetPosition(TGRSIDetectorHit &) = 0;	//!

	ClassDef(TGRSIDetectorHit,1)
};




#endif
