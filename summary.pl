#!/usr/bin/perl
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


# process list of files/directories
sub processlist
{
	@ARGL=@_;
	foreach my $file (@ARGL)
	{
		if (-d "$file")
		{
			my $pattern="$file/*.out";
			my @dirlist=glob $pattern;
			processlist(@dirlist);
		}
		else
		{
			# process out-file
			my $resfound=0;	# true if file is complete
			open(FILE, $file) or die "Cannot open $file";
			while (<FILE>) 
			{
				if ($_=~/^best objective value:\s(\d+.?\d*)$/) 
					{ $obj=$1 }
				if ($_=~/^best obtained in iteration:\s(\d+.?\d*)$/) 
					{ $itbest=$1 }
				if ($_=~/^solution time for best:\s(\d+.?\d*)$/) 
					{ $tbest=$1; }
				if ($_=~/^CPU time:\s(\d+.?\d*)/) 
					{ $ttot=$1; }
				if ($_=~/^iterations:\s(\d+.?\d*)$/) 
					{ $ittot=$1; $resfound=1; }
				# for compatibility to old version
				if ($_=~/^best obtained in generation:\s(\d+.?\d*)$/) 
					{ $itbest=$1 }
				if ($_=~/^generations:\s(\d+.?\d*)$/) 
					{ $ittot=$1; $resfound=1; }
			}
			close(FILE);
			if ($resfound)
			{
				if ($paramf)
				{
					# also process corresponding log-file
					# to extract obj0 and obj1
					my $logfile=$file;
					$logfile=~s/.out$/.log/; 
					open(FILE, $logfile) or die "Cannot open $logfile";
					while (<FILE>)
					{
						if ($_=~/^0+\s(\d+.?\d*)$/) 
							{ $obj0=$1; $obj1=$obj0 }
						if ($_=~/^0+1\s(\d+.?\d*)$/) 
							{ $obj1=$1 }
					} 
					close(FILE);
					print("$file\t$obj\t$ittot\t$itbest\t$ttot\t$tbest\t$obj0\t$obj1\n");
				}
				else
				{
					print("$file\t$obj\t$ittot\t$itbest\t$ttot\t$tbest\n");
				}
			}
		}	
	}
}


# main part

if (@ARGV[0]=~/^-f$/)
{
	$paramf=1;
	shift @ARGV;
	print("file\tobj\tittot\titbest\tttot\ttbest\tobj0\tobj1\n");
}
else
{
	print("file\tobj\tittot\titbest\tttot\ttbest\n");
}

processlist(@ARGV);

