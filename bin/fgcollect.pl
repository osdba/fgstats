#!/usr/bin/perl
# written by tangcheng 20130818 for fgstats

use strict;

use IPC::SysV qw(IPC_PRIVATE S_IRWXU S_IRWXG S_IRWXO IPC_CREAT IPC_NOWAIT);
use IPC::Msg;
use IPC::Open2;


use DBI;
use File::Basename;
use POSIX qw(strftime);
#use warnings;

our $g_exitflag;
our $g_currfilepath;

our $g_mgrdbname;
our $g_mgrdbip;
our $g_mgrdbport;
our $g_mgrdbuser;
our $g_mgrdbpass;

our %g_child_stats_dict;

our $g_ipckeyfile;

our $g_savedbpid;
our @g_chldpidarray;

my $i;
my $cnt;

our $g_debug;

my $dbconn;
my $mgrsth;
my $mgrdbh;
my $currtime;
my $nexttime;

my $connstr;

my $errflag=0;

my $runresult;

our @g_taskconfig;
our @g_newtaskconfig;
my $row;

$g_debug=0;

if (@ARGV >=1)
{
    if($ARGV[0] eq "debug")
	{
	    $g_debug=1;
	}
}


$g_exitflag = 0;

$g_currfilepath=dirname($0);

our $g_getstatsql="
select c.method,b.hostid,b.hostname,c.stat_id,b.dbipaddr,b.dbport,b.dbname,b.dbuser,
       b.dbpass,b.osuser,c.stat_name,c.command,a.collect_interval
  from db_def b,collect_def a,dbstats_def c
 where a.hostid = b.hostid
   and a.stat_id = c.stat_id
   and c.status=1
 order by c.method,c.stat_id";

$SIG{INT} = \&siginal_exit;
$SIG{TERM} = \&siginal_exit;

#防止子进程结束后，还处于僵尸状态
$SIG{CHLD} = 'IGNORE';

loadcfgpara($g_currfilepath."/../conf/fgstats.conf");

#create ipc key file
$g_ipckeyfile="$g_currfilepath/fgipc.key";
if(! -e $g_ipckeyfile )
{
    system("touch $g_ipckeyfile");
}

$mgrdbh=getmgrdbconnect();

load_child_stats_dict($mgrdbh);

$mgrsth=$mgrdbh->prepare($g_getstatsql);

$mgrsth->execute;
$i=0;
while (my @recs=$mgrsth->fetchrow_array)
{
    $g_taskconfig[$i] = [ @recs ];
	$i=$i+1;
}
$mgrdbh->commit();
$mgrdbh->disconnect();

#运行保存数据的进程
$g_savedbpid = run_save_stats_process();
print "g_savedbpid=".$g_savedbpid."\n";

$i=0;
$cnt=@g_taskconfig;
print "Info: collect task count=".$cnt."\n";

for($i=0;$i<$cnt;$i++)
{
    $g_chldpidarray[$i]=run_collect_process($g_taskconfig[$i]);
    printlog ("Info: collect task hostid=".$g_taskconfig[$i][1].",hostname=".$g_taskconfig[$i][2].",stat_id=".$g_taskconfig[$i][3].",pid=".$g_chldpidarray[$i]);
}


#检测系统是否正常
my $check_config=0;
my $check_task=0;
while( !$g_exitflag)
{
    sleep(1);
	if($check_config>=5) #10秒钟检查一次配置是否被改变
	{
	    if($g_debug>0) {printlog ("Debug: start to check whether config is changed...");}
		check_config_change();
		if($g_debug>0) {printlog ("Debug: It is finished that checking whether config is changed.");}
		$check_config=0;
	}
	if($check_task>=5)
	{
	    if($g_debug>0) {printlog("Debug: start to check task is running...");}
		check_task_is_run();
		if($g_debug>0) {printlog("Debug: check task is finished.");}
		$check_task=0
	}
	$check_task++;
	$check_config++;
}

