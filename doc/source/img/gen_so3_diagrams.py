#!/usr/bin/env python3
# Generator for source/img/so3.drawio (multi-page) and the exported PNGs.
#
# The .drawio file is the editable source of truth; this script lets us
# regenerate it from a compact description. After editing, export the PNGs
# with the helper at the bottom of this file (see README in doc/).
#
#   python3 gen_so3_diagrams.py            # (re)generate so3.drawio
#   ./export_png.sh                        # render every page to PNG
#
import html

# ---- palette -------------------------------------------------------------
KERNEL = "fillColor=#dae8fc;strokeColor=#6c8ebf;"
USER   = "fillColor=#d5e8d4;strokeColor=#82b366;"
AVZ    = "fillColor=#ffe6cc;strokeColor=#d79b00;"
CAPS   = "fillColor=#fff2cc;strokeColor=#d6b656;"
HW     = "fillColor=#f8cecc;strokeColor=#b85450;"
NEUTRAL= "fillColor=#f5f5f5;strokeColor=#999999;"
WHITE  = "fillColor=#ffffff;strokeColor=#666666;"
NONE   = "fillColor=none;strokeColor=#444444;dashed=1;"

BOX = "rounded=1;whiteSpace=wrap;html=1;arcSize=8;"
CONT= "rounded=1;whiteSpace=wrap;html=1;arcSize=4;verticalAlign=top;fontStyle=1;"
NOTE= "shape=note;whiteSpace=wrap;html=1;size=14;"
ARR = ("edgeStyle=orthogonalEdgeStyle;rounded=1;html=1;endArrow=block;"
       "strokeColor=#444444;fontSize=12;labelBackgroundColor=#ffffff;")
ARR_D = ARR + "dashed=1;"


class Page:
    def __init__(self, name):
        self.name = name
        self.cells = []
        self.n = 1

    def _id(self):
        self.n += 1
        return f"c{self.n}"

    @staticmethod
    def _v(label):
        # drawio renders \n only when encoded as a numeric char-ref
        return html.escape(label).replace("\n", "&#10;")

    def box(self, x, y, w, h, label, style=BOX, fontsize=12, fontstyle=0):
        i = self._id()
        s = f"{style}fontSize={fontsize};"
        if fontstyle:
            s += f"fontStyle={fontstyle};"
        self.cells.append(
            f'<mxCell id="{i}" value="{self._v(label)}" style="{s}" '
            f'vertex="1" parent="1"><mxGeometry x="{x}" y="{y}" width="{w}" '
            f'height="{h}" as="geometry"/></mxCell>')
        return i

    def label(self, x, y, w, h, text, fontsize=11, bold=False):
        st = ("text;html=1;whiteSpace=wrap;align=center;verticalAlign=middle;"
              f"fontSize={fontsize};{'fontStyle=1;' if bold else ''}")
        i = self._id()

        # html=1 label: escape once more so "<recipe>" is not eaten as a tag
        text = html.escape(text, quote=False)
        self.cells.append(
            f'<mxCell id="{i}" value="{self._v(text)}" style="{st}" '
            f'vertex="1" parent="1"><mxGeometry x="{x}" y="{y}" width="{w}" '
            f'height="{h}" as="geometry"/></mxCell>')
        return i

    def edge(self, src, dst, label="", style=ARR):
        i = self._id()
        self.cells.append(
            f'<mxCell id="{i}" value="{self._v(label)}" style="{style}" '
            f'edge="1" parent="1" source="{src}" target="{dst}">'
            f'<mxGeometry relative="1" as="geometry"/></mxCell>')
        return i

    def xml(self):
        body = "".join(self.cells)
        return (f'<diagram name="{html.escape(self.name)}" id="{self.name}">'
                f'<mxGraphModel dx="1000" dy="700" grid="0" gridSize="10" '
                f'guides="1" tooltips="1" connect="1" arrows="1" fold="1" '
                f'page="1" pageScale="1" pageWidth="1100" pageHeight="800" '
                f'math="0" shadow="0"><root>'
                f'<mxCell id="0"/><mxCell id="1" parent="0"/>{body}'
                f'</root></mxGraphModel></diagram>')


pages = []

