# Vagrant

This page shows you how to create a virtual machine image provisioned with all tools required to use the Contiki-NG ecosystem.

## Instructions

* Install a virtualization platform such as [VirtualBox](https://www.virtualbox.org)
* Install Vagrant from [www.vagrantup.com/downloads](https://www.vagrantup.com/downloads.html)
* Get the Contiki-NG repository
```bash
$ sudo apt-get install git git-lfs
$ git clone https://github.com/contiki-ng/contiki-ng.git
```
* Initialize Vagrant image
```bash
$ cd contiki-ng/tools/vagrant
$ vagrant up
``` 

* If you get an error "Vagrant failed to initialize at a very early stage", you might need to cleanup your pluging as follows:
```
$ vagrant plugin expunge --reinstall
```

* Log in to the Vagrant image
```bash
$ vagrant ssh
``` 
* In case of a Windows host, you may have to convert line endings of the bootstrap.sh script. From the Vagrant shell:
```bash
$ sudo apt update
$ sudo apt install dos2unix
$ dos2unix contiki-ng/tools/vagrant/bootstrap.sh
```
* Install Contiki-NG toolchain. From the Vagrant shell:
```bash
$ ./contiki-ng/tools/vagrant/bootstrap.sh
```
* Exit the SSH session

* Restart the vagrant image.
```bash
$ vagrant reload
``` 

* Log in to the Vagrant image again
```bash
$ vagrant ssh
``` 

That's it :)
You have a provisioned Linux box with all the required dependencies to get started with Contiki-NG.
If you want to enable the VirtualBox GUI, edit the `tools/vagrant/Vagrantfile`, remove the leading `#` symbols in the following section:
```
  # config.vm.provider "virtualbox" do |vb|
  #   # Display the VirtualBox GUI when booting the machine
  #   vb.gui = true
  ##
  #   # Customize the amount of memory on the VM:
  #   vb.memory = "1024"
  # end
```

## Provision a VM with a desktop environment
This assumes that you have already completed the instructions above to provision a gui-less image.

* Edit `tools/vagrant/Vagrantfile` and uncomment the following lines:

```
  # config.vm.provider "virtualbox" do |vb|
  #   # Display the VirtualBox GUI when booting the machine
  #   vb.gui = true
[...]
  # end
```
Within the commented block there is also a `  #   vb.memory = "1024"`. Leave that one commented.

* Run `sudo ./contiki-ng/tools/vagrant/bootstrap-vbox-with-x.sh`.

This will install VirtualBox guest additions, X and the Xfce desktop environment.

* Start the image. This will now show the VirtualBox GUI.
* Login using username 'vagrant' and password 'vagrant'
* `sudo startx`

## Making local modifications to your Vagrant image

You may wish to make local modifications to the Vagrant image, to e.g. add to the list of directories to be shared between your computer and the Contiki-NG Vagrant image. The easy way to do this is by modifying the `Vagrantfile` in the Contiki-NG tree, but this will cause git to flag the file as changed. Alternatively, you can exploit [how Vagrant loads Vagrantfiles](https://www.vagrantup.com/docs/vagrantfile/):

> An important concept to understand is how Vagrant loads Vagrantfiles. Vagrant actually loads a series of Vagrantfiles, merging the settings as it goes. This allows Vagrantfiles of varying level of specificity to override prior settings. 

Therefore, you can for example modify (or create) `~/.vagrant.d/Vagrantfile` and extend the VM's configuration therein.

## Connecting USB devices to your vagrant guest image

The instructions on how to achieve this largely depend on what virtualization environment you have chosen to use. For that reason, our setup will leave USB devices connected to your host OS. To connect a USB device to the vagrant image, you will have to use the recommended method for your virtualization environment.

### An example for VirtualBox
In all cases, start by installing the VirtualBox Extension Pack which will add USB support to VirtualBox.

#### VirtualBox: Adding USB devices manually
Make sure the vagrant image is not running. If you have previously fired it up with `vagrant up`, shut it down with `vagrant halt`. Install the VirtualBox Extension Pack which will add USB support to VirtualBox. You then need to open the VirtualBox Manager, select your Vagrant VM and enable USB support for the guest. You can then connect/disconnect USB device as normal, or you can use filters that will allow USB devices to automatically connect to the vagrant image. When you are done re-boot your image.

#### VirtualBox: Adding USB devices by configuring the Vagrant image
You can configure the Vagrant image to enable USB support. Modify a `Vagrantfile` and add:

```
config.vm.provider "virtualbox" do |vb|
  vb.customize ["modifyvm", :id, "--usb", "on"]
end
```

You can also create a filter such that your USB device will connect automatically with the Vagrant image as soon as its plugged in. To achieve this, add this line to the virtualbox provider configuration block that you created above (Don't forget to select correct values for `name`, `vendorid` and `productid` below):

```
vb.customize ['usbfilter', 'add', '0', '--target', :id, '--name', '<a name for the filter>', '--vendorid', '<VID>', '--productid', '<PID>']
```
