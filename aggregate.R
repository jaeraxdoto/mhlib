#!/usr/bin/Rscript

# Calculate grouped basic statistics for detailed csv results,
# which are either given by stdin or in a file provided as parameter
# If two csv files are given as parameter, they are assumed to be results
# from two different algorithms on the same instances, and they are compared
# including a Wilcoxon rank sum test
#
# Consider this R script more as an example or template. It is adopted
# from the BBSS project and contains some quite specific functions for 
# this particular project which are not meaningful for other
# projects.


options(width=10000) # default line width for print, i.e., do not break lines

#--------------------------------------------------------------------------------
# General helper functions

geometric_mean <- function(x,shift=0) {
  exp(mean(log(x+shift)))-shift
}

# read raw CSV-file, i.e. summarized out-files
readraw <- function(f) {
  read.table(f,header=TRUE,stringsAsFactors=FALSE)
}

# print a results table in a user friendly format
printagg <- function (a) {
  print(format(a,digits=8,scientific=FALSE))
}

# print results table in more precise machine readable format
printagg_exact <- function (a) {
  write.table(a,quote=FALSE) 
}

# determine the category name to aggregate over from the given file name
categ <- function(x) sub("^(.*B_)(.*)_(.*)_(.*)_(.*)_([^-]*)(-.*)?.out","\\1\\2_\\5_\\6",x,perl=TRUE)
categ2 <- function(x) sub("^.*(B_.*)_(.*)_(.*)_(.*)_([^-]*)(-.*)?.out","\\1_\\2_\\4_\\5",x,perl=TRUE)
# determine basename of file name
categbase <- function(x) sub("^.*B_([^-]*)(-00)?(-.*)?\\.out","\\1\\3",x,perl=TRUE)


#-------------------------------------------------------------------------
# Aggregation of one CSV-file obtained from heu_sum.pl

# determine aggregated results for one raw table
aggregate <- function(rawdata,categfactor=factor(sapply(as.vector(rawdata$file),categ))) {
  # create factor, i.e., grouping
  # categfactor <- factor(sapply(rawdata$file,categ))
  # calculate all aggregated data
  aggregated <- data.frame(
    # file=levels(categfactor),
    runs=tapply(rawdata$obj,categfactor,length),
    obj_mean=tapply(rawdata$obj,categfactor,mean),
    obj_sd=tapply(rawdata$obj,categfactor,sd),
    ittot_med=tapply(rawdata$ittot,categfactor,median),
    itbest_med=tapply(rawdata$itbest,categfactor,median),
    ttot_med=tapply(rawdata$ttot,categfactor,median),
    tbest_med=tapply(rawdata$tbest,categfactor,median)
    # tbest_sd=tapply(rawdata$tbest,categfactor,sd),
    # bal_mean=tapply(floor(rawdata$obj),categfactor,mean),
    # bal_sd=tapply(floor(rawdata$obj),categfactor,sd),
    # rest_mean=tapply(rawdata$obj-floor(rawdata$obj),categfactor,mean),
    # rest_sd=tapply(rawdata$obj-floor(rawdata$obj),categfactor,sd),
    )
  if ('obj0' %in% colnames(rawdata)) {
  	aggregated$obj0_mean <- tapply(rawdata$obj0,categfactor,mean)
  	aggregated$obj1_mean <- tapply(rawdata$obj1,categfactor,mean)
  }
  aggregated
}

aggregatemip <- function(rawdata,categfactor=factor(sapply(as.vector(rawdata$File),categ))) {
  # create factor, i.e., grouping
  # categfactor <- factor(sapply(rawdata$file,categ))
  # calculate all aggregated data
  aggregated <- data.frame(
    # file=levels(categfactor),
    runs=tapply(rawdata$Upper_bound,categfactor,length),
    ub_mean=tapply(rawdata$Upper_bound,categfactor,mean),
    ub_sd=tapply(rawdata$Upper_bound,categfactor,sd),
    lb_mean=tapply(rawdata$Lower_bound,categfactor,mean),
    lb_sd=tapply(rawdata$Lower_bound,categfactor,sd),
    ttot_med=tapply(rawdata$Time,categfactor,median)
    )
#  if ('obj0' %in% colnames(rawdata)) {
#  	aggregated$obj0_mean <- tapply(rawdata$obj0,categfactor,mean)
#  	aggregated$obj1_mean <- tapply(rawdata$obj1,categfactor,mean)
#  }
  aggregated
}

