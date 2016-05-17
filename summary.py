#!/usr/bin/python3
# Summarizes the essential information from several heuristic algorithm runs
# into a CSV-table written to stdout.
# The result files to be processed are given as arguments (out-files or 
# directories containing out-files)
# The CSV-table contains:
# - file: name of processed out-filename
# - obj: objective value of final solution
# - ittot: total number of iterations
# - itbest: number of iteration at which final solution was found
# - ttot: total CPU-time
# - tbest: CPUT-time when final solution was found
#
# if option "-f" is given, the corresponding log-Files are additionally
# scanned and the objective values for iteration 0 and 1 (obj0, obj1) are also
# printed; in case of the VNS, these are the values for the greedy
# initial solution and the solution after first performing the VND. 

from __future__ import print_function
import sys, getopt, os, glob, re

paramf = False	# print also result of initial solution and first iteration; set by option -f


# process list of files/directories
def processlist(args):
	for file in args:
		if os.path.isdir(file):
			processlist(glob.glob(file+"/*.out"))
		else:
			# process out-file
			resfound = False	# becomes True when all data found
			# print(file)
			with open(file) as f:
				for line in f:
					m = re.match(r'best objective value:\s(\d+\.?\d*)',line)
					if m: obj = m.group(1)
					m = re.match(r'best obtained in iteration:\s(\d+\.?\d*)',line)
					if m: itbest = m.group(1)
					m = re.match(r'solution time for best:\s(\d+\.?\d*)',line)
					if m: tbest = m.group(1)
					m = re.match(r'CPU time:\s(\d+\.?\d*)',line)
					if m: ttot = m.group(1)
					m = re.match(r'iterations:\s(\d+\.?\d*)',line)
					if m: ittot = m.group(1); resfound = True
			if resfound:
				if not paramf:
					print(file,obj,ittot,itbest,ttot,tbest,sep="\t")
				else:
					# also process corresponding log files, extracting obj0 and obj1
					lfile=re.sub("(.out)$",".log",file)
					with open(lfile) as f:
						obj0=obj1="-"
						for line in f:
							m = re.match(r'^0+\s(\d+.?\d*)',line)
							if m: obj0 = obj1 = m.group(1)
							m = re.match(r'^0+1\s(\d+.?\d*)',line)
							if m: obj1 = m.group(1)
					print(file,obj,ittot,itbest,ttot,tbest,obj0,obj1,sep="\t")


try:
	opts, args = getopt.getopt(sys.argv[1:],"hf")
except getopt.GetoptError:
	print('summary.py -f <out-files>')
	sys.exit(2)
for opt, arg in opts:
	if opt == '-h':
		print('summary.py -f <out-files>')
		sys.exit()
	elif opt == "-f":
		paramf = True
if not paramf:
	print("file\tobj\tittot\titbest\tttot\ttbest")
else:
	print("file\tobj\tittot\titbest\tttot\ttbest\tobj0\tobj1")
processlist(args)
