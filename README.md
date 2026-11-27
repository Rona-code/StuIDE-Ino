# StuIDE-Ino

A lightweight, fast, cross-platform Integrated Development Environment (IDE) built for the Arduino ecosystem, written in C++ with **Qt 6** and powered under the hood by **arduino-cli**.

---

## ✨ Key Features

* **Modern & Clean Interface**: Native dark theme, distraction-free workspace focused on your code, and an integrated output console.
* **Automated Core & Tool Management**: Automatic background download and installation of `arduino-cli`, board indexes, and cores (e.g., `arduino:avr`) over HTTPS/TLS.
* **Automatic Board Detection**: Serial port detection and quick target board selection.
* **Compile & Upload**: Verify sketches (`.ino`) and upload directly to your Arduino board.
* **Cross-Platform**: Ready-to-use binaries for **Windows (x64)**, **Linux (x64)**, and **Linux ARM64 (Raspberry Pi)**.
* **Multilingual**: Interface available in English and French.

---

## 📦 Download & Installation

Head over to the [**Releases**](../../releases) page to download the package for your system:

| Platform | Architecture | Format | Installation |
| :--- | :--- | :--- | :--- |
| **Windows** | x64 | `StuIDE-Ino_Setup_1.0.0.exe` | Run the installer and follow the setup wizard. |
| **Linux** | x86_64 | `StuIDE-Ino-1.0.0-x64.AppImage` | `chmod +x StuIDE-Ino-1.0.0-x64.AppImage && ./StuIDE-Ino-1.0.0-x64.AppImage` |
| **Raspberry Pi / Linux** | ARM64 (`aarch64`) | `StuIDE-Ino-1.0.0-aarch64.AppImage` | `chmod +x StuIDE-Ino-1.0.0-aarch64.AppImage && ./StuIDE-Ino-1.0.0-aarch64.AppImage` |

---

## 🚀 Quick Start

1. Launch **StuIDE-Ino**.
2. On first launch, the IDE automatically downloads and configures `arduino-cli` along with the board index.
3. Connect your Arduino board via USB and select it from the top toolbar.
4. Write your code inside `setup()` and `loop()`, then click **Verify** (`✓`) or **Upload** (`→`).

---

## 📄 License

This project is licensed under the **MIT** License. See the [LICENSE](LICENSE.txt) file for details.
