




        TH1D *hist = (TH1D*)(GetOutputList()->FindObject(Form("Charge_0x%04x",fragment->ChannelAddress)));
	if(hist) hist->Fill(fragment->Charge.at(0)/512.0);

	hist = (TH1D*)(GetOutputList()->FindObject(Form("Energy_0x%04x",fragment->ChannelAddress)));
	if(hist) hist->Fill(channel->CalibrateENG(fragment->Charge.at(0)));
        
	TH1D *histcfd = (TH1D*)(GetOutputList()->FindObject(Form("Cfd_0x%04x",fragment->ChannelAddress)));
        if(histcfd) histcfd->Fill(fragment->Cfd.at(0));

	TH2D *mat = (TH2D*)(GetOutputList()->FindObject("hp_charge"));
	if(mat) mat->Fill(channel->GetNumber(),fragment->Charge.at(0)/512.0);

	hist = (TH1D*)(GetOutputList()->FindObject("test"));
	if(hist) hist->Fill(channel->GetNumber());

	hist = (TH1D*)(GetOutputList()->FindObject("PPG"));
	if(hist) hist->Fill(fragment->PPG);
        
	hist = (TH1D*)(GetOutputList()->FindObject("ChannelId"));
	if(hist) hist->Fill(fragment->ChannelId);

	hist = (TH1D*)(GetOutputList()->FindObject("MidasId"));
	if(hist) hist->Fill(fragment->MidasId);

	hist = (TH1D*)(GetOutputList()->FindObject("MidasTimeStamp"));
	if(hist) hist->Fill(fragment->MidasTimeStamp);

	hist = (TH1D*)(GetOutputList()->FindObject("TriggerId"));
	if(hist) hist->Fill(fragment->TriggerId);

	hist = (TH1D*)(GetOutputList()->FindObject("Charge_nofilter"));
        if(hist && fragment->TriggerId < 0) hist->Fill(fragment->Charge.at(0)/512.0);

        hist = (TH1D*)(GetOutputList()->FindObject("Charge_filter"));
        if(hist && fragment->TriggerId > 0) hist->Fill(fragment->Charge.at(0)/512.0); 

	hist = (TH1D*)(GetOutputList()->FindObject("TriggerPattern"));
        if(hist && fragment->TriggerBitPattern>-1) hist->Fill(fragment->TriggerBitPattern);