# =========================================================================
# 1. Polymorphic modes
# =========================================================================
p = Page("modes")
p.label(0, 10, 1080, 30, "SO3 — one code base, three deployment modes", 20, True)
# Standalone
p.box(30, 70, 320, 460, "Standalone OS", CONT, 16, 1)
p.box(60, 120, 260, 70, "User space\n(init, shell, apps)", USER, 16)
p.box(60, 210, 260, 90, "SO3 kernel  (EL1)\nmm · sched · vfs · net · drivers", KERNEL, 16)
p.box(60, 320, 260, 60, "Hardware (EL1)\nQEMU virt / RPi4 / i.MX8MP", HW, 16)
p.label(60, 400, 260, 90, "virt64_defconfig\n\nThe kernel owns the machine\ndirectly. No hypervisor.", 14)
# Hypervisor
p.box(390, 70, 320, 460, "Hypervisor mode", CONT, 16, 1)
p.box(420, 120, 260, 70, "Guest (EL1) — no capsules\nLinux, or plain SO3\n(CONFIG_SOO=n)", USER, 15)
p.box(420, 210, 260, 80, "AVZ hypervisor  (EL2)\ndomains · stage-2 · vGIC", AVZ, 16)
p.box(420, 310, 260, 60, "Hardware (EL2/EL1)", HW, 16)
p.label(420, 390, 260, 110, "virt64_avz_defconfig\n\nAVZ (Agency VirtualiZer) runs\nat EL2 and boots a single guest\nat EL1 — Linux or a plain SO3.", 14)
# Hypervisor + capsules
p.box(750, 70, 320, 460, "SOO / capsule mode", CONT, 16, 1)
p.box(780, 120, 90, 60, "Linux\nagency\n(EL1)", USER, 14)
p.box(880, 120, 55, 60, "S3C\n#1", CAPS, 16)
p.box(945, 120, 55, 60, "S3C\n#2", CAPS, 16)
p.box(1010, 120, 40, 60, "…", CAPS, 16)
p.box(780, 200, 270, 80, "AVZ hypervisor  (EL2)\ncapsule scheduling\ngrant tables · vbstore", AVZ, 16)
p.box(780, 300, 270, 60, "Hardware", HW, 16)
p.label(780, 380, 270, 130, "SO3 capsules (S3C) run as\nlightweight EL1 guests beside a\nLinux agency.\n\nThe agency is built here via the\n:soo override; its SOO patches\nderive from a separate project.", 14)
pages.append(p)

# =========================================================================
# 2. System architecture (standalone)
# =========================================================================
p = Page("architecture")
p.label(0, 10, 1080, 30, "SO3 system architecture (standalone)", 20, True)
p.box(40, 60, 1000, 120, "User space  (EL0)", CONT, 16, 1)
for i, (lbl) in enumerate(["init.elf", "sh.elf", "ls / cat /\nmore / echo", "ping", "LVGL\ndemos", "MicroPython"]):
    p.box(70 + i*155, 100, 140, 60, lbl, USER, 16)
p.box(40, 210, 1000, 300, "Kernel space  (EL1)", CONT, 16, 1)
# syscall layer
p.box(70, 250, 940, 40, "System call interface  (svc → trap_handle → syscall_table)", NEUTRAL, 16)
subs = [
    ("Process & thread mgmt", "kernel/ — pcb, tcb,\nround-robin sched"),
    ("Memory mgmt", "mm/ — frame table,\nheap, MMU"),
    ("VFS", "fs/ — fat, devfs,\nELF loader"),
    ("IPC", "ipc/ — signals,\npipes, semaphores"),
    ("Networking", "net/ — lwIP\nsockets"),
]
for i, (t, d) in enumerate(subs):
    x = 70 + i*188
    p.box(x, 310, 175, 80, f"{t}\n\n{d}", KERNEL, 14)
p.box(70, 410, 940, 80, "Device model & drivers   (devices/)\n"
      "FDT probe · GIC (irq) · timer · pl011 serial · mmc · framebuffer (PL111) · "
      "input (PL050 kbd, absmouse) · net (smc911x) · ramdev",
      KERNEL, 14)
p.box(40, 540, 1000, 60, "Hardware  —  described by a Device Tree (dts/*.dtb)", HW, 16)
pages.append(p)

