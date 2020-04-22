
#!/bin/bash

# Deploy usr apps into the first partition
echo Deploying usr apps into the first partition...
cd ../filesystem
./mount.sh 1

sudo cp ../usr/out/*.elf fs/
sudo cp ../usr/resources/* fs/

sudo ./umount.sh
