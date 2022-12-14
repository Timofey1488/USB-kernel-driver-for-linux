#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define USB_DEMO_MINOR_BASE     100

static struct mutex usb_mutex;

//Структура для всех драйверных устройств:
struct usb_demo {
	struct usb_device    *udev;
	struct usb_interface *interface;
	unsigned char        *bulk_in_buffer;
	size_t               bulk_in_size;
	__u8                 bulk_in_endpoint_addr;
	__u8                 bulk_out_endpoint_addr;
	struct kref          kref;
};


static void usb_delete(struct kref *krf) {
	struct usb_demo *dev = container_of(krf , struct usb_demo, kref);

	usb_put_dev(dev->udev);
	//очистка буфера
	kfree(dev->bulk_in_buffer);
	kfree(dev);

	return ;
}

static struct usb_driver my_usb_driver;

//Open Callback (file operation):
static int file_open(struct inode *inode, struct file *filp) 
{
	struct usb_demo      *dev;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;

	subminor = iminor(inode);

	interface = usb_find_interface(&my_usb_driver, subminor);
	if(!interface) {
		printk(KERN_ALERT "myDriver: Cannot find interface information for minor: %d\n", subminor);
		retval = -ENODEV;
		goto exit;
	}

	dev = usb_get_intfdata(interface);
	if(!dev) {
		printk(KERN_ALERT "myDriver: Cannot find device for given interface\n");
		retval = -ENODEV;
		goto exit;
	}

	//Increment usage count:
	kref_get(&dev->kref);

	filp->private_data = dev;

exit:

	return retval;
}

//Close Callback (file operation):
static int file_close(struct inode *inode, struct file *filp) {
	struct usb_demo *dev = (struct usb_demo *) filp->private_data;

	if(dev == NULL) {
		return -ENODEV;
	}

	kref_put(&dev->kref, usb_delete);

	return 0;
}

//Read Callback (file operation):
//usb_bulk_msg:
static ssize_t file_read(struct file *filp, char __user *buffer, size_t count, loff_t *ppos) {
	struct usb_demo *dev = (struct usb_demo *) filp->private_data;
	int retval = 0;
	
	printk(KERN_ALERT "myDriver: usb read method called\n");

	//Блокировка чтения bulk для получения данных с устройства:
	retval = usb_bulk_msg(	dev->udev,
				usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpoint_addr),
				dev->bulk_in_buffer,
				min(dev->bulk_in_size, count),
				(int *)&count,
				(HZ * 10));

	//если прочитано успешно:
	//копируем данные в пользовательское пространство(userspace):
	if(!retval) {
		if(copy_to_user(buffer, dev->bulk_in_buffer, count)) {
			retval = -EFAULT;
		} else {
			retval = count;
		}
	}

	return retval;
}

//После успешной передачи urb(USB request block) на USB-устройство:
//Или что-то происходит при передаче:
//Обратный вызов urb вызывается ядром USB(USB Core):
//static void file_write_callback(struct urb *urb, struct pt_regs *regs) {
static void file_write_callback(struct urb *urb) {
	if(urb->status &&
	    !(urb->status == -ENOENT     ||
	       urb->status == -ECONNRESET ||
	       urb->status == -ESHUTDOWN
	    )
	 ) {
		printk(KERN_ALERT "myDriver: nonzero write bulk status received");
	    }

	usb_free_coherent(urb->dev,
			   urb->transfer_buffer_length,
			   urb->transfer_buffer,
			   urb->transfer_dma);

	return ;
}

