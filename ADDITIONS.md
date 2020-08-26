# Additions - Part of CI Project for REDS

* `st2` a modified version of the `st` start script that does not rely on root privileges and has no networking.

The `deploy.sh` script has been entirely rewritten as well as SD card related scripts in order to generate the sd card image without root privileges. Root privileges are now only required when a real SD card requires to be written, which is a manual oration most of the time.

## Toolchain folder

The toolchain folder `toolchain` holds a single script to install the toolchain (in this folder) and export environment variables to the user.

* `setup_env` can be sourced to get the toolchain (it checks if the toolchain is installed, if not it will download and install it, and export the binaries from the toolchain). Sourcing this script before cross-compiling is recommended.

## CI folder

The CI (Continuous Integration) folder `ci` holds a few new files :

* `Jenkinsfile` describes the Jenkins pipeline used for continuous integration.
* `cukinia.conf` is the configuration for the [cukinia](https://github.com/savoirfairelinux/cukinia) test suite. It holds the tests that should be run during the "test" stage of the pipeline.
* `setup_cukinia.sh` a script to setup the cukinia executable.

### Setting up a Jenkins pipeline

- From the main Jenkins interface (in browser) select "New Item"
- Enter an item name e.g., "SO3 pipeline"
- Select "Pipeline"
- Click OK
- (optional) add a description
- Select options e.g., "Do not allow concurrent builds" (nothing mandatory)
- Select build triggers (nothing mandatory but "Poll SCM" may be of interest)
- Under Pipeline
  - Select "Pipeline script from SCM"
  - Select "git" as "SCM"
  - Fill the "Repository URL" with the SSH git link, e.g., `git@gitlab.com:smartobject/so3.git` 
  - Select or add the credentials (SSH private key, not your own, create a new one and put the public key as a deploy key in the git repository)
  - Select the branches to build (e.g., `*/ci`)
  - Set the "Script Path" `ci/Jenkinsfile` (other pipelines may use other Jenkinsfiles, the file can be named with any name)
- Save the pipeline

#### Launching / Testing the pipeline

Select "Build Now" from the interface. If some stage fails see the logs.

### Pipeline Steps sub folder

The Jenkinsfile that describes the pipeline will call the associated script from this folder `ci/pipeline_steps` for each step of the pipeline. This is done to make it more convenient to run each step manually..

### Test Scripts sub folder

Supplemental test scripts are stored in `ci/test_scripts`.

* `so3_test_script.sh` is a generic script to check if SO3 boots, but can also run test programs and check their output. This is used in `cukinia.conf` to run tests in SO3.

### Adding a test

In order to add a test modify the cukinia.conf file with the command you want to launch.

If you want to create a userspace test for SO3 create it in `usr/tests/src` using `usr/tests/src/test.h` to specify if a test is successful or not. Then use the `so3_test_script.sh` to launch it by passing the executable name as argument.

The `so3_test_script.sh` will launch SO3 in qemu and launch the specified executable. It will fail if there is no so3 prompt, if the executable returns as a failed test, or if the executable crashes or there is a timeout. Exact behavior can be found inside of the script (expect is used for interaction).

The test suite will catch the first call to `SO3_TEST_SUCCESS(...)` or `SO3_TEST_FAIL(...)` in a test program so therefore usually the program returns after a failed test and only calls success if everything it tests did work.

### Running the tests manually

In the `ci` directory run :

```shell
    ./setup_cukinia.sh
    ./cukinia cukinia.conf
```

If a test fails the command can be run manually to see what went wrong e.g.,

```shell
    ./test_scripts/so3_test_script.sh fail01
```

Since this was supposed to be run in a pipeline, it supposes the previous stages have been run (e.g., build qemu, build so3, build user space apps and tests, etc.). If this is run manually check out the `ci/Jenkinsfile` in order to execute the missing previous steps manually.

## Userspace folder

### Tests folder

Under `usr/tests/src` are test programs that check the kernel functionality as a black box through syscalls.

These test programs are run through the automated test suite (Cukinia in the test stage of the Jenkins pipeline)

## File System folder

The `create_img.sh` script has been replaced by two scripts `create_partitions.sh` and `populate_sd_image.sh`. The new script do not require root privileges at all. The final populated sd image can be copied to an actual sd card using dd.

The new scripts (e.g., deploy.sh) make use of tools such as mke2fs and mcopy in order to copy files to the partitions without mounting them (which requires root privileges). The partitions can still be mounted for e.g., manual inspection or modification.

This is removes the need to run anything with root privileges and so will save the user from harm (e.g., formatting the wrong /dev/entry) as well as allow for easier automation.

## Rootfs / Ramfs folder

The `create_ramfs.sh` script has been rewritten so that it does not require any privileged commands, it will create an image with a FAT partition. The `deploy.sh` has also been rewritten so that it does not require any privileged commands. This script also copies the contents of `usr/out/` in the file system if no specific folder is passed as an argument.
