# ecl-dqm-coursework
A bunch of files used for data processing in my coursework "Data quality monitoring of data from electromagnetic calorimeter of the detector Belle II"

CERN ROOT (C++ and PyROOT) is used in this work. Task was to process about 5000 .root files with histograms, describing the quality of data from the electromagnetic calorimeter, and get some parameters such as average signal peak time, its deviation, ADC pedestal width, and others. 

Processing algorithm was implemented in Python scripts and extracted parameters were saved in ROOT TTree. After tests and finding the most informative parameters, data processing was implemented in C++ as ROOT-script, which will be used in the global Belle II DQM system to display charts for extracted parameters on the web-page.