//Write Callback (file operation):
static ssize_t file_write(struct file *filp, const char __user * buffer, size_t count, loff_t *ppos) {
	struct usb_demo *dev = filp->private_data;
	int             retval = 0;
	struct urb      *urb = NULL;
	char            *buf = NULL;

	printk(KERN_ALERT "myDriver: usb write method called\n");

	//if no data to write, exit:
	if(!count) {
		printk(KERN_ALERT "myDriver: no data to write\n");
		goto exit;
	}

	//Create a urb:
	//create buffer for it:
	//copy data to the urb:
	urb = usb_alloc_urb(0, GFP_KERNEL);
	if(!urb) {
		printk(KERN_ALERT "myDriver: Could not allocate urb\n");
		retval = -ENOMEM;
		goto error;
	}

	buf = usb_alloc_coherent(dev->udev, count, GFP_KERNEL, &urb->transfer_dma);
	if(!buf) {
		printk(KERN_ALERT "myDriver: Could not allocate buffer\n");
		retval = -ENOMEM;
		goto error;
	}
	
	if(copy_from_user(buf, buffer, count)) {
		printk(KERN_ALERT "myDriver: copy_from_user failed in file_write\n");
		retval = -EFAULT;
		goto error;
	}

	//Initialize the urb properly:
	usb_fill_bulk_urb(urb,
			   dev->udev,
			   usb_sndbulkpipe(dev->udev, dev->bulk_out_endpoint_addr),
			   buf,
			   count,
			   file_write_callback,
			   dev);

	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	//Send the data out the bulk port:
	retval = usb_submit_urb(urb, GFP_KERNEL);
	if(retval) {
		printk(KERN_ALERT "myDriver: usb_submit_urb failed\n");
		goto error;
	}

	usb_free_urb(urb);

exit:
	return count;

error:
	usb_free_coherent(dev->udev, count, buf, urb->transfer_dma);
	usb_free_urb(urb);
	kfree(buf);

	return retval;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open  = file_open,
	.release = file_close,
	.read  = file_read,
	.write = file_write,
};

static struct usb_class_driver demo_class = {
	.name = "usb/demo%d",
	.fops = &fops,
	.minor_base = USB_DEMO_MINOR_BASE,
};


