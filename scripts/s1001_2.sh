#/bin/bash

platform=`ssh $FG_OSUSER@$FG_DBIPADDR uname`
if [ "$platform" = "Linux" ];then
    ssh $FG_OSUSER@$FG_DBIPADDR "vmstat -n $FG_INTERVAL 100" |awk '{if($1 == $1+0){printf("kthr_r %s\nkthr_b %s\npi %s\npo %s\ncpu_user %s\ncpu_sys %s\ncpu_wait %s\n",$1,$2,$7,$8,$13,$14,$16);fflush();}}'
fi
if [ "$platform" = "AIX" ];then
  ssh $FG_OSUSER@$FG_DBIPADDR "vmstat $FG_INTERVAL 100" |awk '{if($1==$1+0) {printf("kthr_r %s\nkthr_b %s\npi %s\npo %s\ncpu_user %s\ncpu_sys %s\ncpu_wait %s\n",$1,$2,$5,$6,$14,$15,$17);fflush();}}'
fi
if [ "$platform" = "HP-UX" ];then
  ssh $FG_OSUSER@$FG_DBIPADDR "vmstat $FG_INTERVAL 100" |awk '{if($1==$1+0) {printf("kthr_r %s\nkthr_b %s\npi %s\npo %s\ncpu_user %s\ncpu_sys %s\ncpu_wait %s\n",$1,$2,$8,$9,$16,$17,0);fflush();}}'
fi
