cmd_/home/tim/usb_driver/driver/modules.order := {   echo /home/tim/usb_driver/driver/myUSBdrive.ko; :; } | awk '!x[$$0]++' - > /home/tim/usb_driver/driver/modules.order