#检查配置是否发生变化，如果发生变化则重新加载数据
sub check_config_change
{
    my $i;
	my $j;
    my $cnt1;
    my $cnt2;
	my $mgrdbh;
	my $mgrsth;
	my $childpidcnt;
	my $oldpid;

    $mgrdbh=getmgrdbconnect();
    $mgrsth=$mgrdbh->prepare($g_getstatsql);

    $mgrsth->execute;

	if ($mgrsth->err())
    {
	    printlog("Error: execute sql error,sql statement:\n$g_getstatsql \n".$DBI::errstr);
        $mgrdbh->commit();
	    $mgrdbh->disconnect();
		return;
	}

    $i=0;
    while (my @recs=$mgrsth->fetchrow_array)
    {
        $g_newtaskconfig[$i] = [ @recs ];
	    if($g_debug>0)
		{
		    printlog("Debug: current config: hostname=".$g_newtaskconfig[$i][3].",hostid=".$g_newtaskconfig[$i][2].",statid=".$g_newtaskconfig[$i][4]);
		}

	    $i=$i+1;
    }
    $mgrdbh->commit();
	$mgrdbh->disconnect();
	splice(@g_newtaskconfig,$i);

	$cnt1=@g_newtaskconfig;
	if($g_debug>0) {printlog("Debug: current task config count=".$cnt1);}

	$cnt1=@g_newtaskconfig;
	for($i=0;$i< $cnt1;$i++)
    {
	    $cnt2=@g_taskconfig;
		for($j=0;$j<$cnt2;$j++)
		{
		    if($g_chldpidarray[$j]==0)
			{
			    next;
			}

			if( ($g_newtaskconfig[$i][1] eq $g_taskconfig[$j][1] ) && ($g_newtaskconfig[$i][3]eq $g_taskconfig[$j][3]) )
			{
			    #如果数据发生变化了
				if($g_newtaskconfig[$i][0]  ne $g_taskconfig[$j][0]
				 ||$g_newtaskconfig[$i][4]  ne $g_taskconfig[$j][4]
				 ||$g_newtaskconfig[$i][5]  ne $g_taskconfig[$j][5]
				 ||$g_newtaskconfig[$i][6]  ne $g_taskconfig[$j][6]
				 ||$g_newtaskconfig[$i][7]  ne $g_taskconfig[$j][7]
				 ||$g_newtaskconfig[$i][8]  ne $g_taskconfig[$j][8]
				 ||$g_newtaskconfig[$i][9]  ne $g_taskconfig[$j][9]
				 ||$g_newtaskconfig[$i][10] ne $g_taskconfig[$j][10]
				 ||$g_newtaskconfig[$i][11] ne $g_taskconfig[$j][11]
				)
				{
					#重新运行这个采集进程
					$oldpid=$g_chldpidarray[$j];
					mykill($g_chldpidarray[$j]);
					$g_chldpidarray[$j]=run_collect_process($g_newtaskconfig[$i]);
					printlog("Notice: hostid=".$g_taskconfig[$j][1].",hostname=".$g_taskconfig[$j][2].",statid=".$g_taskconfig[$i][3].",pid=".$oldpid." config is changed,restart collect process,new pid=".$g_chldpidarray[$j]);
					$g_taskconfig[$j]=$g_newtaskconfig[$i];
				}
				last;
			}
		}

		#没有找到，说明是新的配置项，启动一个新的进程处理这个任务
		if($j==$cnt2)
		{
			$childpidcnt=@g_chldpidarray;
			$g_chldpidarray[$childpidcnt]=run_collect_process($g_newtaskconfig[$i]);
			$g_taskconfig[$childpidcnt]=$g_newtaskconfig[$i];
            printlog("Notice: hostid=".$g_newtaskconfig[$i][1].",hostname=".$g_newtaskconfig[$i][2].",statid=".$g_newtaskconfig[$i][4]." is new config ,run new process to collect,pid=".$g_chldpidarray[$childpidcnt]);
		}
	}

	$cnt1=@g_taskconfig;
	for($i=0;$i< $cnt1;$i++)
    {
		$cnt2=@g_newtaskconfig;
		for($j=0;$j<$cnt2;$j++)
		{
			if($g_taskconfig[$i][1] eq $g_newtaskconfig[$j][1]
			&& $g_taskconfig[$i][3] eq $g_newtaskconfig[$j][3])
			{
			    last;
			}
		}

		#没有找到，说明是原先的任务已被删除，则kill掉原先的任务
		if($j==$cnt2)
		{
        	printlog("Notice: hostid=".$g_taskconfig[$i][1].",hostname=".$g_taskconfig[$i][2].",statid=".$g_taskconfig[$i][3].",pid=".$g_chldpidarray[$i]." config is droped ,stop process.");
			mykill($g_chldpidarray[$i]);
			splice(@g_taskconfig,$i,1);
			splice(@g_chldpidarray,$i,1);
			if($i==$cnt1-1)
			{
			    last;
			}
			$cnt1=$cnt1-1;
		}
	}
}

