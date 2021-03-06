#define TFragmentSelector_cxx

#include "TGRSIOptions.h"

// The class definition in TFragmentSelector.h has been generated automatically
// by the ROOT utility TTree::MakeSelector(). This class is derived
// from the ROOT class TSelector. For more information on the TSelector
// framework see $ROOTSYS/README/README.SELECTOR or the ROOT User Manual.

// The following methods are defined in this file:
//    Begin():        called every time a loop on the tree starts,
//                    a convenient place to create your histograms.
//    SlaveBegin():   called after Begin(), when on PROOF called only on the
//                    slave servers.
//    Process():      called for each event, in this function you decide what
//                    to read and fill your histograms.
//    SlaveTerminate: called at the end of the loop on the tree, when on PROOF
//                    called only on the slave servers.
//    Terminate():    called at the end of the loop on the tree,
//                    a convenient place to draw/fit your histograms.
//
// To use this file, try the following session on your Tree T:
//
// Root > T->Process("TFragmentSelector.C")
// Root > T->Process("TFragmentSelector.C","some options")
// Root > T->Process("TFragmentSelector.C+")
//


///////////////////////////////////////////////////////////////////
//
// TFragmentSelector
//
// TFragmentSelector uses PROOF to process TFragments into user defined
// histograms (as well as a TAnalysisTree?). The User defined histograms are
// intialized in users/UserInitObj.h and filled in users/UserFillObj.h
// PROOF allows for parrallel processing of TFragments.
//
/////////////////////////////////////////////////////////////////////

#include "TFragmentSelector.h"
#include <TH2.h>
#include <TStyle.h>

void TFragmentSelector::Begin(TTree * /*tree*/)
{
   // The Begin() function is called at the start of the query.
   // When running with PROOF Begin() is only called on the client.
   // The tree argument is deprecated (on PROOF 0 is passed).

   TString option = GetOption();
}

void TFragmentSelector::SlaveBegin(TTree * /*tree*/)
{
   // The SlaveBegin() function is called after the Begin() function.
   // When running with PROOF SlaveBegin() is called on each slave server.
   // The tree argument is deprecated (on PROOF 0 is passed).

   #include "UserInitObj.h"
   TString option = GetOption();

}

Bool_t TFragmentSelector::Process(Long64_t entry)
{
   // The Process() function is called for each entry in the tree (or possibly
   // keyed object in the case of PROOF) to be processed. The entry argument
   // specifies which entry in the currently loaded tree is to be processed.
   // It can be passed to either TFragmentSelector::GetEntry() or TBranch::GetEntry()
   // to read either all or the required parts of the data. When processing
   // keyed objects with PROOF, the object is already loaded and is available
   // via the fObject pointer.
   //
   // This function should contain the "body" of the analysis. It can contain
   // simple or elaborate selection criteria, run algorithms on the data
   // of the event and typically fill histograms.
   //
   // The processing can be stopped by calling Abort().
   //
   // Use fStatus to set the return value of TTree::Process().
   //
   // The return value is currently not used.

   fChain->GetEntry(entry);
   TChannel *channel = TChannel::GetChannel(fragment->ChannelAddress);
   if(!channel){
      return true;   // I think if we don't have a channel at this point (i.e. the channel
                  // was not defined a name/address/odb; we should drop the fragment.  
                  // The TSelector by design is made to crunch lots of data, worrying about 
                  // whether we are dropping good events should have already happen by this stage.  pdb.
//     channel = new TChannel();
//     channel->SetAddress(fragment->ChannelAddress);
//     TChannel::AddChannel(channel);
   }
   
  // if(TChannel::GetNumberOfChannels() != 0 ) 
	   #include "UserFillObj.h"
       //  gSystem->CompileMacro("UserFillObj.h");

   return kTRUE;
}

void TFragmentSelector::SlaveTerminate()
{
   // The SlaveTerminate() function is called after all entries or objects
   // have been processed. When running with PROOF SlaveTerminate() is called
   // on each slave server.

}

void TFragmentSelector::Terminate()
{
   // The Terminate() function is the last function to be called during
   // a query. It always runs on the client, it can be used to present
   // the results graphically or save the results to file.

   char* histsname;
   if(fsubrunnumber == -1)
      histsname = Form("hists%05i.root",frunnumber);
   else
      histsname = Form("hists%05i_%03i.root",frunnumber,fsubrunnumber);

   TFile f(histsname,"recreate");
   std::string rootfilename = f.GetName();
//   TGRSIOptions::AddInputRootFile(rootfilename);   this real messess up so library loading... pcb.
/*
   if(fsubrunnumber == -1){
      TFile f(Form("hists%05i.root",frunnumber),"recreate");
   }
   else{
      TFile f(Form("hists%05i_%03i.root",frunnumber,fsubrunnumber),"recreate");
   }*/
   TIter iter(this->GetOutputList());
   while(TObject *obj =iter.Next()) {
	if(obj->InheritsFrom("TH1"))
		obj->Write();
   }

}
