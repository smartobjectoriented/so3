<p align="center">
  <img src="doc/source/img/SO3_with_text.png" alt="SO3" width="420">
</p>

# Smart Object Oriented (SOO) Operating System — code name SO3

SO3 is a compact, lightweight, full-featured and extensible operating system,
particularly well suited to embedded systems. From a **single code base** it can
be built in three ways:

- **Standalone OS** — running directly on the hardware (EL1 on ARM64).
- **AVZ hypervisor** (*Agency VirtualiZer*) — running at EL2 and hosting a guest
  *agency* domain.
- **SO3 capsule** (S3C) — a lightweight guest running on top of AVZ alongside a
  Linux *agency*, as part of the **SOO** framework.

It targets ARM 32-bit and 64-bit, is multicore, and comes with a MUSL-based user
space and integrations such as LVGL, lwIP and MicroPython.

## Documentation

The complete and up-to-date documentation — architecture, build system, user
guide, debugging and more — is the source of truth. It lives in [`doc/`](doc/)
and is published at:

### 👉 https://smartobjectoriented.github.io/so3

Start there for everything about building, configuring, running and debugging
SO3.

## Supported targets

- QEMU `virt` — ARM 32-bit and 64-bit
- Raspberry Pi 4 (64-bit)
- Toradex Verdin iMX8M Plus

## Contributing

The `main` branch always holds the last released version.

> [!IMPORTANT]
> Do not push directly to `main`. Each development is tracked by an issue with
> its own branch; open a merge/pull request as soon as it is stable enough for
> review.

If you would like to contribute, please first get in touch with the maintainer at
[info@soo.tech](mailto:info@soo.tech).

## Credits

We warmly thank our sponsors for their generous support in funding the
development of the SO3 ecosystem, in particular
[HEIG-VD](https://www.heig-vd.ch) and the
[Hasler Foundation](https://haslerstiftung.ch/en/welcome-to-the-hasler-foundation).

## License

SO3 is released under the [GNU General Public License v2](LICENSE).