sub mykill
{
    my $ret;
	my ($pid)=@_;
	my $i;
	my $cnt=5;

	$ret= kill(15, $pid);
	for($i=0;$i<$cnt;$i++)
	{
		$ret= kill(0,$pid);
	    if(!$ret)
		{
		    return 0;
		}
		sleep(1);
	}
	if($i==$cnt)
	{
	   $ret= kill(9,$pid);
	}
	return 0;
}


#检查采集任务是否在运行，如果退出了，则重新运行
sub check_task_is_run
{
	my $childpidcnt;
	my $i;
	my $oldpid;
	$childpidcnt=@g_chldpidarray;
	for($i=0;$i<$childpidcnt;$i++)
	{
	    if($g_chldpidarray[$i]==0) #是被删除的任务
		{
		    next;
		}

		my $ret= kill(0,$g_chldpidarray[$i]);
		if(!$ret) #如果进程不存在，重新运行这个进程
		{
			$oldpid=$g_chldpidarray[$i];
			$g_chldpidarray[$i]=run_collect_process($g_taskconfig[$i]);
		    printlog("Notice: hostid=".$g_taskconfig[$i][2].",statid=".$g_taskconfig[$i][3].",pid=".$oldpid." is abnormal quit, restart it,new pid=".$g_chldpidarray[$i]);
		}
	}

	#检查保存数据的进程是否存在
	my $ret= kill(0, $g_savedbpid);
	if(!$ret) #如果进程不存在，重新运行这个进程
	{
		$oldpid=$g_savedbpid;
		$g_savedbpid = run_save_stats_process();
		printlog("Notice: save statistics process pid=".$oldpid." is abnormal exit, restart it,new pid=".$g_savedbpid);
	}

}

sub exec_single_result_sql
{
    my $dbh = $_[0];
    my $sql = $_[1];
    my $len = @_;

    my $sth=$dbh->prepare($sql);
    
    for (my $i=0; $i < $len - 2; $i++)
    {
        printlog("bind_param(".($i+1).",".$_[$i+2].")\n");
        $sth->bind_param($i+1, $_[$i+2]);
    }

    printlog("ExecuteSQL: ".$sql."\n");
    $sth->execute();
    my @recs=$sth->fetchrow_array;
    $sth->finish();
    return $recs[0];
}

sub exec_no_result_sql
{
    my $len = @_;
    my $dbh = $_[0];
    my $sql = $_[1];

    my $sth=$dbh->prepare($sql);
    for (my $i=0; $i < $len - 2; $i++)
    {
        printlog("bind_param(".($i+1).",".$_[$i+2].")\n");
        $sth->bind_param($i+1, $_[$i+2]);
    }
    printlog("ExecuteSQL: ".$sql."\n");
    $sth->execute();
    $sth->finish();
}

sub load_child_stats_dict
{
    my ($dbh) = @_;
    our %g_child_stats_dict = {};
    
    my $sql='select stat_id, child_id,  child_name from childstats_def order by stat_id, child_id';
    my $sth=$dbh->prepare($sql);
    $sth->execute();

    my $pre_stat_id = 0;
    my $i = 0;
    while (my @recs=$sth->fetchrow_array)
    {
        my $stat_id = $recs[0];
        if ($stat_id != $pre_stat_id)
        {
            $g_child_stats_dict{$stat_id} = {};
            $i = 0;
        }
        $g_child_stats_dict{$stat_id}{$recs[2]} = $recs[1];
        $pre_stat_id = $stat_id
    }
    $sth->finish();
}

sub get_child_id
{
    my ($stat_id, $child_stat_name)=@_;
    if(exists $g_child_stats_dict{$stat_id}{$child_stat_name})
    {
        return $g_child_stats_dict{$stat_id}{$child_stat_name};
    }
    my $sql;
	my $mgrdbh=getmgrdbconnect();
    my $sql='select count(*) from childstats_def where stat_id=? and child_name=?';
    my $cnt = exec_single_result_sql($mgrdbh, $sql, $stat_id, $child_stat_name);
    if ($cnt <= 0)
    {
        $sql='select coalesce(max(child_id),0) from childstats_def where stat_id=?';
        my $max_child_id = exec_single_result_sql($mgrdbh, $sql, $stat_id);
        $sql = "insert into childstats_def(stat_id, child_id, child_name) values(?, ?, ?)";
        exec_no_result_sql($mgrdbh, $sql, $stat_id, $max_child_id + 1, $child_stat_name);
        $mgrdbh->commit();
    }
    load_child_stats_dict($mgrdbh);
    my $child_id = $g_child_stats_dict{$stat_id}{$child_stat_name};
    printlog("New stat item: stat_id=$stat_id, child_stat_name=$child_stat_name, child_id=$child_id\n");
    $mgrdbh->disconnect();
    return $child_id;
}