# calculate total values over aggregate data
totalagg <- function(agg) {
  total <- data.frame(
    row.names="total",
    runs=sum(agg$runs),
    obj_mean=mean(agg$obj_mean),
    ittot_mean=mean(agg$ittot_med),
    itbest_mean=mean(agg$itbest_med),
    ttot_mean=mean(agg$ttot_med),
    tbest_mean=mean(agg$tbest_med),
    obj_gmean=geometric_mean(agg$obj_mean),
    ittot_gmean=geometric_mean(agg$ittot_med),
    itbest_gmean=geometric_mean(agg$itbest_med),
    ttot_gmean=geometric_mean(agg$ttot_med,0.01),
    tbest_gmean=geometric_mean(agg$tbest_med,0.01)
    )
  if ('obj0_mean' %in% colnames(agg)) {
    total$obj0_mean <- mean(agg$obj0_mean)  
    total$obj1_mean <- mean(agg$obj1_mean)  
    total$obj0_gmean <- geometric_mean(agg$obj0_mean)  
    total$obj1_gmean <- geometric_mean(agg$obj1_mean)  
  }
  total
}

# reasonably round aggregated results
roundagg <- function(a) {
  a$obj_mean <- round(a$obj_mean,6)
  a$obj_sd <- round(a$obj_sd,6)
  a$ittot_med <- round(a$ittot_med,1)
  a$itbest_med <- round(a$itbest_med,1)
  a$ttot_med <- round(a$ttot_med,1)
  a$tbest_med <- round(a$tbest_med,1)
  if ('obj0_mean' %in% colnames(a)) {
    a$obj0_mean <- round(a$obj0_mean,6)
    a$obj1_mean <- round(a$obj1_mean,6)
  }
  a
}

# reasonably round aggregated results
roundaggmip <- function(a) {
  a$ub_mean <- round(a$ub_mean,6)
  a$ub_sd <- round(a$ub_sd,6)
  a$lb_mean <- round(a$lb_mean,6)
  a$lb_sd <- round(a$lb_sd,6)
  a$ttot_med <- round(a$ttot_med,1)
  a
}

# perform aggregation and print results for one raw data
agg_print <- function(rawdata) {
  aggregated <- aggregate(rawdata)
  aggtotal <- totalagg(aggregated)
  printagg(roundagg(aggregated))
  cat("\n")
  printagg(aggtotal)
}

#-------------------------------------------------------------------------
# Aggregation and comparison of two CSV-files obtained from heu_sum.pl

#library(BSDA)

# perform Wilcoxon rank sum test on col1[x] and col2[x]
wtest <- function (x,col1,col2) {
  round(suppressWarnings(wilcox.test(col1[x],col2[x],alternative="less",paired=TRUE))$p.value,6)
  # binom.test(sum(col1[x]<col2[x]),sum(col1[x]!=col2[x]),alternative="less") # sign test
  # wilcox.test(col1,col2,subset=x,alternative="less",paired=TRUE)$p.value
}

