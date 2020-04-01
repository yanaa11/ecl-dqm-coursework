#!/usr/bin/env python3

import ROOT

file = ROOT.TFile('outall.root', 'read')

tree = file.FindObjectAny('tree')

#print(type())

for ent in tree:
	print(ent.run, ent.crate, ent.mean, ent.merror, ent.bkg_overlay_frac, ent.high_amp_WF_frac)

file.Close()