# =========================================================================
# 3. Boot flow
# =========================================================================
p = Page("boot")
p.label(0, 20, 1080, 30, "Boot flow (QEMU virt / U-Boot / FIT image)", 20, True)
EX = ARR + "exitX=1;exitY=0.5;entryX=0;entryY=0.5;"   # left→right
ub  = p.box(40, 90, 190, 64, "U-Boot\nloads the FIT (.itb)\nfrom the SD-card", NEUTRAL, 14)
fit = p.box(280, 90, 190, 64, "FIT image(s) (.itb)\nrendered from the BSP\nlayer (files/its/)", NEUTRAL, 14)
av  = p.box(610, 78, 190, 44, "AVZ (EL2)  avz_start()", AVZ, 14)
load= p.box(840, 78, 220, 44, "Loading Guest Domain\nstage-2 → guest", AVZ, 14)
so  = p.box(610, 168, 190, 44, "SO3 kernel (EL1)\n__start → kernel_start()", KERNEL, 14)
p.edge(ub, fit, "", EX)
p.edge(fit, av, "AVZ: 2 ITBs\n(guest-boot x0/x1)")
# "standalone": drop out of FIT's bottom and enter SO3's left, staying below the
# AVZ row so the arrow does not cross the AVZ box.
p.edge(fit, so, "standalone", ARR + "exitX=0.5;exitY=1;entryX=0;entryY=0.5;")
p.edge(av, load, "", EX)
# AVZ hands the loaded guest to SO3: drop below the AVZ row, then into SO3's right.
p.edge(load, so, "", ARR + "exitX=0.5;exitY=1;entryX=1;entryY=0.5;")
# kernel bring-up — snake: row A left→right, row B right→left
rowA = [
    ("memory_init()", "frame table + MMU"),
    ("devices_init()", "FDT probe, GIC,\ntimer, serial"),
    ("timer_init()\n+ calibrate_delay()", "tick source"),
    ("scheduler_init()", "round-robin\n+ local_irq_enable()"),
]
rowB = [
    ("rest_init()", "first kernel thread"),
    ("create_root_process()", "map __root_proc\n@ 0x1000 (EL0)"),
    ("execve(\"init.elf\")", "SO3 Init Program"),
    ("sh.elf", "so3%  shell prompt"),
]
ax = [40, 295, 550, 805]
ids_a = [p.box(ax[i], 280, 235, 80, f"{rowA[i][0]}\n{rowA[i][1]}", KERNEL, 16) for i in range(4)]
ids_b = [p.box(ax[3-i], 410, 235, 80, f"{rowB[i][0]}\n{rowB[i][1]}",
               USER if i >= 2 else KERNEL, 16) for i in range(4)]
p.edge(so, ids_a[0], "", ARR + "exitX=0.5;exitY=1;entryX=0.5;entryY=0;")
for i in range(3):
    p.edge(ids_a[i], ids_a[i+1], "", EX)
p.edge(ids_a[3], ids_b[0])                 # scheduler → rest_init (drop down)
for i in range(3):                          # rowB flows right→left
    p.edge(ids_b[i], ids_b[i+1], "",
           ARR + "exitX=0;exitY=0.5;entryX=1;entryY=0.5;")
pages.append(p)