/*------------------------------- #include<usb.h> -------------------------------*/
/* Эта функция вызывается автоматически, когда мы вставляем USB-устройство, предоставленное этим драйвером.
* успешно установлен. Если эта функция не вызывается, одной из причин может быть то, что
* какой-то другой USB-драйвер имеет доступ к USB-шине и вызывается функция проверки его драйвера
* каждый раз, когда мы вставляем USB. Вы можете проверить, какой драйвер работает, а какой вызывается
* методом отладки, предоставляемым Linux. Команда терминала для этого — dmesg. В нем перечислены все
* отладочные сообщения, распечатываемые разными драйверами. Попробуйте " dmesg | grep -i <ваш тег здесь> "
* команда для фильтрации отладочных сообщений с тегом, таким же, как вы указали в команде. Таким образом, сделать
* убедитесь, что вы сохранили тег в своем сообщении. Все, что предшествует ":" в printk(), считается тегом.
*/
static int probe(struct usb_interface *interface,
			   const struct usb_device_id *id) {
	struct usb_demo                *dev = NULL;
	struct usb_host_interface      *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	size_t buffer_size;
	int    i;
	int    retval = -ENOMEM;
	struct usb_device *connected_device;

	//Выделяем память для нашего устройства и инициализируем:
	dev = kmalloc(sizeof(struct usb_demo), GFP_KERNEL);

	if(dev == NULL) {
		printk(KERN_ALERT "myDriver: Out Of Memory. Cannot allocate memory\n");
		goto error;
	}

	memset(dev, 0x00, sizeof(*dev));
	kref_init(&dev->kref);

	connected_device = interface_to_usbdev(interface);
	dev->udev = usb_get_dev(connected_device);
	dev->interface = interface;

	//Информация о настройке endpoint:
	//первые bulk-in, bulk-out endpoints:
	iface_desc = interface->cur_altsetting;

	//Выводим некоторую информацию об интерфейсе:
	printk(KERN_ALERT "myDriver: Name: %s \n",connected_device->product);
	printk(KERN_ALERT "myDriver: Manufacturer: %s\n",connected_device->manufacturer);
	printk(KERN_ALERT "myDriver: serial: %s",connected_device->serial);
	printk(KERN_ALERT "myDriver: Vendor id: 0x%x Product id: 0x%x",id->idVendor, id->idProduct);
	printk(KERN_ALERT "myDriver: USB interface %d probed\n", iface_desc->desc.bInterfaceNumber);
	printk(KERN_ALERT "myDriver: bNumEndpoints 0x%02x\n", iface_desc->desc.bNumEndpoints);
	printk(KERN_ALERT "myDriver: bInterfaceClass 0x%02x\n\n", iface_desc->desc.bInterfaceClass);

	for(i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
		endpoint = &iface_desc->endpoint[i].desc;

		//Выводим некоторую информацию о конечных точках:
		printk(KERN_ALERT "myDriver: Endpoint[%d]: bEndpointAddress 0x%02x\n", i, endpoint->bEndpointAddress);
		printk(KERN_ALERT "myDriver: Endpoint[%d]: bmAttributes     0x%02x\n", i, endpoint->bmAttributes);
		printk(KERN_ALERT "myDriver: Endpoint[%d]: wMaxPacketSize   0x%04x (%d)\n",
		       		  i, endpoint->wMaxPacketSize, endpoint->wMaxPacketSize);

		if(!dev->bulk_in_endpoint_addr &&
		    (endpoint->bEndpointAddress & USB_DIR_IN) &&
		    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
		      USB_ENDPOINT_XFER_BULK)) {
			//Этот случай удовлетворяет всем условиям
			//bulk in endpoint:

			buffer_size = endpoint->wMaxPacketSize;
			dev->bulk_in_size = buffer_size;
			dev->bulk_in_endpoint_addr = endpoint->bEndpointAddress;
			dev->bulk_in_buffer = kmalloc(buffer_size, GFP_KERNEL);

			if(!dev->bulk_in_buffer) {
				printk(KERN_ALERT "myDriver: Could not allocate bulk in buffer\n");
				goto error;
			}
		}

		if(!dev->bulk_out_endpoint_addr &&
		    !(endpoint->bEndpointAddress & USB_DIR_IN) &&
		    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
		    USB_ENDPOINT_XFER_BULK)) {
			//Этот случай удовлетворяет всем условиям
			//bulk out endpoint:

			dev->bulk_out_endpoint_addr = endpoint->bEndpointAddress;
		}
	}

	if(!dev->bulk_in_endpoint_addr && !dev->bulk_out_endpoint_addr) {
		printk(KERN_ALERT "myDriver: Could not find either bulk in endpoint or"
				  " bulk out endpoint\n");
		goto error;
	}

	//Сохраняем указатель на наши данные на это устройство:
	//Функция, предоставляемая USB Core;
	usb_set_intfdata(interface, dev);

	//Регистрация устройства в ядре USB:
	// Готов к использованию:
	retval = usb_register_dev(interface, &demo_class);

	if(retval) 
	{
		printk(KERN_ALERT "myDriver: Error registering Driver to the USB Core\n");
		printk(KERN_ALERT "myDriver: Not able to get minor for the Device\n");
		//Сбросить указатель на наши данные на устройстве:
		usb_set_intfdata(interface, NULL);
		goto error;
	}

	printk(KERN_ALERT "myDriver: USB Device now attached to USBDemo-%d", interface->minor);
	return 0;

error:

	if(dev) {
		kref_put(&dev->kref, usb_delete);
	}

	return retval;
}


