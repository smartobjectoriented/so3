echo Deploying user application into the initrd filesystem ... 

./mount.sh 
sudo rm -rf fs/*

sudo cp *.elf fs
sudo cp *.txt fs

./umount.sh