# =========================================================================
# 4. ARM64 address space (standalone, EL1)
# =========================================================================
p = Page("memory")
p.label(0, 10, 1080, 30, "ARM64 virtual address space (standalone EL1)", 20, True)
# TTBR0 (user)
p.box(120, 70, 360, 500, "TTBR0_EL1  —  user space\n0x0000_0000_0000_0000 … 0x0000_FFFF_FFFF_FFFF", CONT, 12, 1)
p.box(150, 120, 300, 50, "0x1000  __root_proc trampoline\n(initial EL0 entry)", USER, 14)
p.box(150, 190, 300, 60, "ELF text / data / bss\n(per-process, from init.elf)", USER, 14)
p.box(150, 270, 300, 50, "heap  (brk, grows up)", USER, 14)
p.box(150, 470, 300, 50, "user stack  (grows down)", USER, 14)
p.label(150, 330, 300, 120, "Each process owns a private\nL0 page table (pcb->pgtable).\nThe MMU uses TTBR0_EL1 for\nthe low half (bits [47:0]).", 14)
# TTBR1 (kernel)
p.box(620, 70, 360, 500, "TTBR1_EL1  —  kernel space\nCONFIG_KERNEL_VADDR = 0xFFFF_8000_0000_0000", CONT, 12, 1)
p.box(650, 120, 300, 45, "0xFFFF_8000_0008_0000\nvectors + .head.text", KERNEL, 14)
p.box(650, 175, 300, 45, ".text / .data / .bss", KERNEL, 14)
p.box(650, 230, 300, 45, "per-CPU data (.percpu_data)", KERNEL, 14)
p.box(650, 285, 300, 45, "system page tables (idmap, linear)", KERNEL, 14)
p.box(650, 340, 300, 45, "kernel heap (CONFIG_HEAP_SIZE_MB)", KERNEL, 14)
p.box(650, 395, 300, 45, "per-CPU kernel stacks", KERNEL, 14)
p.label(650, 450, 300, 100, "__pa(v) = v - KERNEL_VADDR + phys_base\nKERNEL_VADDR differs per mode:\nstandalone 0xFFFF800000000000\nAVZ (EL2)  0x0000100000000000", 14)
pages.append(p)

# =========================================================================
# 5. Exception levels
# =========================================================================
p = Page("exception-levels")
p.label(0, 10, 1080, 30, "ARM64 exception levels: standalone vs. AVZ", 20, True)
# standalone
p.box(60, 70, 440, 420, "Standalone", CONT, 16, 1)
p.box(110, 120, 340, 60, "EL0 — user processes (init, shell, apps)", USER, 16)
p.box(110, 210, 340, 70, "EL1 — SO3 kernel\nVBAR_EL1 vectors · TTBR0/1_EL1", KERNEL, 16)
p.box(110, 320, 340, 60, "EL2 — unused", NONE, 16)
p.box(110, 400, 340, 50, "Hardware", HW, 16)
# AVZ
p.box(620, 70, 440, 420, "AVZ hypervisor", CONT, 16, 1)
p.box(670, 120, 160, 60, "EL0 — guest\nuser space", USER, 16)
p.box(850, 120, 160, 60, "EL0 — capsule\nuser space", CAPS, 16)
p.box(670, 200, 160, 60, "EL1 — guest\nSO3 kernel", USER, 16)
p.box(850, 200, 160, 60, "EL1 — capsule\nSO3 kernel", CAPS, 16)
p.box(670, 290, 340, 80, "EL2 — AVZ hypervisor\nVBAR_EL2 · VTTBR (stage-2)\nvGIC · HCR_EL2", AVZ, 16)
p.box(670, 400, 340, 50, "Hardware", HW, 16)
pages.append(p)

# =========================================================================
# 6. Syscall path
# =========================================================================
p = Page("syscall")
p.label(0, 10, 1080, 30, "System call path (AArch64)", 20, True)
a = p.box(60, 90, 200, 96, "EL0 user code\nx8 = syscall nr\nx0..x5 = args\nsvc #0", USER, 16)
b = p.box(320, 90, 220, 96, "VBAR_EL1 + 0x400\n“Lower EL / Synchronous”\nb el01_sync_handler", KERNEL, 16)
c = p.box(600, 90, 220, 96, "el01_sync_handler\nkernel_entry (save regs)\nprepare_to_enter_to_el1", KERNEL, 16)
d = p.box(880, 90, 180, 96, "trap_handle()\ndecode ESR_EL1\n(EC = SVC64?)", KERNEL, 16)
e = p.box(880, 230, 180, 96, "syscall_handle()\nbounds-check nr", KERNEL, 16)
f = p.box(600, 230, 220, 96, "syscall_table[nr]\n(generated from\nsyscall.tbl)", KERNEL, 16)
g = p.box(320, 230, 220, 96, "sys_xxx()\nSYSCALL_DEFINEn()", KERNEL, 16)
h = p.box(60, 230, 200, 96, "return value → x0\nkernel_exit · eret\n→ back to EL0", USER, 16)
for s, t in [(a,b),(b,c),(c,d),(d,e),(e,f),(f,g),(g,h)]:
    p.edge(s, t)
p.label(60, 360, 1000, 40, "syscall.tbl is processed by scripts/syscall_gen.sh into "
        "generated/syscall_table.h.in, giving the kernel and the MUSL libc a common ABI.", 14)
