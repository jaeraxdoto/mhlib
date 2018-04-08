#!/usr/bin/python3

# Calculate grouped basic statistics for detailed csv results,
# which are either given by stdin or in a file provided as parameter
# If two csv files are given as parameter, they are assumed to be results
# from two different algorithms on the same instances, and they are compared
# including a Wilcoxon rank sum test
#
# Important: For the aggregation to work correctly, adapt in particular
# below definitions of categ, categ2 and categbase according to your
# conventions for the filenames encoding instance and run information.
#
# Consider this script more as an example or template. 

import pandas as pd
# import numpy as np
import scipy.stats
import sys
import re
import warnings


#--------------------------------------------------------------------------------
# determine the category name to aggregate over from the given file name

# for aggregating a single table of raw data: 
# return category name for a given filen name
def categ(x):
    # re.sub(r"^(.*)/(T.*)-(.*)_(.*).res",r"\1/\2-\3",x)
    return re.sub(r".*[lcs|lcps]_(\d+)_(\d+)_(\d+)\.(\d+)(\.out)",
           r"\1/\2-\3",x)

# for aggregating two tables corresponding to two different
# configurations that shall be compared:
# return category name for a given filen name
def categ2(x):
    # re.sub(r"^(.*)/(T.*)-(.*)_(.*).res",r"\2-\3")
    return re.sub(r"^.*[lcs|lcps]_(\d+)_(\d+)_(\d+)\.(\d+)(\.out)",
           r"\1/\2-\3",x)

# for aggregating two tables corresponding to two different configurations that shall be compared
# return detailed name of run (basename) that should match a corresponding one
# of the other configuration
def categbase(x):
    #re.sub(r"^.*/(T.*)-(.*)_(.*).res",r"\1-\2-\3",x)
    return re.sub(r"^.*[lcs|lcps]_(\d+)_(\d+)_(\d+)\.(\d+)(\.out)",
           r"\1_\2_\3.\4\5",x)
        
pd.options.display.width = 10000
pd.options.display.max_rows = None
pd.options.display.precision = 8
#pd.set_eng_float_format(accuracy=8)

#--------------------------------------------------------------------------------
# General helper functions

# read raw CSV-file, i.e. summarized out-files
def readraw(f):
    return pd.read_csv(f,sep="\t")


# print results table in more precise machine readable format
def printagg_exact(a):
    a.to_csv(sep="\t")


#-------------------------------------------------------------------------
# Aggregation of one CSV-file obtained from summary.pl

# determine aggregated results for one raw table
def aggregate(rawdata,categfactor=categ):
    rawdata["cat"]=rawdata.apply(lambda row: categfactor(row["file"]),axis=1)
    rawdata["gap"]=rawdata.apply(lambda row: (row["ub"]-row["obj"])/row["ub"],axis=1)
    grp = rawdata.groupby("cat")
    # =factor(sapply(as.vector(rawdata$file),categ))
    aggregated = pd.DataFrame({"runs":grp["obj"].size(),
                               "obj_mean":grp["obj"].mean(),
                               "obj_sd":grp["obj"].std(),
                               "ittot_med":grp["ittot"].median(),
                               "ttot_med":grp["ttot"].median(),
                               "ub_mean":grp["ub"].mean(),
                               "gap_mean":grp["gap"].mean(),
                               "tbest_med":grp["tbest"].median(),
                               # "tbest_sd":grp["tbest"].std(),
                               })
    return aggregated[["runs","obj_mean","obj_sd","ittot_med","ttot_med",
                       "ub_mean","gap_mean","tbest_med"]]
    
def aggregatemip(rawdata,categfactor=0):
    #=factor(sapply(as.vector(rawdata$File),categ))
    rawdata["cat"]=rawdata.apply(lambda row: categfactor(row["file"]),axis=1)
    rawdata["gap"]=rawdata.apply(lambda row: (row["Upper_bound"]-
           row["Lower_bound"])/row["Upper_bound"],axis=1)
    grp = rawdata.groupby("cat")
    # =factor(sapply(as.vector(rawdata$file),categ))
    aggregated = pd.DataFrame({"runs":grp["obj"].size(),
                               "ub_mean":grp["Upper_bound"].mean(),
                               "ub_sd":grp["Upper_bound"].std(),
                               "lb_mean":grp["Lower_bound"].mean(),
                               "lb_sd":grp["Lower_bound"].std(),
                               "ttot_med":grp["ttot"].median(),
                               "gap_mean":grp["gap"].mean(),
                               # "tbest_med":grp["tbest"].med(),
                               # "tbest_sd":grp["tbest"].std(),
                               })
    return aggregated[["runs","ub_mean","ub_sd","lb_mean","ttot_med","gap_mean"]]

# calculate total values over aggregate data
def totalagg(agg):
    pass

# reasonably round aggregated results
def roundagg(a):
    return a.round({'obj_mean':6, 'obj_sd':6, 'ittot_med':1, 'itbest_med':1,
        'ttot_med':1, 'tbest_med':1, 'obj0_mean':6, 'obj1_mean':6})

# reasonably round aggregated results
def roundaggmip(a):
    return a.round({'ub_mean':6, 'ub_sd':6, 'lb_mean':6, 'lb_sd':6, 'ttot_med':1,
                    'gap_mean':1})

# perform aggregation and print results for one raw data
def agg_print(rawdata):
    aggregated = aggregate(rawdata)
    aggtotal = totalagg(aggregated)
    printagg(roundagg(aggregated))
    print("")
    printagg(aggtotal)


