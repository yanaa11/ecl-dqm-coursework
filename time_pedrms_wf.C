#include "TXMLEngine.h"
#include <string>
#include <cstdlib>
#include <iostream>

//             								CODE TO GET THETA_ID AND PHI_ID FROM CELLID 

// For getPhi, getTheta functions.
const int m_ring_start_id[70] = {
  // forward (theta_id 0-12)
  1,    49,   97,   161,  225,  289,  385,  481,  577,  673,
  769,  865,  1009,
  // barrel (theta_id 13-58)
  1153, 1297, 1441, 1585, 1729, 1873, 2017, 2161, 2305, 2449,
  2593, 2737, 2881, 3025, 3169, 3313, 3457, 3601, 3745, 3889,
  4033, 4177, 4321, 4465, 4609, 4753, 4897, 5041, 5185, 5329,
  5473, 5617, 5761, 5905, 6049, 6193, 6337, 6481, 6625, 6769,
  6913, 7057, 7201, 7345, 7489, 7633,
  // forward (theta_id 59-68)
  7777, 7921, 8065, 8161, 8257, 8353, 8449, 8545, 8609, 8673,
  // last_crystal+1
  8736+1
};

int getPhi(int ch)
{
  for (int i = 0; i < 69; i++) {
    if (ch < m_ring_start_id[i+1])
      return ch - m_ring_start_id[i];
  }

  return -1;
}

int getTheta(int ch)
{
  for (int i = 0; i < 69; i++) {
    if (ch < m_ring_start_id[i+1])
      return i;
  }

  return -1;
}

//                                                    PROCESS DATA 

