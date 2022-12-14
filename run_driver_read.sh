#!/bin/sh

#nautilus --browser /dev/ &

#Проверяем, есть ли в нем файл со словом demo, что указывает, вставлено ли USB-устройство или нет.
if find /dev/ -name "demo*" -print -quit | grep -q .
then

#если USB-устройство вставлено, пробует прочитать с него.
echo "Press any key to read data... "
read key
sudo cat /dev/demo* #>> received.txt

echo "\n-------------------------------------------------------------------------------------" >> log.txt
echo "--------------------------- " $(date) "---------------------------" >> log.txt
echo "-------------------------------------------------------------------------------------\n" >> log.txt

#copy all the log messages with myDriver tag into log file
sudo dmesg | grep -i myDriver >> log.txt

#open that log file
gedit log.txt &

fi