sub run_save_stats_process
{
	my $ret;
	my $cnt;
	my $starttime;
	my $endtime;
	
	$g_savedbpid = fork();
	if (!$g_savedbpid)
    {
        # This is the child process.
		my ($key,$msg,$msgtype,$buf);
		my $mgrdbh;
		my $mgrsth;
        my $sql;
		$mgrdbh=getmgrdbconnect();
                $sql='insert into db_stats_keepalive(hostid,stat_id,collect_time,perfs) values(?,?,?,?)';
                $mgrsth=$mgrdbh->prepare($sql);

		$key = IPC::SysV::ftok($g_ipckeyfile,1);
		$msg =new IPC::Msg($key,0666|IPC_CREAT) or die "new IPC::Msg($key,0666|IPC_CREAT) failed!";
		$cnt = 0;
		$starttime = time();
		while( !$g_exitflag)
        {
            $msg->rcv($buf,65536) or last;

            if($g_debug >0) {printlog("DEBUG: Read line: $buf\n");}
            my @recs=split (/,/, $buf);
            my $len = @recs;
            my $hostid = $recs[0];
            my $stat_id = $recs[1];
            my $currtime = $recs[2];
            my $i = 3;
            my  @va;
            while ( $i < $len)
            {
                my $k = $recs[$i];
                $i++;
                my $v = $recs[$i];
                $i++;
                #printlog("i=$i, k=$k,v=$v\n");
                my $child_id = get_child_id($stat_id, $k);
                #printlog("get child_id=".$child_id."\n");
                $va[$child_id - 1] = $v;
            }
            
            foreach my $k ( @va )
            {
                if ($k == undef)
                {
                    $k = 0;
                }
                else
                {
                    #把前后的空格去掉
                    $k=~ s/^\s+//;
                    chomp($k);
                }
            }
            
            my $vastr = join(',', @va );
            $vastr = '{'.$vastr.'}';
            #printlog("vastr:$vastr");
            
            $mgrsth->bind_param(1, $hostid );
            $mgrsth->bind_param(2, $stat_id);
            $mgrsth->bind_param(3, $currtime);
            $mgrsth->bind_param(4, $vastr);
            $mgrsth->execute();
            if ($mgrsth->err())
            {
                printlog("Error: execute sql failed, bind parameter: $hostid, $stat_id, $currtime, $vastr \n sql statement:$sql\n".$DBI::errstr);
				$mgrdbh->disconnect();
				$mgrdbh=getmgrdbconnect();
                $mgrsth=$mgrdbh->prepare($sql);
			}
			
			$endtime = time();
			if($endtime - $starttime >2)
			{
			    $mgrdbh->commit();
			    $starttime=$endtime;
			}
        }
        $mgrdbh->commit();
		$mgrdbh->disconnect();
		exit(-1);
	}
	return $g_savedbpid;
}

sub run_collect_process
{
	my ($myarg) = @_;
	#my $a_n = @$myarg;#长度
    my @row = @$myarg;#数组
	my $childpid;

	$childpid = fork();
    if (!defined($childpid))
    {
		printlog("Error: Fork process failured!\n");
        exit();
    }
    if (!$childpid)
    {
        # This is the child process.
		$SIG{INT} = \&siginal_child_exit;
        $SIG{TERM} = \&siginal_child_exit;

        if($row[0] eq "sql")
        {
        collect_sqlstats(
                     $row[0],
                     $row[1],
                     $row[2],
                     $row[3],
                     $row[4],
                     $row[5],
                     $row[6],
                     $row[7],
                     $row[8],
                     $row[9],
                     $row[10],
                     $row[11],
                     $row[12]
                     );
        }
        else
        {
            collect_shellstats(
                     $row[0],
                     $row[1],
                     $row[2],
                     $row[3],
                     $row[4],
                     $row[5],
                     $row[6],
                     $row[7],
                     $row[8],
                     $row[9],
                     $row[10],
                     $row[11],
                     $row[12]
                     );
        }
        exit();
    }

	return $childpid;
}

