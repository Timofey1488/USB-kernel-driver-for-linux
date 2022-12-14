#nautilus --browser /dev/ &
#проверяем, есть ли в нем файл со словом demo. Что указывает, вставлено ли USB-устройство или нет.

if find /dev/ -name "demo*" -print -quit | grep -q .
then

#try to copy a text file in that file
echo "Press any key to send data... "
read key
sudo cp -r /home/tim/downloads/* /dev/demo*

echo "\n-------------------------------------------------------------------------------------" >> log.txt
echo "--------------------------- " $(date) "---------------------------" >> log.txt
echo "-------------------------------------------------------------------------------------\n" >> log.txt

#копируем все сообщения журнала с тегом myDriver в log.txt
sudo dmesg | grep -i myDriver >> log.txt

#открывает log.txt
gedit log.txt &

fi