void time_pedrms_wf(const char* filename = "ecl_run_summary.xml")
{
  //                                                  GET RUN INFO AND CHECK RUN

  //measure processing time 
  TStopwatch* sw = new TStopwatch();
  sw->Start();

  //get run and exp from _file0 name
  std::string fname(_file0->GetName()); 
  int exp = std::stoi(fname.substr(fname.find("e0")+1,4));
  int run = std::stoi(fname.substr(fname.find("r0")+1,6));

  //change name for output file
  //std::string outfile = "eclout_e" + std::to_string(exp) + "r" + std::to_string(run) + ".root";
  fname.replace(fname.begin(), fname.begin()+7, "eclout_");

  //check run
  TH1F* rtype = (TH1F*)_file0->FindObjectAny("DQMInfo/rtype"); 
  if (!rtype) return; 

  //get runtype 
  const char* runtype = rtype->GetTitle();
  //std::cout << runtype << std::endl;

  //get date and time
  TKey* key = (TKey*)_file0->FindKey("DQMInfo/rtype");
  const char* datime = key->GetDatime().AsSQLString();

  //std::cout << "run " << run << std::endl;

  //												 GET PEDRMS AND ADC DATA
  //get TProfile with pedRMS
  TProfile* pedrms_cellid = (TProfile*)_file0->FindObjectAny("ECL/pedrms_cellid");

  //get ADC waveform info 
  TH1F* wf_cid_dphy = (TH1F*)_file0->FindObjectAny("ECL/wf_cid_dphy");
  if(!wf_cid_dphy) return;
  TH1F* wf_cid_logic = (TH1F*)_file0->FindObjectAny("ECL/wf_cid_logic");
  if(!wf_cid_logic) return;
  TH1F* wf_cid_rand = (TH1F*)_file0->FindObjectAny("ECL/wf_cid_rand");
  if(!wf_cid_rand) return;
  TH1F* wf_cid_psd = (TH1F*)_file0->FindObjectAny("ECL/wf_cid_psd");
  if(!wf_cid_psd) return;

  TH1F* event_dphy = (TH1F*)_file0->FindObjectAny("ECL/event_dphy");
  if(!event_dphy) return;
  TH1F* event_logic = (TH1F*)_file0->FindObjectAny("ECL/event_logic");
  if(!event_logic) return;
  TH1F* event_rand = (TH1F*)_file0->FindObjectAny("ECL/event_rand");
  if(!event_rand) return;
  TH1F* psd_cid = (TH1F*)_file0->FindObjectAny("ECL/psd_cid");
  if(!psd_cid) return;


  // 												 DECLARE ALL

  //53 for easier numeration in variables (use 1 to 52 as indices)
  //for time histograms
  double entries[53]; //(Integral)
  double mean[53];
  double merror[53];
  double sigma[53];
  double serror[53];
  double chi2[53];

  //5 canvases for fitted time time_histograms, crates are divided as in charts 
  TCanvas* canvas_1_12 = new TCanvas("Barrel_1_to_12", "Barrel_1_to_12");
  canvas_1_12->Divide(3, 4);

  TCanvas* canvas_13_24 = new TCanvas("Barrel_13_to_24", "Barrel_13_to_24");
  canvas_13_24->Divide(3, 4);

  TCanvas* canvas_25_36 = new TCanvas("Barrel_25_to_36", "Barrel_25_to_36");
  canvas_25_36->Divide(3, 4);

  TCanvas* canvas_forward = new TCanvas("Forward", "Forward");
  canvas_forward->Divide(2, 4);

  TCanvas* canvas_backward = new TCanvas("Backward", "Backward");
  canvas_backward->Divide(2, 4);

  //for ADC waveform fractions
  double min_fraction_dphy = 2.0;
  double min_fraction_logic = 2.0;
  double min_fraction_rand = 2.0;
  double min_fraction_psd = 2.0;

  //for mean pedpms phi histograms (7 theta intervals)
  TCanvas* canvas_pedrms_phi = new TCanvas("pedrms_phi", "pedrms_phi");
  canvas_pedrms_phi->Divide(2, 4);

  //for average pedrms(phi) on canvas  (phi from 0)
  TProfile* pedrms_phi_fw1 = (TProfile*)new TProfile("pedrms_phi_fw1", "Pedestal RMS vs. Phi ID (0 <= theta < 5: Forward 1)", 64, 0, 63); 
  pedrms_phi_fw1->SetXTitle("Phi ID");
  pedrms_phi_fw1->SetYTitle("Pedestal RMS");
  TProfile* pedrms_phi_fw2 = (TProfile*)new TProfile("pedrms_phi_fw2", "Pedestal RMS vs. Phi ID (5 <= theta < 11: Forward 2)", 96, 0, 95); 
  pedrms_phi_fw2->SetXTitle("Phi ID");
  pedrms_phi_fw2->SetYTitle("Pedestal RMS");
  TProfile* pedrms_phi_b1 = (TProfile*)new TProfile("pedrms_phi_b1", "Pedestal RMS vs. Phi ID (11 <= theta < 32: Barrel 1)", 144, 0, 143); 
  pedrms_phi_b1->SetXTitle("Phi ID");
  pedrms_phi_b1->SetYTitle("Pedestal RMS");
  TProfile* pedrms_phi_b2 = (TProfile*)new TProfile("pedrms_phi_b2", "Pedestal RMS vs. Phi ID (32 <= theta < 42: Barrel 2)", 144, 0, 143); 
  pedrms_phi_b2->SetXTitle("Phi ID");
  pedrms_phi_b2->SetYTitle("Pedestal RMS");
  TProfile* pedrms_phi_b3 = (TProfile*)new TProfile("pedrms_phi_b3", "Pedestal RMS vs. Phi ID (42 <= theta < 61: Barrel 3)", 144, 0, 143); 
  pedrms_phi_b3->SetXTitle("Phi ID");
  pedrms_phi_b3->SetYTitle("Pedestal RMS");
  TProfile* pedrms_phi_bk1 = (TProfile*)new TProfile("pedrms_phi_bk1", "Pedestal RMS vs. Phi ID (61 <= theta < 66: Backward 1)", 96, 0, 95); 
  pedrms_phi_bk1->SetXTitle("Phi ID");
  pedrms_phi_bk1->SetYTitle("Pedestal RMS");
  TProfile* pedrms_phi_bk2 = (TProfile*)new TProfile("pedrms_phi_bk2", "Pedestal RMS vs. Phi ID (66 <= theta <= 68: Backward 2)", 64, 0, 63); 
  pedrms_phi_bk2->SetXTitle("Phi ID");
  pedrms_phi_bk2->SetYTitle("Pedestal RMS");

  //for average pedrms(theta) on chart
  double total_fw1 = 0;
  double total_fw2 = 0;
  double total_b1 = 0;
  double total_b2 = 0;
  double total_b3 = 0;
  double total_bk1 = 0;
  double total_bk2 = 0;

  double entries_fw1 = 0;
  double entries_fw2 = 0;
  double entries_b1 = 0;
  double entries_b2 = 0;
  double entries_b3 = 0;
  double entries_bk1 = 0;
  double entries_bk2 = 0;

  
  //                                          LOOP OVER 8736 CELLS

  int i = 1;
  for(i = 1; i < 8737; i++)
  {
    // 								ADC waveform fraction

    double current_fraction_dphy = (double) wf_cid_dphy->GetBinContent(i)/event_dphy->GetBinContent(1);
    if(current_fraction_dphy < min_fraction_dphy)
      min_fraction_dphy = current_fraction_dphy;

    double current_fraction_logic = (double) wf_cid_logic->GetBinContent(i)/event_logic->GetBinContent(1);
    if(current_fraction_logic < min_fraction_logic)
      min_fraction_logic = current_fraction_logic;

    double current_fraction_rand = (double) wf_cid_rand->GetBinContent(i)/event_rand->GetBinContent(1);
    if(current_fraction_rand < min_fraction_rand)
      min_fraction_rand = current_fraction_rand;

    double current_fraction_psd = (double) wf_cid_psd->GetBinContent(i)/psd_cid->GetBinContent(i);
    if(current_fraction_psd < min_fraction_psd)
      min_fraction_psd = current_fraction_psd;

    // 								pedRMS

    int theta = getTheta(i);
    int phi = getPhi(i);

    if(theta < 5) //FW1
    {
    	//calculate mean for all phi and theta
      	//fill histogram 
    	total_fw1 += pedrms_cellid->GetBinContent(i);
    	entries_fw1 += pedrms_cellid->GetBinEntries(i);
    	pedrms_phi_fw1->Fill(phi, pedrms_cellid->GetBinContent(i));
    }
    else if(theta < 11) //FW2
    {
    	total_fw2 += pedrms_cellid->GetBinContent(i);
    	entries_fw2 += pedrms_cellid->GetBinEntries(i);
    	pedrms_phi_fw2->Fill(phi, pedrms_cellid->GetBinContent(i));
    }
    else if(theta < 32) //BARREL1
    {
    	total_b1 += pedrms_cellid->GetBinContent(i);
    	entries_b1 += pedrms_cellid->GetBinEntries(i);
    	pedrms_phi_b1->Fill(phi, pedrms_cellid->GetBinContent(i));
    }
    else if(theta < 42) //BARREL2
    {
    	total_b2 += pedrms_cellid->GetBinContent(i);
    	entries_b2 += pedrms_cellid->GetBinEntries(i);
    	pedrms_phi_b2->Fill(phi, pedrms_cellid->GetBinContent(i));
    }
    else if(theta < 61) //BARREL3
    {
    	total_b3 += pedrms_cellid->GetBinContent(i);
    	entries_b3 += pedrms_cellid->GetBinEntries(i);
    	pedrms_phi_b3->Fill(phi, pedrms_cellid->GetBinContent(i));
    }
    else if(theta < 66) //BCKW1
    {
    	total_bk1 += pedrms_cellid->GetBinContent(i);
    	entries_bk1 += pedrms_cellid->GetBinEntries(i);
    	pedrms_phi_bk1->Fill(phi, pedrms_cellid->GetBinContent(i));
    }
    else if(theta <= 68) //BCKW2
    {
    	total_bk2 += pedrms_cellid->GetBinContent(i);
    	entries_bk2 += pedrms_cellid->GetBinEntries(i);
    	pedrms_phi_bk2->Fill(phi, pedrms_cellid->GetBinContent(i));
    }

  } //                                     END LOOP OVER CELLS

  //calculate average pedrms for each theta interval 
  double avg_pedrms_fw1 = total_fw1/entries_fw1;
  double avg_pedrms_fw2 = total_fw2/entries_fw2;
  double avg_pedrms_b1 = total_b1/entries_b1;
  double avg_pedrms_b2 = total_b2/entries_b2;
  double avg_pedrms_b3 = total_b3/entries_b3;
  double avg_pedrms_bk1 = total_bk1/entries_bk1;
  double avg_pedrms_bk2 = total_bk2/entries_bk2;

  //add pedrms_phi to canvas
  canvas_pedrms_phi->cd(1);
  pedrms_phi_fw1->Draw();
  canvas_pedrms_phi->cd(2);
  pedrms_phi_fw2->Draw();
  canvas_pedrms_phi->cd(3);
  pedrms_phi_b1->Draw();
  canvas_pedrms_phi->cd(4);
  pedrms_phi_b2->Draw();
  canvas_pedrms_phi->cd(5);
  pedrms_phi_b3->Draw();
  canvas_pedrms_phi->cd(6);
  pedrms_phi_bk1->Draw();
  canvas_pedrms_phi->cd(7);
  pedrms_phi_bk2->Draw();

	
  //                                       LOOP OVER 52 CRATES

  //52 time_histograms (for each crate) for each .root file

  for(i = 1; i < 53; i++)
  {
	std::string time_histname = "ECL/time_crate_" + std::to_string(i) + "_Thr1GeV";

    //"open" corresponding canvas
    if(i <= 12)
      canvas_1_12->cd(i); 

    else if(i <= 24)
      canvas_13_24->cd(i-12);

    else if(i <= 36)
      canvas_25_36->cd(i-24);

    else if(i <= 44)
      canvas_forward->cd(i-36);

    else if(i <= 52)
      canvas_backward->cd(i-44);

    //extract time_histogram
	TH1F* time_hist = (TH1F*)_file0->FindObjectAny(time_histname.c_str());

	//if problems with one time_histogram, don't write all run 
	//check time_histogram: 
	if(!time_hist)
		return;

	entries[i] = time_hist->Integral();
	if(entries[i] < 2000)
		return;

	//fitting
	TFitResultPtr fit = time_hist->Fit("gaus", "QS");
	Int_t fitStatus = fit; //The fitStatus is 0 if the fit is OK (i.e no error occurred). 
	if(fitStatus != 0)
		return;

	//extract variables from the time_histogram
	mean[i] = fit->Parameter(1);
    merror[i] = fit->ParError(1);
    sigma[i] = fit->Parameter(2);
    serror[i] = fit->ParError(2);
    chi2[i] = fit->Chi2();

    //add time_histogram to the canvas
    time_hist->Draw();
  } //                                  END LOOP OVER CRATES 

  //std::cout << "run " << run << std::endl;


//                                    SAVE ALL RESULTS TO .ROOT AND XML

  //write canvases to output .root file
  TFile* fout = new TFile(fname.c_str(),"UPDATE");

  canvas_1_12->Write();
  canvas_13_24->Write();
  canvas_25_36->Write();
  canvas_forward->Write();
  canvas_backward->Write();
  canvas_pedrms_phi->Write();

  fout->Close();

  // add a run node to run summary xml file
  TXMLEngine xml;
  XMLDocPointer_t xmldoc = xml.ParseFile(filename);
  XMLNodePointer_t mainnode;
  int nw=0;
  if ( !xmldoc )
  	{
    	xmldoc = xml.NewDoc();
    	mainnode = xml.NewChild(0, 0, "data");
    	nw=1;
  	}
  else mainnode = xml.DocGetRootElement(xmldoc);
  
  XMLNodePointer_t info = xml.NewChild(mainnode, 0, "info");

  //add run info to xml
  xml.NewChild(info, 0, "run", (std::to_string(run)).c_str());
  xml.NewChild(info, 0, "exp", (std::to_string(exp)).c_str());
  xml.NewChild(info, 0, "datetime", datime);
  xml.NewChild(info, 0, "runtype", runtype);

  //add ADC waveform fraction variables to xml
  xml.NewChild(info, 0, "fraction_dphy", (std::to_string(min_fraction_dphy)).c_str());
  xml.NewChild(info, 0, "fraction_logic", (std::to_string(min_fraction_logic)).c_str());
  xml.NewChild(info, 0, "fraction_rand", (std::to_string(min_fraction_rand)).c_str());
  xml.NewChild(info, 0, "fraction_psd", (std::to_string(min_fraction_psd)).c_str());

  //add pedRMS info to xml
  xml.NewChild(info, 0, "avg_pedrms_fw1", (std::to_string(avg_pedrms_fw1)).c_str());
  xml.NewChild(info, 0, "entries_fw1", (std::to_string(entries_fw1)).c_str());
  xml.NewChild(info, 0, "avg_pedrms_fw2", (std::to_string(avg_pedrms_fw2)).c_str());
  xml.NewChild(info, 0, "entries_fw2", (std::to_string(entries_fw2)).c_str());
  xml.NewChild(info, 0, "avg_pedrms_b1", (std::to_string(avg_pedrms_b1)).c_str());
  xml.NewChild(info, 0, "entries_b1", (std::to_string(entries_b1)).c_str());
  xml.NewChild(info, 0, "avg_pedrms_b2", (std::to_string(avg_pedrms_b2)).c_str());
  xml.NewChild(info, 0, "entries_b2", (std::to_string(entries_b2)).c_str());
  xml.NewChild(info, 0, "avg_pedrms_b3", (std::to_string(avg_pedrms_b3)).c_str());
  xml.NewChild(info, 0, "entries_b3", (std::to_string(entries_b3)).c_str());
  xml.NewChild(info, 0, "avg_pedrms_bk1", (std::to_string(avg_pedrms_bk1)).c_str());
  xml.NewChild(info, 0, "entries_bk1", (std::to_string(entries_bk1)).c_str());
  xml.NewChild(info, 0, "avg_pedrms_bk2", (std::to_string(avg_pedrms_bk2)).c_str());
  xml.NewChild(info, 0, "entries_bk2", (std::to_string(entries_bk2)).c_str());

  //add time variables to xml
  for(i = 1; i < 53; i++)
  	{
      //generate names
  		std::string entries_n = "entries_" + std::to_string(i);
  		std::string mean_n = "mean_" + std::to_string(i);
  		std::string merror_n = "mean_error_" + std::to_string(i);
  		std::string sigma_n = "sigma_" + std::to_string(i);
  		std::string serror_n = "sigma_error_" + std::to_string(i);
  		std::string chi2_n = "chi2_" + std::to_string(i);

      //add to xml
  		xml.NewChild(info, 0, entries_n.c_str(), (std::to_string(entries[i])).c_str());
  		xml.NewChild(info, 0, mean_n.c_str(), (std::to_string(mean[i])).c_str());
  		xml.NewChild(info, 0, merror_n.c_str(), (std::to_string(merror[i])).c_str());
  		xml.NewChild(info, 0, sigma_n.c_str(), (std::to_string(sigma[i])).c_str());
  		xml.NewChild(info, 0, serror_n.c_str(), (std::to_string(serror[i])).c_str());
  		xml.NewChild(info, 0, chi2_n.c_str(), (std::to_string(chi2[i])).c_str());
  	}

  	if (nw == 1) xml.DocSetRootElement(xmldoc, mainnode);
  	xml.SaveDoc(xmldoc, filename);
  	xml.FreeDoc(xmldoc);

  sw->Stop();
  //sw->Print();
  std::cout << sw->RealTime() << std::endl;
  std::cout << sw->CpuTime() << std::endl;
}