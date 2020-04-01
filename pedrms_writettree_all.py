#!/usr/bin/env python3

import ROOT
from glob import glob
import re
from array import array
import ecl_map

eclmap = ecl_map.EclMap()


writefile = ROOT.TFile('pedrmstree2.root', 'recreate')
tree = ROOT.TTree('pedrmstree', 'pedrmstree')

files_list = []
files = glob('/home/yana/2019.12.04_dqm_histograms/hltdqm*.root')
files.sort()
files_list += files[1:100]
#print(files_list)

def getExpRun(fname):
    m = re.search('dqm_e([0-9]+)r([0-9]+)', fname)
    return int(m.group(1)), int(m.group(2))

exp = array('i', [0])
run = array('i', [0])

cellid = array('i', [0]) #Cell ID - номер бина в исходном TProfile 
theta = array('i', [0]) #Theta ID
phi = array('i', [0]) #Phi ID
entries = array('f', [0.0]) #entries in bin 

pedrms = array('d', [0.0]) #ширина пьедестала - среднее значение в бине
pedrms_stddev = array('d', [0.0]) #стандартное отклонение ширины пьедестала (в бине) (s по формулам в описании TProfile)

exp_br = tree.Branch('exp', exp, 'exp/I')
run_br = tree.Branch('run', run, 'run/I')

cid_br = tree.Branch('cell_ID', cellid, 'cell_ID/I')
theta_br = tree.Branch('theta_ID', theta, 'theta_ID/I')
phi_br = tree.Branch('phi_ID', phi, 'phi_ID/I')
entries_br = tree.Branch('entries', entries, 'entries/F')

pedrms_br = tree.Branch('pedrms', pedrms, 'pedrms/D')
stddev_br = tree.Branch('pedrms_stddev', pedrms_stddev, 'pedrms_stddev/D')



for i, fname in enumerate(files):

    exp[0], run[0] = getExpRun(fname)

    runfile = ROOT.TFile(fname)

    if runfile.FindObjectAny('DQMInfo/rtype') == None:
    	continue

    pedrms_profile = runfile.FindObjectAny('ECL/pedrms_cellid')

    if pedrms_profile == None: 
    	continue

    #adc_flag = runfile.FindObjectAny('ECL/adc_flag')
	#if adc_flag == None: continue

    #ev_total = int(adc_flag.GetBinContent(1))
    #if ev_total == 0: continue

    #1 - 8736
    for cid in range(1, pedrms_profile.GetNbinsX()+1):  

    	cellid[0] = cid

    	theta[0] = eclmap.getThetaId(cid)
    	phi[0] = eclmap.getPhiId(cid)

    	entries[0] = pedrms_profile.GetBinEntries(cid)
    	pedrms[0] = pedrms_profile.GetBinContent(cid)

    	#use standart deviation instead of 'standart error on the mean'
    	pedrms_profile.SetErrorOption('s') 
    	pedrms_stddev[0] = pedrms_profile.GetBinError(cid)

    	tree.Fill()

writefile.Write()
writefile.Close()


    	#print(f"{exp}     {run}     {j}     {fit.Parameter(1):1.5f}     {fit.ParError(1):1.5f}     {fit.Parameter(2):1.5f}     {fit.ParError(2):1.5f}")
    	#print(i, '   ', fit.Parameter(1),'   ', fit.ParError(1), '   ', fit.Parameter(2), '   ', fit.ParError(2))
