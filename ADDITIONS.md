# Additions - Part of CI Project for REDS

* `st2` a modified version of the `st` start script that does not rely on root priviledges and has no networking.

The `deploy.sh` script has been entirely rewritten as well as SD card related scripts in order to generate the sd card image without root priviledges. Root priviledges are now only required when a real SD card requires to be written, which is a manual opration most of the time.

## Toolchain folder

The toolchain folder `toolchain` holds a single script to install the toolchain (in this folder) and export environment variables to the user.

* `setup_env` can be sourced to get the toolchain (it checks if the toolchain is installed, if not it will download and install it, and export the binaries from the toolchain). Sourcing this script before cross-compiling is recommended.

## CI folder

The CI (Continuous Integratoin) folder `ci` holds a few new files :

* `Jenkinsfile` describes the Jenkins pipeline used for contiunous integration.
* `cukinia.conf` is the configuration for the [cukinia](https://github.com/savoirfairelinux/cukinia) test suite. It holds the tests that should be run during the "test" stage of the pipeline.
* `setup_cukinia.sh` a script to setup the cukinia executable.

### Test Scripts sub folder

Supplemental test scripts are stored in `ci/test_scripts`.

* `so3_test_script.sh` is a generic script to check if SO3 boots, but can also run test programs and check their output. This is used in `cukinia.conf` to run tests in SO3.

## Userspace folder

### Tests folder

Under `usr/tests/src` are test programs that check the kernel functionnality as a black box through syscalls.

These test programs are run through the automated test suite (Cukinia in the test stage of the Jenkins pipeline)

## File System folder

The `create_img.sh` script has been replaced by two scripts `create_partitions.sh` and `populate_sd_image.sh`. The new script do not require root priviledges at all. The final populated sd image can be copied to an actual sd card using dd.

The new scripts (e.g., deploy.sh) make use of tools such as mke2fs and mcopy in order to copy files to the partitions without mounting them (which requires root priviledges). The partitions can still be mounted for e.g., manual inspection or modification.

This is removes the need to run anything with root priviledges and so will save the user from harm (e.g., formatting the wrong /dev/entry) as well as allow for easier automation.
