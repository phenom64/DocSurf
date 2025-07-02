# DocSurf

<a href="https://syndromatic.com"><img src="src/gfx/Old Syn Logo.png" alt="DocSurf Logo" width="100"/></a>

**DocSurf** is the default file manager of **SynOS**, built as a modern, beautiful, and fast Qt-based alternative to Dolphin. Designed with the flair of the Atmo design language and a commitment to elegant human-centric design, DocSurf aims to make file navigation not only powerful, but also deeply enjoyable.

Currently in **very early alpha**, DocSurf is a **work in progress** and is actively being developed to reflect the design philosophy of Syndromatic: clarity, charm, and control.

---

## ✨ Features

- **Qt-based cross-platform architecture**  
  Leveraging the power of Qt and KDE Frameworks for native performance and deep integration.
  
- **Cover Flow-inspired file browsing** *(WIP)*  
  A visually immersive file preview mode with a reflective dimensional carousel of icons.
  
- **Custom “About Document Surfer” Window**  
  Unique branding and about page tailored for SynOS.
  
- **Skeuomorphic Icons and Styling**  
  Inspired by the visual DNA of early-2000s computing, retrofuturism meets utility.

- **Lightweight & Fast**  
  Minimal system resource usage without compromising on features.

- **Intended as the de facto file manager for SynOS**  
  Replaces Dolphin in SynOS while maintaining full compatibility with KDE frameworks.

---

## 💾 Installation

Installation has been tested on SynOS Canora (Beta 1) and should work on all Debian/Ubuntu-based Linux distributions with a Qt 5 based desktop environment.

```bash
git clone https://github.com/phenom64/DocSurf.git
cd DocSurf
mkdir build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=/usr -DQT5BUILD=ON
make -j$(nproc)
sudo make install
```

---

## 🚧 Project Status
    ⚠️ DocSurf is currently in very early alpha.
    While it compiles and runs, many components such as toolbar UI,
    full menu integration, and advanced view modes (like Cover Flow) are incomplete or experimental. Retina Display support is also currently lacklustre.

Expect bugs, unfinished features, and rough edges. We welcome collaboration and testing, but this is not ready for production use.

---

## 🙏 Credits
DocSurf is a modified and actively maintained fork of <a href="https://sourceforge.net/projects/kdfm/">KDFM</a>, an open-source file manager originally developed by Robert Metsaranta. Immense gratitude to Robert for his excellent foundational work.

---

## 🧠 Philosophy & Goals
DocSurf is more than just a file manager. It embodies a design rebellion against bland utility. We believe your file explorer should spark joy, nostalgia, and control without sacrificing power.
As a keystone of the SynOS Atmo design language ecosystem, DocSurf will continue to evolve toward a full-featured, skeuomorphic, and deeply human file browsing experience.
