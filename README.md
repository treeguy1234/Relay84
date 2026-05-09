# TI-84 Plus CE Messaging System

A peer-to-peer messaging system for TI-84 Plus CE calculators, built with the CE C Toolchain and ESP32-S3 boards communicating over ESP-NOW.

---

## How It Works

Each calculator connects via USB to an ESP32-S3. When a message is sent from the calculator, the ESP32-S3 broadcasts it over **ESP-NOW** — a lightweight Wi-Fi protocol by Espressif — to all other ESP32-S3 boards registered by MAC address. Those boards forward the message to their connected calculators over USB.

```
[TI-84 CE] <--USB--> [ESP32-S3] <--ESP-NOW--> [ESP32-S3] <--USB--> [TI-84 CE]
```

---

## Requirements

### Hardware
- One TI-84 Plus CE per user
- One ESP32-S3 board per user (tested on ESP32-S3-DevKitM-1)
- USB-Mini cable for each calculator-to-ESP32 connection
- USB-C cable for ESP32-to-power-supply

### Software
- [CE C/C++ Toolchain](https://github.com/CE-Programming/toolchain)
- [PlatformIO](https://platformio.org/)
- [Visual Studio Code](https://code.visualstudio.com/)
- A jailbreak program if your calculator's OS version is **5.5.1 or higher** ([Cesium Shell](https://www.ticalc.org/archives/files/fileinfo/465/46574.html) and [arTIfiCE Shell](https://yvantt.github.io/arTIfiCE/))

---

## Project Structure

```
TI-84-Plus-CE-Messaging-System/
├── calc/        # CE C Toolchain source for the calculator client
├── esp32/       # PlatformIO source for the ESP32-S3 firmware
└── pictures/    # Picture references for how to set up. Primarily used as a picture hosting folder for the main .md files
```

---

## Getting Started

### 1. Flash the ESP32-S3

Open `esp32/` in PlatformIO. Before building, edit the MAC address table in `main.c` to include the MAC addresses of every ESP32-S3 in your group:

```c
uint8_t peerMACAdresses[][6] = {
    {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX},
};
```

You can find an ESP32's MAC address by flashing it and reading the boot log output over serial. Build and flash to each board.

### 2. Build the Calculator Program

Follow the [CE C Toolchain setup guide](https://ce-programming.github.io/toolchain/static/setup.html) to install the toolchain, then build the calculator source from the `calculator/` directory. Transfer the resulting `.8xp` file in the `bin` folder to your TI-84 Plus CE using [TI Connect CE](https://education.ti.com/en/products/computer-software/ti-connect-ce-sw) or [TILP](http://lpg.ticalc.org/prj_tilp/).

> **OS 5.5.1 or higher?** You will need to install a jailbreak before running unsigned programs. See [JAILBREAK.md]([https://tiplanet.org/](https://github.com/treeguy1234/TI-84-Plus-CE-Messaging-System/blob/main/JAILBREAK.md)) for instructions.

### 3. Connect and Chat

Plug each calculator into its paired ESP32-S3 via USB-C. Launch the messaging program on the calculator. Messages will be sent to all other calculators in the group in real time.

---

## Versioning

| Component | Version |
|---|---|
| ESP32-S3 Firmware | 0.5 |
| Calculator Client | 0.2 |

---

## Contributing

Contributions are welcome. Please open an issue before submitting a pull request.

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Commit your changes (`git commit -m 'Add your feature'`)
4. Push to the branch (`git push origin feature/your-feature`)
5. Open a pull request

---

## License

This project is open source and under the MIT License.
