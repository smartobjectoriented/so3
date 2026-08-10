# Build environment for the SO3 Infrabase build system.
#
# The image carries ONLY the host environment: cross toolchains, host
# packages, Python. It contains NO project source. The repository stays
# on the host and is bind-mounted by scripts/dbuild.sh at its own
# absolute path, so the same tree can be built from inside or outside
# the container interchangeably (bitbake stamps, the buildroot host tree,
# CMake caches and the *.attach.sha256 manifests all embed absolute paths).
#
# Build the image:  scripts/dbuild.sh --build
# Use it:           scripts/dbuild.sh build.sh bsp-linux
#                   scripts/dbuild.sh st.sh -d
#
# The build context is this directory only — never the project root,
# which holds tens of GB of build output.
#
# Copyright (c) 2026 REDS Institute - HEIG-VD

# ---------------------------------------------------------------------
# Stage 1 — fetch the Arm cross toolchains
# ---------------------------------------------------------------------

FROM ubuntu:24.04 AS toolchains

# Pinned deliberately: these are the versions the build is validated
# with. Bump them here, rebuild the image and re-validate — never by
# installing something else on a host.
#
# Only the 64-bit toolchains are downloaded. The 32-bit ones come from
# apt (see packages.txt) because Ubuntu's packages carry exactly the
# prefixes the recipes pin. The 64-bit ones do NOT: Ubuntu's
# gcc-aarch64-linux-gnu installs "aarch64-linux-gnu-", while
# IB_TOOLCHAIN:virt64/:rpi4_64 and the patched kernel Makefile pin
# "aarch64-none-linux-gnu-".

ARG AARCH64_LINUX_VER=12.3.rel1
ARG AARCH64_ELF_VER=12.3.rel1
ARG ARM_BASE=https://developer.arm.com/-/media/Files/downloads/gnu

RUN apt-get update && apt-get install -y --no-install-recommends \
	ca-certificates curl xz-utils \
	&& rm -rf /var/lib/apt/lists/*

RUN mkdir -p /opt/toolchains

# Linux-hosted toolchain: 64-bit kernel and U-Boot (virt64, rpi4_64).

RUN curl -fSL "${ARM_BASE}/${AARCH64_LINUX_VER}/binrel/arm-gnu-toolchain-${AARCH64_LINUX_VER}-$(uname -m)-aarch64-none-linux-gnu.tar.xz" \
	| tar -xJ -C /opt/toolchains \
	&& mv /opt/toolchains/arm-gnu-toolchain-*-aarch64-none-linux-gnu \
	      /opt/toolchains/aarch64-none-linux-gnu

# Bare-metal toolchain: the SO3 kernel and AVZ, 64-bit. Selected by
# CONFIG_CROSS_COMPILE in the SO3 defconfig, NOT by IB_TOOLCHAIN — which
# is why it is so easy to miss on a hand-built host.

RUN curl -fSL "${ARM_BASE}/${AARCH64_ELF_VER}/binrel/arm-gnu-toolchain-${AARCH64_ELF_VER}-$(uname -m)-aarch64-none-elf.tar.xz" \
	| tar -xJ -C /opt/toolchains \
	&& mv /opt/toolchains/arm-gnu-toolchain-*-aarch64-none-elf \
	      /opt/toolchains/aarch64-none-elf

# ---------------------------------------------------------------------
# Stage 2 — the build environment proper
# ---------------------------------------------------------------------

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# BitBake requires a UTF-8 locale.

RUN apt-get update && apt-get install -y --no-install-recommends locales \
	&& localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8 \
	&& rm -rf /var/lib/apt/lists/*

ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8

# Host packages come from packages.txt — the single source of truth.
# Adding a dependency there is enough.
#
# The list covers building AND running QEMU: qemu/build/qemu-system-* is
# built into the bind-mounted tree but resolves its libraries inside the
# container.

COPY packages.txt /tmp/packages.txt

RUN apt-get update \
	&& sed -e 's/#.*//' -e '/^[[:space:]]*$/d' /tmp/packages.txt \
	   | xargs apt-get install -y --no-install-recommends \
	&& apt-get install -y --no-install-recommends vim nano less \
	&& rm -rf /var/lib/apt/lists/* /tmp/packages.txt

# pcpp: pre-processor used by the SO3 ITS/device-tree tooling.

RUN pip3 install --break-system-packages --no-cache-dir pcpp

COPY --from=toolchains /opt/toolchains /opt/toolchains

ENV PATH=/opt/toolchains/aarch64-none-linux-gnu/bin:/opt/toolchains/aarch64-none-elf/bin:${PATH}

# The build escalates a few operations (losetup, mount, mkfs, cpio with
# device nodes) through `sudo -n`. Inside a throw-away build container a
# blanket NOPASSWD rule is the simplest correct answer, and it makes the
# host-side tty_tickets problem (scripts/common/setup_sudo.sh) moot.
#
# `Defaults !authenticate` is NOT redundant with NOPASSWD. The front-end
# scripts open their session with `sudo -v`, which validates the user
# rather than a specific command: sudo then requires authentication as
# soon as ANY applicable rule asks for it — and the build user also
# matches Ubuntu's `%sudo ALL=(ALL:ALL) ALL`. Without this line,
# `sudo -n true` succeeds while `sudo -v` fails with "a terminal is
# required to read the password", and every `build.sh bsp-linux*` /
# `deploy.sh` dies at sudo_session_start.

RUN printf 'Defaults !authenticate\nALL ALL=(ALL) NOPASSWD:ALL\n' \
	> /etc/sudoers.d/build-env \
	&& chmod 0440 /etc/sudoers.d/build-env

# The tree is bind-mounted and owned by the host user; keep git quiet if
# the ids ever fail to line up.

RUN git config --system --add safe.directory '*'

COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

# rc file for `dbuild.sh` with no command: an interactive shell with
# env.sh already sourced (see the file for why it must be sourced there
# and not in a parent).

COPY bashrc /etc/so3-build.bashrc

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["bash"]