#-------------------------------------------------------------------------
# Aggregation and comparison of two CSV-files obtained from summary.pl


# perform statistical test (Wilcoxon signed ranktest) on col1[x] and col2[x]
def stattest(col1,col2):
    dif = col1-col2
    noties = len(dif[dif!=0])
    lessass = dif.sum()<0
    if noties<1:
        return float(1)
    # if (col1==col2).all():
    #     return 3
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        msr,p = scipy.stats.wilcoxon(col1,col2,correction=True,zero_method="wilcox")
    #s,p = scipy.stats.mannwhitneyu(col1,col2,alternative="less")
    p = p/2
    if not lessass:
        p = 1-p
    return p

# aggregate results of two merged inputs
def doaggregate2(raw,fact):
    raw["obj_diff"]=raw.apply(lambda row: row["obj_x"]-row["obj_y"],axis=1)
    raw["AlessB"]=raw.apply(lambda row: row["obj_x"]<row["obj_y"],axis=1)
    raw["BlessA"]=raw.apply(lambda row: row["obj_x"]>row["obj_y"],axis=1)
    raw["AeqB"]=raw.apply(lambda row: row["obj_x"]==row["obj_y"],axis=1)
    # rawdata["gap"]=raw.apply(lambda row: (row["ub"]-row["obj"])/row["ub"],axis=1)
    grp = raw.groupby(fact)
    p_AlessB={}
    p_BlessA={}
    for g,d in grp:
        p_AlessB[g] = stattest(d["obj_x"],d["obj_y"])
        p_BlessA[g] = stattest(d["obj_y"],d["obj_x"])
    # =factor(sapply(as.vector(rawdata$file),categ))
    aggregated = pd.DataFrame({"runs":grp["obj_x"].size(),
                               "A_obj_mean":grp["obj_x"].mean(),
                               "B_obj_mean":grp["obj_y"].mean(),
                               "diffobj_mean":grp["obj_diff"].mean(),
                               "AlessB":grp["AlessB"].sum(),
                               "BlessA":grp["BlessA"].sum(),
                               "AeqB":grp["AeqB"].sum(),
                               "p_AlessB":p_AlessB,
                               "p_BlessA":p_BlessA,
                            })
    return aggregated[["runs","A_obj_mean","B_obj_mean","AlessB","BlessA",
                      "AeqB","p_AlessB","p_BlessA"]]
    
# determine aggregated results for two inputs including comparison of results
def aggregate2(rawdata1,rawdata2):
    rawdata1["base"]=rawdata1.apply(lambda row: categbase(row["file"]),axis=1)
    rawdata2["base"]=rawdata2.apply(lambda row: categbase(row["file"]),axis=1)
    raw = pd.merge(rawdata1,rawdata2,on="base",how="outer")
    raw["class"]=raw.apply(lambda row: categ2(row["file_x"]),axis=1)
    aggregated = doaggregate2(raw,"class")
    raw["total"]=raw.apply(lambda row: "total",axis=1)
    aggtotal = doaggregate2(raw,"total")
    return {"grouped":aggregated,"total":aggtotal}

def roundagg2(a):
    a["AlessB"] = a["AlessB"].map(lambda x: int(x))
    a["BlessA"] = a["BlessA"].map(lambda x: int(x))
    a["AeqB"] = a["AeqB"].map(lambda x: int(x))
    a = a.round({"A_obj_mean":6, "B_obj_mean":6, "diffobj_mean":6, 
                    "AlessB":0, "BlessA":0, "AeqB":0, "p_AlessB":4, "p_BlessA":4})
    return a
    
def printsigdiffs(agg2):
    Awinner = sum(agg2["AlessB"]>agg2["BlessA"])
    Bwinner = sum(agg2["AlessB"]<agg2["BlessA"])
    gr = agg2["AlessB"].size
    print("A is yielding more frequently better results on ", Awinner,
          " groups (",round(Awinner/gr*100,2),"%)") 
    print("B is yielding more frequently better results on ", Bwinner, 
          " groups (",round(Bwinner/gr*100,2),"%)") 
    print("\nSignificant differences:")
    sigAlessB = agg2[agg2.p_AlessB<=0.05] 
    sigBlessA = agg2[agg2.p_BlessA<=0.05] 
    if not sigAlessB.empty:
        print("\np_AlessB<=0.05\n",sigAlessB)
    if not sigBlessA.empty:
        print("\np_BlessA<=0.05\n",sigBlessA)         

# perform aggregation and print comparative results for two raw data
def agg2_print(rawdata1,rawdata2):
    aggregated = aggregate2(rawdata1,rawdata2)
    print(roundagg2(pd.concat([aggregated["grouped"],aggregated["total"]])))
    #print(roundagg2(aggregated["total"]))
    print("")
    printsigdiffs(roundagg2(pd.concat([aggregated["grouped"],aggregated["total"]])))
    
#-------------------------------------------------------------------------
# main part

# if called as script read csv-file or stdin, aggregate, and print
if __name__ == "__main__":
    if len(sys.argv) <= 2:
        # process one CSV-file
        f = sys.argv[1] if len(sys.argv) > 1 else sys.stdin 
        rawdata = readraw(f)
        agg_print(rawdata)
    else:
        # process and compare two CSV-files
        rawdata1 = readraw(sys.argv[1])
        rawdata2 = readraw(sys.argv[2])
        agg2_print(rawdata1,rawdata2) 