# aggregate results of two merged inputs
doaggregate2 <- function(raw,fact) {
  data.frame(
    # row.names=levels(fact),
    runs=tapply(raw$obj.x,fact,length),
    A_obj_mean=tapply(raw$obj.x,fact,mean),
    B_obj_mean=tapply(raw$obj.y,fact,mean),
    diffobj_mean=tapply(raw$obj.x-raw$obj.y,fact,mean),
    AlessB=tapply(raw$obj.x<raw$obj.y,fact,sum),
    BlessA=tapply(raw$obj.y<raw$obj.x,fact,sum),
    AeqB=tapply(raw$obj.y==raw$obj.x,fact,sum),
    p_AlessB=tapply(1:length(raw$class),fact,wtest,col1=raw$obj.x,col2=raw$obj.y), 
    p_BlessA=tapply(1:length(raw$class),fact,wtest,col1=raw$obj.y,col2=raw$obj.x) 
    # obj_sd=tapply(rawdata$obj,fact,sd),
    # ittot_med=tapply(rawdata$ittot,fact,median),
    # itbest_med=tapply(rawdata$itbest,fact,median),
    # ttot_med=tapply(rawdata$ttot,fact,median),
    # tbest_med=tapply(rawdata$tbest,fact,median)
    # tbest_sd=tapply(rawdata$tbest,fact,sd),
    # bal_mean=tapply(floor(rawdata$obj),fact,mean),
    # bal_sd=tapply(floor(rawdata$obj),fact,sd),
    # rest_mean=tapply(rawdata$obj-floor(rawdata$obj),fact,mean),
    # rest_sd=tapply(rawdata$obj-floor(rawdata$obj),fact,sd),
  )
}

# determine aggregated results for two inputs including comparison of results
aggregate2 <- function(rawdata1,rawdata2) {
  rawdata1$base <- categbase(rawdata1$file)
  rawdata2$base <- categbase(rawdata2$file)
  raw <- merge(rawdata1,rawdata2,by.x="base",by.y="base")
  raw$class <- categ2(raw$file.x)
  #for (l in levels(categfactor1,categfactor2)))) {
  #  print(l)
  #  r1<-rawdata1[rawdata1$class==l,]
  #  r2<-rawdata2[rawdata2$class==l,]
  #  r<-merge(r1,r2,by.x="base",by.y="base")
  #  print(r)
  #}
  aggregated<-doaggregate2(raw,factor(raw$class))
  aggtotal<-doaggregate2(raw,rep("total",times=length(raw$class)))
  list(grouped=aggregated,total=aggtotal)
}

roundagg2 <- function(a) {
  a$A_obj_mean <- round(a$A_obj_mean,6)
  a$B_obj_mean <- round(a$B_obj_mean,6)
  a$diffobj_mean <- round(a$diffobj_mean,6)
  a
}

printsigdiffs <- function(agg2) {
  Awinner<-sum(agg2$AlessB>agg2$BlessA)
  Bwinner<-sum(agg2$AlessB<agg2$BlessA)
  gr<-length(agg2$AlessB)
  cat("A is yielding more frequently better results on ", Awinner,
	" groups (",round(Awinner/gr*100,2),"%)\n") 
  cat("B is yielding more frequently better results on ", Bwinner, 
	" groups (",round(Bwinner/gr*100,2),"%)\n") 
  cat("\nSignificant differences:\n\np_AlessB<=0.05\n")
  printagg(roundagg2(subset(agg2,agg2$p_AlessB<=0.05)))
  cat("\np_BlessA<=0.05\n")         
  printagg(roundagg2(subset(agg2,agg2$p_BlessA<=0.05)))
}                    

# perform aggregation and print comparative results for two raw data
agg2_print <- function(rawdata1,rawdata2) {
  aggregated <- aggregate2(rawdata1,rawdata2)
  printagg(roundagg2(rbind(aggregated$grouped,aggregated$total)))
  cat("\n")
  printsigdiffs(rbind(aggregated$grouped,aggregated$total))
}

#-------------------------------------------------------------------------
# Evaluation of alpha-tests for GRASP   (used for EUROCAST)

# determine Vnum from file name
getVnum <- function(x) as.numeric(sub("^.*B_(\\d*)_.*$","\\1",x,perl=TRUE))
# determine alpha from directory name
getalpha <- function(x) as.numeric(sub("^.*/alpha([-\\.0-9]*)/B.*$","\\1",x,perl=TRUE))
# determine basename of category ignoring directories
categbasealphacateg <- function(x) sub("^.*(B_.*)$","\\1",x,perl=TRUE)
categbasealpha <- function(x) sub("^.*(B_.*)_(.*)_(.*)_(.*)_([^-]*)(-.*)?.out","\\1_\\4_\\5",x,perl=TRUE)