pages.append(p)

# =========================================================================
# 7. AVZ domains + vGIC
# =========================================================================
p = Page("avz")
p.label(0, 10, 1080, 30, "AVZ: domains, stage-2 isolation and vGIC injection", 20, True)
p.box(60, 70, 980, 230, "Guests (EL1) — isolated by per-domain stage-2 tables (VTTBR)", CONT, 16, 1)
ag = p.box(100, 120, 220, 150, "Agency  (DOMID_AGENCY)\nLinux — owns the devices\nvbstore server", USER, 14)
c1 = p.box(335, 120, 180, 150, "Capsule (S3C)  slot 2\nSO3 guest\nstage-2 isolated", CAPS, 14)
c2 = p.box(530, 120, 180, 150, "Capsule (S3C)  slot 3\nSO3 guest", CAPS, 14)
c3 = p.box(725, 120, 285, 150, "… up to MAX_S3C_DOMAINS\n(5 capsules) beside\nthe agency", CAPS, 14)
p.box(60, 330, 980, 170, "AVZ hypervisor  (EL2)", CONT, 16, 1)
p.box(100, 370, 190, 100, "Domain scheduler\nsched_agency\nsched_flip (capsules)\nper-CPU current_domain", AVZ, 14)
p.box(305, 370, 225, 100, "Hypercalls (hvc #0)\nconsole · event channels\ndomain control · grant tables", AVZ, 14)
p.box(545, 370, 230, 100, "vGIC\nphys IRQ → EL2 (HCR.IMO)\ninject via list registers (HW=1)\nmaintenance IRQ", AVZ, 14)
p.box(790, 370, 220, 100, "Grant tables &\nevent channels\ninter-domain memory\n+ signalling", AVZ, 14)
hw = p.box(60, 530, 980, 50, "Physical GIC · generic timer · UART · RAM", HW, 16)
pages.append(p)

# =========================================================================
# 8. Capsule split drivers / vbus / vbstore
# =========================================================================
p = Page("capsule")
p.label(0, 10, 1080, 30, "SO3 capsules: split drivers (vbus / vbstore)", 20, True)
# vbus split
p.box(60, 70, 470, 330, "Split-driver model (per device)", CONT, 16, 1)
be = p.box(95, 120, 160, 90, "Agency — backend\n(Linux, EL1)\nreal device", USER, 14)
fe = p.box(335, 120, 170, 90, "Capsule — frontend\n(S3C, EL1)\nvfbdev / vinput /\nvuart / vuihandler", CAPS, 14)
ring = p.box(160, 250, 250, 60, "shared ring + event channel\n(pages shared via grant tables)", NEUTRAL, 14)
p.edge(fe, be, "vbstore\nhandshake", ARR_D)
p.edge(fe, ring); p.edge(be, ring)
p.label(95, 330, 400, 50, "Each virtual device is split: the frontend in the\ncapsule talks to a backend in the Linux agency.", 14)
# vbstore
p.box(570, 70, 470, 330, "vbstore  (Xenstore-like config tree)", CONT, 16, 1)
p.box(605, 120, 400, 40, "device/<domID>/<dev>/state  →  Connected", WHITE, 14)
p.box(605, 175, 400, 40, "ring-ref · event-channel · protocol params", WHITE, 14)
p.label(605, 230, 400, 90, "Frontend and backend rendez-vous through the\nvbstore tree; the state machine drives probe(),\nconnect, suspend and resume of each device.", 14)
p.box(605, 330, 400, 50, "snapshot: AVZ_S3C_READ/WRITE_SNAPSHOT\nsave & restore a capsule's state", AVZ, 14)
pages.append(p)

