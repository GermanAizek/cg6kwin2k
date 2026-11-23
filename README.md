# cg6kwin2k

cg6kwin2k (cg6kwin2k.sys / CG6K) - kernel mode driver used by NMS Communications / Sangoma Technologies CG 6000 PCI telecom hardware boards.

My opensource driver does not use the original source codes and is a reverse-engineered driver using [Ghidra](https://en.wikipedia.org/wiki/Ghidra) and [Radare2](https://en.wikipedia.org/wiki/Radare2). The rights have not been violated, and the new driver cannot be signed due to the EV certificate. I hope that the vulnerabilities can be closed using my security patches.

The main goal of the project is to recreate a secure driver with fixed vulnerabilities and errors made by past developers.

### Functions

Driver cg6kwin2k.sys allows utility and management software to interact with system hardware and CG 6000 boards directly:

- Direct physical memory mapping, reading, and writing byte-by-byte (IOCTL 0x80012F18, 0x80012F1C)
- Word-by-word and dword-by-word physical memory read and write (IOCTL 0x80012EE4, 0x80012EE8, 0x80012EF0, 0x80012EF4, 0x80012F10, 0x80012F14)
- Direct x86 hardware I/O port input and output (IOCTL 0x80012EF8, 0x80012EFC, 0x80012F28, 0x80012F2C, 0x80012F30, 0x80012F34)
- PCI configuration space inspection and modification (IOCTL 0x80012EE0, 0x80012F00, 0x80012F04, 0x80012F08, 0x80012F0C, 0x80012F20)
- Board status, firmware, diagnostics, and version querying (IOCTL 0x002A2800, 0x80012024, 0x80012F3C)

Fixed vulnerabilities in opensource driver cg6kwin2k.sys:

- [ ] (LOLDrivers: cg6kwin2k.sys)[https://www.loldrivers.io/drivers/fda3ff67-12dc-488c-a4c2-603f5bf420bd/]
  - Sangoma cg6kwin2k versions before 2.1.7.0 expose IOCTLs without sufficient access control (CVE-2024-29216). A low-privilege user can perform I/O against arbitrary hardware ports or physical addresses, enabling firmware erasure or modification.

Fixed no publish vulnerabilities and bugs which contributors to this project found:

- [ ] Device deletion bypassed on symbolic link deletion failure ([view code](main.c))
- [ ] Arbitrary physical memory read and write in commands 0x80012F18, 0x80012F1C ([view code](mem.c))
- [ ] Arbitrary physical memory word and dword access in commands 0x80012EE4, 0x80012EE8, 0x80012EF0, 0x80012EF4, 0x80012F10, 0x80012F14 ([view code](mem.c))
- [ ] Unfiltered write to critical x86 I/O ports and controllers in commands 0x80012EF8, 0x80012EFC, 0x80012F28, 0x80012F2C, 0x80012F30, 0x80012F34 ([view code](io.c))
- [ ] Arbitrary PCI configuration space modification in commands 0x80012EE0, 0x80012F00, 0x80012F04, 0x80012F08, 0x80012F0C, 0x80012F20 ([view code](pci.c))
- [ ] Device creation with weak permissions ([view code](main.c))

Authors:

- 2026 - Herman Semenoff <<GermanAizek@yandex.ru>> ([germanaizek.github.io](https://germanaizek.github.io))
- NMS Communications / Sangoma Technologies <<support@sangoma.com>>
