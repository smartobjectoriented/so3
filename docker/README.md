**This directory contains the Dockerfiles related to SO3.**

All images build SO3 with the **Infrabase** (bitbake) build system. The container
holds the whole repository at `/so3`; inside it the kernel is at `/so3/so3/so3`,
the user space at `/so3/so3/usr` and the bundled LVGL at `/so3/so3/usr/lib/lvgl`.

# Base build environment

- [`Dockerfile.toolchains`](./Dockerfile.toolchains) — Ubuntu image with the
  bitbake host dependencies, the SO3 build tools (dtc, u-boot-tools, mtools, QEMU
  build deps, …) and the **bare-metal kernel** cross toolchains
  (`aarch64-none-elf`, `arm-none-eabi`). The MUSL user-space toolchains are **not**
  built here — they are produced by the `meta-toolchain` layer during the build.
- [`Dockerfile.env`](./Dockerfile.env) — a thin layer over the toolchains image,
  used by the `Build` CI to compile SO3 with the repository mounted at `/so3`.

# LVGL performance test images

- [`Dockerfile.lvperf_32b`](./Dockerfile.lvperf_32b)
- [`Dockerfile.lvperf_64b`](./Dockerfile.lvperf_64b)

Images with SO3 pre-built using the `virtXX_lvperf_defconfig` configuration, used
to run [LVGL](https://lvgl.io/) performance tests under the patched QEMU.

## Getting Started

**Build** the images from the repository root. Use **`--network=host`**: the
image build fetches the components (QEMU tarball, U-Boot/AVZ git) via
Infrabase, and on hosts where the Docker *bridge* network can't resolve/reach
external mirrors (common with systemd-resolved or a VPN), the build's
`do_fetch` fails with a wget network error — host networking uses the host
resolver and works.
```bash
# 32-bit
docker build --network=host . -f docker/Dockerfile.lvperf_32b -t so3-lvperf32b
# 64-bit
docker build --network=host . -f docker/Dockerfile.lvperf_64b -t so3-lvperf64b
```

**Run** them. `--privileged` is **required**: the SD-card image is created with
`losetup`/`mkfs`/`mount` (this cannot be done during `docker build`, which is not
privileged — hence the build-time / run-time split below). Add `--network=host`
too (same reason as the build — the run-time `usr-so3` rebuild may fetch LVGL):
```bash
docker run -it --privileged --network=host -v /dev:/dev so3-lvperf64b   # or so3-lvperf32b
```

## Technical Details

- **At image-build time** (non-privileged), Infrabase builds the emulator and the
  full BSP. `build.sh bsp-so3` is privilege-free: it only compiles (the MUSL
  toolchain via `meta-toolchain`, the kernel, the user space, U-Boot) and creates
  an empty `rootfs.fat`. The privileged rootfs loop-mount is deferred to
  `deploy.sh`.
  ```
  build.sh -x qemu  &&  build.sh bsp-so3
  ```
- **At container-run time** (privileged), the entrypoint rebuilds the user space
  (picking up a mounted LVGL), creates+formats the SD-card image (`build.sh -x filesystem` —
  `losetup`/`fdisk`/`mkfs`, the privileged step that cannot run at build time),
  then `deploy.sh` assembles the FIT, populates the rootfs and writes the SD-card,
  and finally QEMU runs:
  ```
  build.sh -x usr-so3  &&  build.sh -x filesystem  &&  deploy.sh bsp-so3  &&  docker/scripts/run.sh
  ```

With `virtXX_lvperf_defconfig` the kernel runs the LVGL benchmark as its init
program; when it finishes the kernel performs a **semihosting exit**, so QEMU
(`-semihosting`) halts and the container exits with the perf output on stdout.

## Adding Additional Dependencies

To install extra dependencies without rebuilding the image, mount a shell script
at `/so3/install_dependencies.sh` (the entrypoint runs it first).

## Persistence

Each run repeats the run-time steps. To cache between runs, mount the bitbake work
tree and the boot media as volumes:

- `/so3/build/tmp` — the bitbake work tree (toolchain, QEMU, kernel, usr, sstate)
- `/so3/filesystem` — the generated SD-card image

```bash
docker run -it --privileged \
    -v /dev:/dev \
    -v "$(pwd)/../so3-build-tmp-64b:/so3/build/tmp" \
    -v "$(pwd)/../so3-filesystem-64b:/so3/filesystem" \
    so3-lvperf64b
```

> [!NOTE]
> Use separate cache volumes for the 32-bit and 64-bit images.

## Customization Options

### LVGL

The LVGL source lives at `/so3/so3/usr/lib/lvgl` and its configuration at
`/so3/so3/usr/lib/lv_conf.h`. Mount your own to benchmark them — the run-time
`build.sh -x usr-so3` rebuilds the user space against them:

```bash
docker run -it --privileged \
    -v /dev:/dev \
    -v <lvgl_path>:/so3/so3/usr/lib/lvgl \
    -v <lv_conf_path>:/so3/so3/usr/lib/lv_conf.h \
    so3-lvperf64b
```

### SO3 kernel / U-Boot

You can also override the prebuilt kernel or U-Boot binaries:

```bash
docker run -it --privileged \
    -v /dev:/dev \
    -v <patched_so3.bin>:/so3/so3/so3/so3.bin \
    -v <patched_u-boot>:/so3/u-boot/u-boot \
    so3-lvperf64b
```