boxplotalpha <- function(x,df) {
  boxplot(df$obj[x] ~ df$alpha[x],xlab="alpha",ylab="obj",main=df$class[x][1])
}

evalalpha <- function() {
  # system("./heu_sum.pl -f petrina/grasp/test_alpha*/*/*out > grasp-alpha.csv")
  grasp.alpha <- readraw("grasp-alpha.csv")
  grasp.alpha$alpha <- sapply(grasp.alpha$file,getalpha)
  grasp.alpha$class <- sapply(grasp.alpha$file,categbasealpha)
  negind <- grasp.alpha$alpha<0
  grasp.alpha$class[negind] <- paste(grasp.alpha$class[negind],rep.int("random",sum(negind)),sep="-")
  grasp.alpha$alpha <- abs(grasp.alpha$alpha)
  grasp.alpha$class <-factor(grasp.alpha$class)
  par(mfrow=c(ceiling(length(levels(grasp.alpha$class))/4),4))
  #par(mar = c(4, 3, 3, 2))
  tapply(1:length(grasp.alpha$obj),grasp.alpha$class,boxplotalpha,grasp.alpha)
  
  # aggregated evaluation
  aggregated <- aggregate(grasp.alpha)
  aggregated$alpha <- sapply(row.names(aggregated),getalpha)
  aggregated$class <- sapply(row.names(aggregated),categbasealphacateg)
  aggregated$class <- factor(aggregated$class)
  # attach(aggregated)
  # aggfactor <- factor(sapply(row.names(aggregated),getclass))
  # printagg(aggfactor)
  # coplot(obj_mean ~ alpha | factor(class))
  # detach(aggregated)
  sortedagg <- aggregated[order(aggregated$class),]
  printagg(roundagg(sortedagg))
  sortedagg
}

#-------------------------------------------------------------------------
# Comparison of more than one inputs   (used in HM paper)

# Return for a vector a vector indicating the minimal value(s) by 1
countbest <- function(x) {
  (x==min(x))*1
}

# Comparison of more than two raw data
rankallraw <- function(allobj) {
  # t(apply(allobj,1,rank))
  t(apply(allobj,1,countbest))
}

doaggregateall <- function(allobj,fact) {
  ar <- rankallraw(allobj)
  df <- data.frame(
    # row.names=levels(fact),
    runs=tapply(rownames(allobj),fact,length)
    #A_obj_mean=tapply(raw$obj.x,fact,mean),
    #B_obj_mean=tapply(raw$obj.y,fact,mean),
    #diffobj_mean=tapply(raw$obj.x-raw$obj.y,fact,mean),
  )
  for (i in 1:dim(allobj)[2]) {
    rn<-paste("obj",i,"best",sep="")
    df[[rn]] <- tapply(ar[,i],fact,sum)
  }
  df
}

# aggregateall <- function(allrawlist) {
#   allraw<-data.frame(rownames=rownames(allrawlist[[1]]))
#   # sapply(allrawlist
#   allraw<-allrawlist
#   doaggregateall(allraw,categ2(rownames(allraw)))
# }

hmtablevar <- function(raw,idx) {
  agg<-roundagg(aggregate(raw))
  data.frame(row.names=rownames(agg),
    best=hmbestcount[,idx],
    obj_mean=agg$obj_mean,
    obj_sd=agg$obj_sd,
    tbest_med=agg$tbest_med)
}

hmtable <- function() {
  # nofindbest<-readraw("nofindbest.csv")
  # hmoptl1<-readraw("hmoptl1.csv")
  # etc.
  allobj<-data.frame(row.names=nofindbest$file,
                     objno=nofindbest$obj,
                     obj1=hmoptl1$obj,
                     obj2=hmoptl2$obj,
                     obj3=hmoptl3$obj,
                     obj4=hmcomb4$obj)
  # hmbestcount<-rankallraw(allraw)
  hmbestcount<-doaggregateall(allobj,categ2(rownames(allobj)))
  cbind(
             hmtablevar(nofindbest,2),
             hmtablevar(hmoptl1,3),
             hmtablevar(hmoptl2,4),
             hmtablevar(hmoptl3,5),
             hmtablevar(hmcomb4,6)
  )
}

