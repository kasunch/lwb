OBS="flocklab-observer200 flocklab-observer204"
OBS_PATH="/media/card/root/flocklabtestdata/ferrarif"

pscp -H "$OBS" lwb.hex "$OBS_PATH" && pssh -H "$OBS" -t 120 "tg_interf.py -t 1 && tg_reprog.py -t tmote --image=$OBS_PATH/lwb.hex && tg_usbpwr.py -t 1 -s on"
