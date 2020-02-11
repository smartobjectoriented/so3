
#!/bin/bash

# Deploy usr apps into the first partition
echo Deploying usr apps into the agency partition...
cd ../filesystem
./mount.sh 1
sudo cp ../usr/*.elf fs/
sudo ./umount.sh
