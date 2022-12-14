#Перед этим вводим команду: sudo rmmod uas usb_storage

#Очищаем терминал
clear

#Входим в директорию, в которой находится наш драйвер
cd ~/usb_driver/driver

#Удаляем файл log.txt если такой существует
rm log.txt

#Выполняем команду clean из нашего makefile для очистки лишних файлов
make clean

#Удаляем существующий драйвер, если он был установлен
sudo rmmod myUSBdrive

#Выполняем сборку нашего драйвера
make

#устанавливаем наш драйвер
sudo insmod myUSBdrive.ko

