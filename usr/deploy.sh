
#!/bin/bash

# Deploy usr apps into the first partition
echo Deploying usr apps into the first partition...
cd ../filesystem
./mount.sh 1
sudo cp -r ../usr/out/* fs/
sudo cp -r ../usr/resources/* fs/
sudo ./umount.sh