# For JOGO paper
jogotablevarbestobj <- function(raw,idx) {
  agg<-roundagg(aggregate(raw))
  data.frame(row.names=rownames(agg),
    best=hmbestcount[,idx],
    obj_mean=agg$obj_mean,
    obj_sd=agg$obj_sd)
#    ttot_med=agg$ttot_med)
}
jogotablevarbestobjmip <- function(raw,idx) {
  agg<-roundaggmip(aggregatemip(raw))
  data.frame(row.names=rownames(agg),
    best=hmbestcount[,idx],
    ub_mean=agg$ub_mean,
    ub_sd=agg$ub_sd,
    lb_mean=agg$lb_mean,
    lb_sd=agg$lb_sd)
#    ttot_med=agg$ttot_med)
}
jogotablevartimeiter <- function(raw,idx) {
  agg<-roundagg(aggregate(raw))
  data.frame(row.names=rownames(agg),
    ttot_med=agg$ttot_med,
    ittot_med=agg$ittot_med)
    #best=hmbestcount[,idx],
    #obj_mean=agg$obj_mean,
    #obj_sd=agg$obj_sd,
    #ttot_med=agg$ttot_med)
}
jogotablevartimeitermip <- function(raw,idx) {
  agg<-roundaggmip(aggregatemip(raw))
  data.frame(row.names=rownames(agg),
    ttot_med=agg$ttot_med)
    #best=hmbestcount[,idx],
    #obj_mean=agg$obj_mean,
    #obj_sd=agg$obj_sd,
    #ttot_med=agg$ttot_med)
}


jogotablebestobj <- function() {
  # nofindbest<-readraw("nofindbest.csv")
  # hmoptl1<-readraw("hmoptl1.csv")
  # usw.
  allobj<-data.frame(row.names=jogooptl0$file,
                     obj0=jogomip$Upper_bound,
                     obj1=jogooptl0$obj,
                     obj2=jogooptl1$obj,
                     obj3=jogooptl2$obj,
                     obj4=jogooptl3$obj)
  # hmbestcount<-rankallraw(allraw)
  hmbestcount<-doaggregateall(allobj,categ2(rownames(allobj)))
  cbind(
             jogotablevarbestobjmip(jogomip,2),
             jogotablevarbestobj(jogooptl0,3),
             jogotablevarbestobj(jogooptl1,4),
             jogotablevarbestobj(jogooptl2,5),
             jogotablevarbestobj(jogooptl3,6)
  )
}
jogotabletimeiter <- function() {
  # nofindbest<-readraw("nofindbest.csv")
  # hmoptl1<-readraw("hmoptl1.csv")
  # usw.
  allobj<-data.frame(row.names=jogooptl0$file,
                     obj0=jogomip$Upper_bound,
                     obj1=jogooptl0$obj,
                     obj2=jogooptl1$obj,
                     obj3=jogooptl2$obj,
                     obj4=jogooptl3$obj)
  # hmbestcount<-rankallraw(allraw)
  hmbestcount<-doaggregateall(allobj,categ2(rownames(allobj)))
  cbind(
             jogotablevartimeitermip(jogomip,2),
             jogotablevartimeiter(jogooptl0,2),
             jogotablevartimeiter(jogooptl1,3),
             jogotablevartimeiter(jogooptl2,4),
             jogotablevartimeiter(jogooptl3,5)
  )
}


#-------------------------------------------------------------------------
# main part

# if called as script read csv-file or stdin, aggregate, and print
if (!interactive()) {
  cmd_args = commandArgs(trailingOnly=TRUE);
  if (length(cmd_args)<=1) {
    # process one CSV-file
    f <- if (length(cmd_args)>0) cmd_args[1] else file("stdin") 
    rawdata <- readraw(f)
	agg_print(rawdata)
    } else {
    # process and compare two CSV-files
    rawdata1 <- readraw(cmd_args[1])
    rawdata2 <- readraw(cmd_args[2])
	agg2_print(rawdata1,rawdata2) 
  }
}

