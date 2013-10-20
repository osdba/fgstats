#!/usr/bin/perl
# written by tangcheng 20090916 for dsc

use strict;

use POSIX qw(strftime);
use Time::Local;

use File::Basename;
use DBI;

our $g_mgrdbname;
our $g_mgrdbip;
our $g_mgrdbport;
our $g_mgrdbuser;
our $g_mgrdbpass;

my $mgrsth;
my $mgrdbh;
my $sql;

my $partname;

my $currfilepath=dirname($0);
loadcfgpara($currfilepath."/../conf/fgstats.conf");

my $currtime=time()+3600*24;
$partname= strftime "p%Y%m%d", localtime($currtime);
my $limittime = $currtime - $currtime%86400 +86400 - 3600*8;

$mgrdbh=getmgrdbconnect();

#按天增加db_stats_keepalive的分区
$sql="CREATE TABLE db_stats_keepalive_".$partname." 
(  
CHECK ( collect_time >= ".($limittime-86400)." AND collect_time < ".$limittime." )
) INHERITS (db_stats_keepalive)";

print $sql."\n";
$mgrdbh->do($sql);

#
#$sql="grant select on db_stats_keepalive_".$partname." to admin";
#print $sql."\n";
#$mgrdbh->do($sql);

$sql="CREATE INDEX idx_db_stats_keepalive_".$partname." ON db_stats_keepalive_".$partname." (collect_time)";
print $sql."\n";
$mgrdbh->do($sql);

$sql="CREATE RULE db_stats_keepalive_insert_".$partname." AS
ON INSERT TO db_stats_keepalive 
 WHERE ( collect_time >= ".($limittime-86400)." AND collect_time < ".$limittime." )
  DO INSTEAD
    INSERT INTO db_stats_keepalive_".$partname."  
	VALUES ( NEW.* )";
print "execute sql:".$sql."\n";
$mgrdbh->do($sql);
$mgrdbh->commit();


#删除db_stats_keepalive的91天前的分区
my $delpartname= strftime("p%Y%m%d",localtime($currtime -91*24*3600 ));

$sql="drop rule if exists db_stats_keepalive_insert_".$delpartname." on db_stats_keepalive";
print $sql."\n";
$mgrdbh->do($sql);

$sql="drop table  if exists db_stats_keepalive_".$delpartname;
print $sql."\n";
$mgrdbh->do($sql);

$mgrdbh->commit();
$mgrdbh->disconnect();

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
    if( ! $mgrdbh)
    {
        printlog("ERROR: Can't connect to database,connection string:$connstr".
                 $DBI::errstr);
        exit(-1);
    }
    return $mgrdbh;
}


