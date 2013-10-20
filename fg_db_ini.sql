SET statement_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;

CREATE TABLE db_def (
    hostid integer NOT NULL,
    hostname character varying(30),--主机名称
    dbipaddr character varying(17),
    dbport integer,
    dbname character varying(30),
    dbuser character varying(30),
    dbpass character varying(64),
    osuser character varying(30),
    dbtype character varying(12),--oracle,mysql
    ostype character varying(12),--AIX,Linux,Solaris
    dbvers character varying(20),
    remark character varying(128)
);

ALTER TABLE ONLY db_def
    ADD CONSTRAINT db_def_pkey PRIMARY KEY (hostid);

--采集数据的方法定义表，一个采集项目的采集方法目前实现了两种，一种是定义一个SQL来采集，
--另一种方法是定义一个unix shell脚本来采集
--脚本和SQL中可以加变量&dbname,&dbuser,&dbpass,&dbipaddr,&dbport,&sid,&osuser,&statsname，
--这些变量在执行时会被替换成真正的值。
CREATE TABLE dbstats_def (
    stat_id integer NOT NULL,
    stat_name character varying(26),
    method character varying(12),--sql,shell
    command character varying(16000),
    remark character varying(4000),
    status integer --1,正在使用的，0-不使用
);

ALTER TABLE ONLY dbstats_def
    ADD CONSTRAINT dbstats_def_pkey PRIMARY KEY (stat_id);

CREATE TABLE childstats_def (
    stat_id integer,
    child_id integer,
    child_name character varying(64)
);

--采集周期定义表，定义了每个数据库需要收集哪些信息，什么时候收集，收集的周期是多少。
CREATE TABLE collect_def (
    hostid integer,
    stat_id integer,
    collect_interval integer DEFAULT 10
);

--采集到的数据存储在下面的表中
CREATE TABLE db_stats_keepalive (
    hostid integer,
    stat_id integer,
    collect_time integer,
    perfs bigint[]
);
CREATE INDEX idx_db_stats_keepalive_collect_time ON db_stats_keepalive USING btree (collect_time);

CREATE FUNCTION unix_timestamp() RETURNS integer AS $$
SELECT (date_part('epoch',now()))::integer;
$$ LANGUAGE SQL IMMUTABLE;

CREATE FUNCTION unix_timestamp(timestamp ) RETURNS integer AS $$
SELECT (date_part('epoch',$1))::integer;
$$ LANGUAGE SQL IMMUTABLE;


CREATE FUNCTION from_unixtime(int) RETURNS timestamp AS $$
SELECT to_timestamp($1)::timestamp;
$$ LANGUAGE SQL IMMUTABLE;



CREATE TABLE httpdbservice (
    id integer NOT NULL,
    svcname character varying(40),
    auth integer,
    sql character varying(2000),
    svctype integer
);

ALTER TABLE ONLY httpdbservice
    ADD CONSTRAINT httpdbservice_pkey PRIMARY KEY (id);

--
-- Data for Name: collect_def; Type: TABLE DATA; Schema: public; Owner: osdba
--

COPY collect_def (hostid, stat_id, collect_interval) FROM stdin;
1	1001	10
\.


--
-- Data for Name: db_def; Type: TABLE DATA; Schema: public; Owner: osdba
--

COPY db_def (hostid, hostname, dbipaddr, dbport, dbname, dbuser, dbpass, osuser, dbtype, ostype, dbvers, remark) FROM stdin;
1	ubuntu01	192.168.1.21	1521	mydb	admin	admin	root	oracle	Linux	10.2	ubuntu
2	ubuntu02	192.168.1.22	1521	mydb	admin	admin	root	oracle	Linux	10.2.0.4	ubuntu
3	ubuntu03	192.168.1.23	1521	mydb	admin	admin	root	oracle	Linux	10.2	ubuntu03
\.

--
-- Data for Name: dbstats_def; Type: TABLE DATA; Schema: public; Owner: osdba
--