sub collect_sqlstats
{
    my ($method,$hostid,$hostname,$stat_id,$dbipaddr,$dbport,$dbname,$dbuser,
        $dbpass,$osuser,$stat_name,$command,$collect_interval)=@_;
    my $key;
    my $msg;
    my $msgtype;
    my $srcdbconn;
    my $srcdbh;
    my $srcsth;

    $key = IPC::SysV::ftok($g_ipckeyfile,1);
    $msg =new IPC::Msg($key,0666|IPC_CREAT) or die "new fail!";
    $msgtype = 1;

    $srcdbconn="(DESCRIPTION=(ADDRESS_LIST=(ADDRESS=(PROTOCOL=TCP)".
                  "(HOST=$dbipaddr)(PORT=$dbport)))(CONNECT_DATA=(SID=$dbname)))";

    $connstr="dbi:Oracle:$srcdbconn";
	$srcdbh=getdbconnect($connstr,$dbuser,$dbpass);

    my $srcsql=$command;
    while( !$g_exitflag)
    {
        unless($srcdbh)
		{
            $srcdbh=getdbconnect($connstr,$dbuser,$dbpass);
            next;
		}
		$srcsth=$srcdbh->prepare($srcsql);
		unless($srcsth)
		{
            printlog("Error: $hostname,prepare sql failed:".$srcsql."\n".$DBI::errstr);
			$srcdbh->disconnect();
			$srcdbh=getdbconnect($connstr,$dbuser,$dbpass);
            next;
		}
        $srcsth->execute();
        if ($srcsth->err())
        {
            printlog("Error: $hostname, execute sql failed:".$srcsql."\n".$DBI::errstr);
			$srcdbh->disconnect();
			$srcdbh=getdbconnect($connstr,$dbuser,$dbpass);
            next;
        }
        
        my $line = '';
        while(my @recs=$srcsth->fetchrow_array)
        {
            $line = $line.",$recs[0],$recs[1]";
        }
        $currtime=time();
        $msg->snd($msgtype,"$hostid,$stat_id,$currtime".$line);
        sleep($collect_interval);
    }
}

sub collect_shellstats
{       
    my ($method,$hostid,$hostname,$stat_id,$dbipaddr,$dbport,$dbname,$dbuser,
        $dbpass,$osuser,$stat_name,$command,$collect_interval)=@_;

    my @filelist;

    my $key;
    my $msg;
    my $msgtype;
    my $msgtext;
    my $childpid;
	my $ret;

    $key = IPC::SysV::ftok($g_ipckeyfile,1);
    $msg =new IPC::Msg($key,0666|IPC_CREAT);
	unless($msg)
	{
	    printlog("Error : new IPC::Msg($key,0666|IPC_CREATE) failed, program is exit.\n");
		exit(-1);
	}
    $msgtype = 1;

    $command =~ s/\r\n/\n/g;
    $ENV{FG_HOSTNAME}=$hostname;
    $ENV{FG_DBUSER} =$dbuser;
    $ENV{FG_DBPASS} =$dbpass;
    $ENV{FG_DBIPADDR} =$dbipaddr;
    $ENV{FG_DBPORT} =$dbport;
    $ENV{FG_DBNAME} =$dbname;
    $ENV{FG_OSUSER} =$osuser;
    $ENV{FG_STATSNAME} =$stat_name;
    $ENV{FG_INTERVAL} =$collect_interval;


    my $srcpath=$g_currfilepath."/../scripts";
    unless ( -e $srcpath )
    {
        mkdir($srcpath,0755)
    }
    my $shellfile=$srcpath."/s".$stat_id."_".$hostid.".sh";
    unless( open SHELLFILE,">".$shellfile )
    {
        printlog("Error: Cannot create file $shellfile:$!");
        return;
    }
    print SHELLFILE $command;
    close SHELLFILE;
    $filelist[0]=$shellfile;
    chmod(0700,@filelist);

    while( !$g_exitflag)
    {
        $childpid=open2(my $SHELL_OUT,my $SHELL_IN, $shellfile);
        close($SHELL_IN);
        #select (SHELL_OUT);
        #$| = 1;
        #select (STDOUT);

        if($g_debug >0) {printlog("Debug: run shell(pid=$childpid) to collect.\n");}

        unless($childpid)
        {
            printlog("Error: Cannot run $shellfile:$!");
            goto exit_out;
        }
        while(my $line= <$SHELL_OUT>)
        {
            $line =~ s/\n//g;
            $line=~ s/^\s+//;
            chomp($line);
            #@recs=split (/[ \t]+/, $line);
            $currtime=time();
            $msgtext="$hostid,$stat_id,$currtime,".$line;
            #if($g_debug >0) {printlog("Debug: $msgtext\n");}
            $ret = $msg->snd($msgtype,$msgtext,IPC_NOWAIT);
            unless($ret)
			{
			    printlog("Error: msg->snd($msgtype,$msgtext,IPC_NOWAIT) failed!");
			}

			if($g_exitflag)
			{
			    kill(9,$childpid);
				last;
			}
        }
        waitpid($childpid, 0);
exit_out:
        close($SHELL_IN);
        close($SHELL_OUT);
    }
}

