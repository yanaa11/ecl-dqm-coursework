#!/usr/bin/env python3

import ROOT
from glob import glob
import re
from array import array

writefile = ROOT.TFile('timefit.root', 'recreate')
tree = ROOT.TTree('tree', 'Tree')

#files_list = []
files = glob('/home/yana/2019.12.04_dqm_histograms/hltdqm*.root')
files.sort()
#files_list += files[1:100]
#print(files_list)

def getExpRun(fname):
    m = re.search('dqm_e([0-9]+)r([0-9]+)', fname)
    return int(m.group(1)), int(m.group(2))

exp = array('i', [0])
run = array('i', [0])
crate = array('i', [0])
mean = array('d', [0.0])
merror = array('d', [0.0])
sigma = array('d', [0.0])
serror = array('d', [0.0])

chi2 = array('d', [0.0])

bkg_overlay_frac = array('d', [0.0])
high_amp_WF_frac = array('d', [0.0])



exp_br = tree.Branch('exp', exp, 'exp/I')
run_br = tree.Branch('run', run, 'run/I')
crate_br = tree.Branch('crate', crate, 'crate/I')
mean_br = tree.Branch('mean', mean, 'mean/D')
merror_br = tree.Branch('mean_error', merror, 'mean_error/D')
sigma_br = tree.Branch('sigma', sigma, 'sigma/D')
serror_br = tree.Branch('sigma_error', serror, 'sigma_error/D')
bkg_br = tree.Branch('bkg_overlay_frac', bkg_overlay_frac, 'bkg_overlay_frac/D')
ha_br = tree.Branch('high_amp_WF_frac', high_amp_WF_frac, 'high_amp_WF_frac/D')

chi2_br = tree.Branch('chi2', chi2, 'chi2/D')

for i, fname in enumerate(files):

    exp[0], run[0] = getExpRun(fname)

    runfile = ROOT.TFile(fname)

    #print('      ', run)

    if runfile.FindObjectAny('DQMInfo/rtype') == None:
        continue

    for j in range(1, 53):

        crate[0] = j

        obj_name = f'ECL/time_crate_{j}_Thr1GeV'
        hist = runfile.FindObjectAny(obj_name)

        if hist == None:
            continue
        if hist.Integral() < 50:
            continue

        fit = hist.Fit("gaus", 'QSN')
        if fit == None:
            continue

        adc_flag = runfile.FindObjectAny('ECL/adc_flag')

        if adc_flag == None: 
            continue

        ev_total = int(adc_flag.GetBinContent(1))
        if ev_total == 0: 
            continue

        print(run[0], crate[0])

        mean[0] = fit.Parameter(1)
        merror[0] = fit.ParError(1)
        sigma[0] = fit.Parameter(2)
        serror[0] = fit.ParError(2)
        chi2[0] = fit.Chi2()
        bkg_overlay_frac[0] = adc_flag.GetBinContent(2) / ev_total
        high_amp_WF_frac[0] = adc_flag.GetBinContent(3) / ev_total

        tree.Fill()

writefile.Write()
writefile.Close()