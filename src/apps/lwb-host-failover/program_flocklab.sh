DIR_CONTIKI="ETH/Contiki/Glossy/contiki-stripped-down"
DIR_APP="$DIR_CONTIKI/apps/lwb-host-failover"
# ssh laptop "flocklab_upload_hex_file.sh 101-109 $DIR_APP/lwb.sky 1" && flocklab_start_serial_reader.sh 101-109 glossy.log
flocklab_upload_hex_file.sh $OBS_ID lwb.sky 1 && flocklab_start_serial_reader.sh $OBS_ID glossy.log
# flocklab_upload_hex_file.sh.old 101-109 lwb.sky 1 && flocklab_start_serial_reader.sh 101-109 glossy.log