sub getmgrdbconnect
{
    my $connstr;
    my $mgrdbh;

    chomp($g_mgrdbip);
    chomp($g_mgrdbport);
    $connstr="dbi:Pg:dbname=$g_mgrdbname;";
    if (length($g_mgrdbip) >0)
    {
        $connstr=$connstr."host=$g_mgrdbip;";
    }
    if (length($g_mgrdbport) >0)
    {
        $connstr=$connstr."port=$g_mgrdbport;";
    }
	
    $mgrdbh = DBI->connect($connstr,$g_mgrdbuser,$g_mgrdbpass,
                          {RaiseError=>0,PrintError=>0,AutoCommit => 0});
    while( ! $mgrdbh)
    {
        printlog("Warn:Can't connect to database,connection string:$connstr".
                 $DBI::errstr);
        unless ($errflag)
        {
            $errflag=1;
            printlog("Collect statistics Error:"."Can't connect to database,connection string:$connstr\n".$DBI::errstr."\n");
        }
        sleep(10);
        $mgrdbh = DBI->connect($connstr,$g_mgrdbuser,$g_mgrdbpass,
                          {RaiseError=>0,PrintError=>0,AutoCommit => 1});
    }
    printlog("Info: Successfully connect to database, connection string:".$connstr);
    return $mgrdbh;
}


sub getdbconnect
{
	my ($connstr,$dbuser,$dbpass)=@_;
    my $dbh;
	my $cnt=50;

    $dbh = DBI->connect($connstr,$dbuser,$dbpass,
                          {RaiseError=>0,PrintError=>0,AutoCommit => 1});
    while( ! $dbh)
    {
        if($g_exitflag)
		{
		    exit(0);
		}

		printlog("Error:Can't connect to database,:$connstr ".$DBI::errstr);
		if($cnt>=50)
		{
		    printlog("Can't connect to database,connection string:$connstr\n".$DBI::errstr."\n");
			$cnt=0;
		}
		$cnt++;
        sleep(10);
        $dbh = DBI->connect($connstr,$dbuser,$dbpass,
                          {RaiseError=>0,PrintError=>0,AutoCommit => 1});
    }

	$dbh->{LongReadLen} = 1024;
    $dbh->{LongTruncOk} = 1;
    return $dbh;
}


sub loadcfgpara
{
    my ($cfgfile)=@_;

    open(CFGFILE,$cfgfile) || die ("Could not open config file:".$cfgfile."\n");
    while(my $line=<CFGFILE>)
    {
        $line=~ s/\n$//;   #trim \n
        $line=~ s/\s+$//;   #trim space

        if($line =~ m/^dbip\=/)
        {
            $g_mgrdbip=substr($line,length('dbip')+1);
        }

        if($line =~ m/^dbport\=/)
        {
            $g_mgrdbport=substr($line,length('dbport')+1);
        }

        if($line =~ m/^dbname\=/)
        {
            $g_mgrdbname=substr($line,length('dbname')+1);
        }

        if($line =~ m/^dbuser\=/)
        {
            $g_mgrdbuser=substr($line,length('dbuser')+1);
        }
        if($line =~ m/^dbpass\=/)
        {
            $g_mgrdbpass=substr($line,length('dbpass')+1);
        }
    }
    close(CFGFILE);
}

sub printlog
{
    my ($msgtext)=@_;
    #use POSIX;
    print strftime("[%Y-%m-%d %H:%M:%S]: ",localtime());
    $msgtext =~ s/\n/\n                       /g;
    print $msgtext."\n";
}

sub siginal_exit
{
    $g_exitflag = 1;
	kill(2, @g_chldpidarray);
    sleep(1);
    kill(9, @g_chldpidarray);

	print strftime("[%Y-%m-%d %H:%M:%S]: ",localtime())."Info: Main Process exit.\n";
	exit(0);
}

sub siginal_child_exit
{
    $g_exitflag = 1;
}