COPY dbstats_def (stat_id, stat_name, method, command, remark, status) FROM stdin;
1000	v$sysstat	sql	select replace(name,' ','_'),value from v$sysstat\n where name in (\n 'consistent gets',\n 'physical reads',\n 'physical writes',\n 'logons cumulative',\n 'parse count (hard)',\n 'parse count (total)',\n 'redo entries',\n 'redo writes',\n 'sorts (disk)',\n 'sorts (memory)',\n 'sorts (rows)',\n 'user calls',\n 'user commits',\n 'user rollbacks',\n 'bytes received via SQL*Net from client',\n 'bytes sent via SQL*Net to client',\n 'bytes received via SQL*Net from dblink',\n 'bytes sent via SQL*Net to dblink',\n 'leaf node splits',\n 'execute count'\n )\nunion all\nselect replace(event,' ','_')||'_total_waits',total_waits value from v$system_event \n where event in (\n 'db file sequential read',\n 'db file scattered read',\n 'log file sync')\nunion all\nselect replace(event,' ','_')||'_time_waited',time_waited value from v$system_event \n where event in (\n 'db file sequential read',\n 'db file scattered read',\n 'log file sync')\n		1
1001	vmstat	shell	#/bin/bash\n\nplatform=`ssh $FG_OSUSER@$FG_DBIPADDR uname`\nif [ "$platform" = "Linux" ];then\n    ssh $FG_OSUSER@$FG_DBIPADDR "vmstat -n $FG_INTERVAL 100" |awk '{if($1 == $1+0){printf("kthr_r,%s,kthr_b,%s,pi,%s,po,%s,cpu_user,%s,cpu_sys,%s,cpu_wait,%s\\n",$1,$2,$7,$8,$13,$14,$16);fflush();}}'\nfi\nif [ "$platform" = "AIX" ];then\n  ssh $FG_OSUSER@$FG_DBIPADDR "vmstat $FG_INTERVAL 100" |awk '{if($1==$1+0) {printf("kthr_r,%s,kthr_b,%s,pi,%s,po,%s,cpu_user,%s,cpu_sys,%s,cpu_wait,%s\\n",$1,$2,$5,$6,$14,$15,$17);fflush();}}'\nfi\nif [ "$platform" = "HP-UX" ];then\n  ssh $FG_OSUSER@$FG_DBIPADDR "vmstat $FG_INTERVAL 100" |awk '{if($1==$1+0) {printf("kthr_r,%s,kthr_b,%s,pi,%s,po,%s,cpu_user,%s,cpu_sys,%s,cpu_wait,%s\\n",$1,$2,$8,$9,$16,$17,0);fflush();}}'\nfi		1
\.


--
-- Data for Name: httpdbservice; Type: TABLE DATA; Schema: public; Owner: osdba
--

COPY httpdbservice (id, svcname, auth, sql, svctype) FROM stdin;
1	gethostlist	1	select hostid,hostname,dbipaddr,dbport,dbname,dbuser,dbpass,osuser,dbtype,ostype,dbvers,remark from db_def order by hostname	101
5	getdbstatsdef	1	select stat_id,stat_name,method,status,command,remark from dbstats_def	101
13	get_fine_grained_dblist	0	select hostid,hostname from db_def order by hostname	101
7	deletedbstatsdef	2	delete from dbstats_def where stat_id=$1	201
8	modifydbstatsdef	2	dbstats_def where stat_id=$1	202
11	deletecollectdef	2	delete from collect_def where hostid=$1 and stat_id=$2	201
2	addhostdef	2	insert into db_def (hostid,hostname,dbipaddr,dbport,dbname,dbuser,dbpass,osuser,dbtype,ostype,dbvers,remark) values($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12)	201
3	deletehostdef	2	delete from db_def where hostid=$1	201
4	modifyhostdef	2	db_def where hostid=$1	202
6	adddbstatsdef	2	insert into dbstats_def(stat_id,stat_name,method,status,command,remark) values($1,$2,$3,$4,$5,$6)	201
9	getdbstatslist	1	select stat_id,stat_name,method,status from dbstats_def order by stat_id	101
12	addcollectdef	2	insert into collect_def(hostid,stat_id) values($1,$2)	201
10	getcollectdef	1	select hostid,stat_id from collect_def order by hostid,stat_id	101
14	get_fine_grained_stats	0	select b.child_name,a.collect_time, coalesce(a.perfs[b.child_id],0) from db_stats_keepalive a, childstats_def b\n where a.stat_id = b.stat_id\n   and a.hostid=$1\n   and b.child_name in (  $list)\n   and a.collect_time> $2 and collect_time<= $2 +660 limit 720\n	102
\.