# =========================================================================
# 9. Device / driver model
# =========================================================================
p = Page("device-model")
p.label(0, 10, 1080, 30, "Device & driver model (FDT-driven)", 20, True)
a = p.box(60, 90, 200, 96, "Device Tree blob\n(dts/*.dtb)\nloaded by U-Boot", NEUTRAL, 16)
b = p.box(320, 90, 200, 96, "parse_dtb()\nwalk FDT nodes\nmatch “compatible”", KERNEL, 16)
c = p.box(580, 90, 220, 96, "initcall sections\n(.initcall_driver_*\ncore / postcore)\nvia ll_entry", KERNEL, 16)
d = p.box(860, 90, 200, 96, "driver .init(dev,\nfdt_offset)\nprobe hardware", KERNEL, 16)
# 'e' sits directly under 'd' so the snake (a→b→c→d top, e→f→g bottom) turns
# with a clean vertical drop instead of an arrow cutting back across box 'c'.
e = p.box(850, 240, 220, 96, "devclass_register()\nclass + fops + id\n(fb, input, serial,\nnic, char, …)", KERNEL, 16)
f = p.box(320, 240, 220, 96, "devfs\nexposes /dev/<name>", KERNEL, 16)
g = p.box(60, 240, 200, 96, "User space\nopen(\"/dev/...\")\nread/write/ioctl", USER, 16)
for s, t in [(a,b),(b,c),(c,d)]:
    p.edge(s, t)
p.edge(d, e, "", ARR + "exitX=0.5;exitY=1;entryX=0.5;entryY=0;")   # vertical drop
p.edge(e, f, "", ARR + "exitX=0;exitY=0.5;entryX=1;entryY=0.5;")   # leftward
p.edge(f, g, "", ARR + "exitX=0;exitY=0.5;entryX=1;entryY=0.5;")   # leftward
p.label(60, 370, 1000, 60,
        "Drivers register with REGISTER_DRIVER_CORE / _POSTCORE, which place an initcall entry in a dedicated "
        "linker section. At boot parse_dtb() matches each FDT node's “compatible” string to a driver and calls its "
        "init() callback; the driver then publishes a devclass that VFS/devfs makes reachable as a /dev entry.", 14)
pages.append(p)

# =========================================================================
# 10. Infrabase build & deploy flow
# =========================================================================
p = Page("build")
p.label(0, 10, 1080, 30, "Infrabase build & deploy flow", 20, True)
# top row: config -> build.sh -> bitbake
cfg = p.box(40, 60, 300, 110,
            "build/conf/local.conf\n\nIB_PLATFORM · IB_CONFIG:so3\nIB_TARGET_ITS:so3 · IB_BOOT_CHAIN",
            NEUTRAL, 14)
bsh = p.box(390, 78, 230, 74, "scripts/build.sh\nbsp-so3   (recipe positional)", WHITE, 14)
bb  = p.box(670, 78, 230, 74, "bitbake\nmeta-* layers + recipes", AVZ, 14)
p.edge(cfg, bsh); p.edge(bsh, bb)
# recipes band
rec = p.box(40, 200, 1020, 175, "Recipes (meta-* layers)", CONT, 16, 1)
so3 = p.box(70, 245, 150, 110,
            "so3 kernel\n(meta-so3)\n\nIN-TREE\nKconfig + dts", KERNEL, 14)
avz = p.box(235, 245, 150, 110,
            "avz\n(meta-so3)\n\nfetched\n+ patched", AVZ, 14)
ub  = p.box(400, 245, 150, 110,
            "u-boot\n(meta-uboot)\n\nfetched\n+ patched", NEUTRAL, 14)
qm  = p.box(565, 245, 150, 110,
            "qemu\n(meta-qemu)\n\nfetched\n+ patched", NEUTRAL, 14)
usr = p.box(730, 245, 150, 110,
            "usr-so3\n(meta-usr)\n\nCMake + MUSL\n(meta-toolchain)", USER, 14)
rf  = p.box(895, 245, 145, 50, "rootfs-so3\n(meta-rootfs)", USER, 14)
bsp = p.box(895, 305, 145, 50, "bsp-so3\n(meta-bsp)", CAPS, 14)
# bitbake drops straight into the recipes band (connect to the band border,
# not to an individual recipe box, so the arrow doesn't cross any rectangle).
p.edge(bb, rec, "", ARR + "exitX=0.5;exitY=1;entryX=0.73;entryY=0;")
# bottom row: itb -> deploy -> run
itb = p.box(70, 420, 250, 64,
            "do_itb → render + mkimage\nfiles/its/*.its  →  images/*.itb", WHITE, 14)
dep = p.box(420, 420, 250, 64,
            "scripts/deploy.sh bsp-so3\n→ sdcard FAT  (+ flash0.img)", WHITE, 14)
