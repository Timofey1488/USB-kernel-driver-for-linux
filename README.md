# USB-kernel-driver-for-linux
This is my education project for course work
The main programming language is "C"

# Main functions
Read some information from usb device
Write information on dev/demo*(run_driver_write.sh)
Read information from userspace(run_driver_read.sh)

# Steps for start project

1.	Remove the default driver: sudo rmmod uas usb_storage
2.  Start the start script: sh run_driver.sh
3.  Check console: sudo dmesg

There you can see some inner information about usb controller. This program is a wrapper in structure linux kernel driver. More information about this project on the link https://docs.google.com/document/d/1aq6UWYz5weHhKY6vhyDk6NXak0pbCTfi/edit?usp=sharing&ouid=116060276267125105165&rtpof=true&sd=true
