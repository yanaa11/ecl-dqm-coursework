#include "TXMLEngine.h"
#include <string>
#include <cstdlib>
#include <iostream>


void fittime2(const char* filename = "ecl_run_summary.xml")
{
	//get run and exp from _file0 name
	std::string fname(_file0->GetName()); 
  	int exp = std::stoi(fname.substr(fname.find("e0")+1,4));
  	int run = std::stoi(fname.substr(fname.find("r0")+1,6));

  	//change name for out file
  	fname.replace(fname.begin(), fname.begin()+7,"eclout_");

  	//int runflag = 0;

  	//check run
	TH1F* rtype = (TH1F*)_file0->FindObjectAny("DQMInfo/rtype"); 
	if (!rtype) 
	{
		return; 
	}

	//get runtype 
	const char* runtype = rtype->GetTitle();

	//get date and time
  	TKey* key = (TKey*)_file0->FindKey("DQMInfo/rtype");
  	const char* datime = key->GetDatime().AsSQLString();

	//declare arrays
	//53 for easier numeration in variables (use 1 to 52 as indices)
	double entries[53]; //(Integral)
	double mean[53];
    double merror[53];
    double sigma[53];
    double serror[53];

    double chi2[53];

    TCanvas* canvas[53];
    //double bkg_overlay_frac[53];
    //double high_amp_WF_frac[53];


    int i = 1;
	//for each .root file I need 52 histograms (for all crates)
	for(i = 1; i < 53; i++)
	{

		std::string histname = "ECL/time_crate_" + std::to_string(i) + "_Thr1GeV";

		TH1F* hist = (TH1F*)_file0->FindObjectAny(histname.c_str());

		//if problems with one histogram, don't write all run 

		//check histogram: 
		if(!hist)
		{
			return;
		}

		entries[i] = hist->Integral();
		if(entries[i] < 50)
		{
			return;
		} 

		//fit
		TFitResultPtr fit = hist->Fit("gaus", "QS");
		Int_t fitStatus = fit; //The fitStatus is 0 if the fit is OK (i.e no error occurred). 
		if(fitStatus != 0)
		{
			return;
		}

		//extract variables from the histogram
		mean[i] = fit->Parameter(1);
        merror[i] = fit->ParError(1);
        sigma[i] = fit->Parameter(2);
        serror[i] = fit->ParError(2);
        chi2[i] = fit->Chi2();


        // bkg_overlay_frac[i] = adc_flag.GetBinContent(2) / ev_total
        //high_amp_WF_frac[i] = adc_flag.GetBinContent(3) / ev_total

        //критерий отбора по значениям для крейтов, хз куда его
        /*if(!(merror[i] < 0.25 && std::abs(mean[i])))
        {
        	delete hist;
        	delete fit; //???
        	continue;
        }*/

		//will be 52 pictures with fitted histograms
		//put theese canvases to new .root file
		std::string canvasname = "time_crate_" + std::to_string(i);
		canvas[i] = new TCanvas(canvasname.c_str(), canvasname.c_str());

		//draw on canvas
		hist->Draw();
		(canvas[i])->Draw();

		//write canvas to output .root file
		/*TFile* fout = new TFile(fname.c_str(),"UPDATE");
		(canvas[i])->Write();
		fout->Close();*/

		std::cout << "crate " << i << std::endl;
	}


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

  	xml.NewChild(info, 0, "run", (std::to_string(run)).c_str());
  	xml.NewChild(info, 0, "exp", (std::to_string(exp)).c_str());
  	xml.NewChild(info, 0, "datetime", datime);
  	xml.NewChild(info, 0, "runtype", runtype);

  	//write canvases to output .root file
	TFile* fout = new TFile(fname.c_str(),"UPDATE");
		
  	// add variables and canvases
  	for(i = 1; i < 53; i++)
  	{
  		(canvas[i])->Write();

  		std::string entries_n = "entries_" + std::to_string(i);
  		std::string mean_n = "mean_" + std::to_string(i);
  		std::string merror_n = "mean_error_" + std::to_string(i);
  		std::string sigma_n = "sigma_" + std::to_string(i);
  		std::string serror_n = "sigma_error_" + std::to_string(i);
  		std::string chi2_n = "chi2_" + std::to_string(i);

  		xml.NewChild(info, 0, entries_n.c_str(), (std::to_string(entries[i])).c_str());
  		xml.NewChild(info, 0, mean_n.c_str(), (std::to_string(mean[i])).c_str());
  		xml.NewChild(info, 0, merror_n.c_str(), (std::to_string(merror[i])).c_str());
  		xml.NewChild(info, 0, sigma_n.c_str(), (std::to_string(sigma[i])).c_str());
  		xml.NewChild(info, 0, serror_n.c_str(), (std::to_string(serror[i])).c_str());
  		xml.NewChild(info, 0, chi2_n.c_str(), (std::to_string(chi2[i])).c_str());

  	}

  	fout->Close();

  	if (nw == 1) xml.DocSetRootElement(xmldoc, mainnode);
  	xml.SaveDoc(xmldoc, filename);
  	xml.FreeDoc(xmldoc);

}