run = p.box(770, 420, 270, 64,
            "scripts/st.sh  [-d]\n→ QEMU (EL1 / EL2 / ATF)", WHITE, 14)
# the band feeds do_itb: drop from the band's lower-left border straight down.
p.edge(rec, itb, "", ARR + "exitX=0.15;exitY=1;entryX=0.5;entryY=0;")
p.edge(itb, dep, "", ARR + "exitX=1;exitY=0.5;entryX=0;entryY=0.5;")
p.edge(dep, run, "", ARR + "exitX=1;exitY=0.5;entryX=0;entryY=0.5;")
p.label(40, 520, 1020, 70,
        "SO3 kernel and user space are built IN-TREE (committed sources). u-boot, qemu and avz are "
        "FETCHED and local edits are kept as patches: edit the source, then 'scripts/updiff.sh <recipe>' "
        "regenerates files/000N-*.patch (diff of S.pristine vs the working tree).", 14)
pages.append(p)

# =========================================================================
# 11. Display & input under QEMU
# =========================================================================
p = Page("io")
p.label(0, 10, 1080, 30, "Display & input under QEMU (virt machine + so3 patch)", 20, True)
# Guest column
p.box(40, 60, 320, 470, "SO3 guest  (EL0/EL1)", CONT, 16, 1)
gapp = p.box(55, 105, 290, 46, "LVGL app (slv) · fb_test", USER, 14)
gfb  = p.box(55, 165, 290, 60, "/dev/fb  —  PL111 driver\nmmap VRAM · ioctl HRES/VRES/BPP", KERNEL, 14)
gkb  = p.box(55, 240, 290, 46, "/dev/keyboard  —  PL050", KERNEL, 14)
gms  = p.box(55, 300, 290, 60, "/dev/mouse  —  so3,absmouse\nabsolute · polled · scaled to res", KERNEL, 14)
gcon = p.box(55, 375, 290, 60, "/dev/console  —  PL011\nCtrl-C → SIGINT to fg job (fg_pcb)", KERNEL, 14)
# QEMU column
p.box(390, 60, 340, 470, "QEMU “virt”  (so3 patch)", CONT, 16, 1)
qfb  = p.box(405, 105, 310, 60, "pl111 @ 0x08800000\nVRAM @ 0x30000000  (8 MB)", HW, 14)
qkb  = p.box(405, 180, 310, 46, "pl050 kbd @ 0x08801000  (SPI 11)", HW, 14)
qms  = p.box(405, 240, 310, 64, "so3.absmouse @ 0x08803000\nabsolute input handler (no grab)", HW, 14)
qmsd = p.box(405, 320, 310, 44, "pl050 mouse @ 0x08802000  (disabled)", NONE, 14)
quart= p.box(405, 380, 310, 46, "pl011 UART @ 0x09000000  (SPI 1)", HW, 14)
# Host column
p.box(760, 60, 320, 470, "Host  (scripts/st.sh -d)", CONT, 16, 1)
hwin = p.box(775, 110, 290, 80, "GTK window\n-display gtk,zoom-to-fit=off\nGDK_BACKEND=x11 (HiDPI/Wayland)", NEUTRAL, 14)
hptr = p.box(775, 250, 290, 64, "host pointer → absolute\n1:1 mapping, no grab / no warp", NEUTRAL, 14)
hterm= p.box(775, 380, 290, 46, "terminal (-serial mon:stdio)", NEUTRAL, 14)
# wiring
EXr = ARR + "exitX=1;exitY=0.5;entryX=0;entryY=0.5;"
p.edge(gfb, qfb, "", EXr); p.edge(qfb, hwin, "", EXr)
p.edge(gkb, qkb, "", EXr)
p.edge(gms, qms, "", EXr); p.edge(qms, hptr, "", EXr)
p.edge(gcon, quart, "", EXr); p.edge(quart, hterm, "", EXr)
pages.append(p)

# ---- emit ----------------------------------------------------------------
out = ('<mxfile host="so3-doc" type="device">'
       + "".join(pg.xml() for pg in pages) + "</mxfile>")
with open("so3.drawio", "w") as fh:
    fh.write(out)
print(f"wrote so3.drawio with {len(pages)} pages:")
for i, pg in enumerate(pages):
    print(f"  [{i}] {pg.name}")