/* Эта функция вызывается автоматически, когда мы извлекаем USB-устройство. Если функция probe
* этого драйвера работает, то эта функция точно будет работать.
*/
static void disconnected(struct usb_interface *interface) {
	struct usb_demo *dev;
	int minor = interface->minor;

	mutex_lock(&usb_mutex);

	dev = usb_get_intfdata(interface);
	usb_set_intfdata(interface, NULL);

	//give back minor:
	usb_deregister_dev(interface, &demo_class);

	mutex_unlock(&usb_mutex);

	kref_put(&dev->kref, usb_delete);

	printk(KERN_ALERT "myDriver: USB Demo #%d now disconnected", minor);
	
	return ;
}

/* Это список USB-устройств, поддерживаемых этим драйвером.
* он находится в паре идентификатора поставщика (VendorID) и идентификатора продукта (ProductID) USB-устройства.
* Последняя {} пустая запись называется записью завершения. 
*/
static struct usb_device_id supported_device[] = {

	//{ USB_DEVICE(VEND_ID,PROD_ID)},
	// {USB_DEVICE(0x04ca,0x0061)},	// USB оптическая мышь
	// {USB_DEVICE(0x0bda,0x0129)},	// Принтер
	// {USB_DEVICE(0x054c,0x09c2)},	// Sony USB memory 	
	{USB_DEVICE(0x8564,0x1000)},
	{USB_DEVICE(0x090c,0x1000)},
	
		// JetFlash USB memory(Именно с ним и работает) 
	// {USB_DEVICE(0x0bc2,0xab26)},	// Seagate 1TB harddrive
	// {USB_DEVICE(0x22d9,0x2773)},	//
	{}								//Завершение записи
};

MODULE_DEVICE_TABLE(usb, supported_device);


/* Это основная структура драйвера устройства, где мы регистрируем все наши функции
* к функциям вызова по умолчанию.
*/
static struct usb_driver my_usb_driver = {
	.name = "my_usb_driver",
	.id_table = supported_device,
	.probe = probe,
	.disconnect = disconnected,
};


/* Это функция, которая играет роль в установке драйвера USB и регистрации
* его в ядре. Чтобы зарегистрировать его в ядре, он использует функцию, предоставленную дистрибутивом Linux
* вызывается с помощью usb_register(). Это возвращает значение, которое указывает результат регистрации.
* Возвращает 0 в случае успеха и отрицательные значения в случае неудачи.
*/

static int __init usb_drv_init(void) {
	int result;

	//Регистрация драйвера в подсистеме USB:
	result = usb_register(&my_usb_driver);

	//Инициализация usb_lock:
	mutex_init(&usb_mutex);

	if(result) {
		printk(KERN_ALERT "myDriver: [%s:%d]: usb_register failed\n",
		       __FUNCTION__, __LINE__);
	}

	return result;
}


/* Это функция для в удалении драйвера USB и отмене регистрации
* его в ядре. Чтобы отменить его регистрацию в ядре, он использует функцию, предоставленную дистрибутивом Linux.
* вызывается с помощью usb_deregister()
*/
static void __exit usb_drv_exit(void) {
	//Отменяет регистрацию этого драйвера в подсистеме USB:
	usb_deregister(&my_usb_driver);
}


/*------------------------------- #include<module.h> -------------------------------*/
/* Здесь мы регистрируем нашу функцию установки драйвера в функции, предоставляемой
* Дистрибутив Linux называется module_init(). Важно отметить, что в Linux драйверы называются
* модули. Посмотреть список модулей, работающих в фоновом режиме, можно с помощью команды lsmod.
* Он показывает список всех модулей, работающих в фоновом режиме. Попробуйте lsmod | grep -i usb к файлору
* Модули с usb word внутри.
*/
module_init(usb_drv_init);

/* Здесь мы регистрируем нашу функцию установки драйвера в функции, предоставляемой
* Дистрибутив Linux называется module_exit(). Это указывает на то, что всякий раз, когда пользователь хочет удалить
* этот драйвер, вызовите функцию pen_exit.
*/
module_exit(usb_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("we 4");
MODULE_DESCRIPTION("Собственный usb-driver");