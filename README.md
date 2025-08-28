# High-Speed File Downloader for ESP32

## Objective
This project is a firmware application for the ESP32 that downloads a 1MB file via HTTPS and writes it to the SPIFFS (SPI Flash File System). The performance target was to achieve a throughput of at least **400 KB/s**.

---

## Firmware Architecture
The final firmware uses a dual-core, producer-consumer architecture to maximize performance by running network and file operations in parallel.

* **Producer Task (Core 0):** Dedicated solely to downloading data from the specified URL using the ESP-IDF HTTP Client.
* **Consumer Task (Core 1):** Dedicated solely to writing the downloaded data to a file on the SPIFFS partition.
* **Data Transfer:** A FreeRTOS Stream Buffer is used as an efficient, thread-safe pipe to transfer data from the producer to the consumer, decoupling the two tasks.

---

## Key Optimizations Implemented
To meet the high-throughput requirement, a series of software and system optimizations were implemented:

#### 1. System & Hardware Configuration
* **CPU Frequency:** Set to the maximum of **240 MHz** for faster processing of network data and HTTPS decryption.
* **Wi-Fi Power Save Mode:** Disabled to ensure the Wi-Fi radio is always active for maximum throughput and minimal latency.
* **Main Task Stack Size:** Increased to **8KB** to ensure stability during the initialization of multiple complex subsystems.

#### 2. Memory & Buffer Management
* **Aggressive File I/O Buffering:** A large **32KB** file I/O buffer was used with `setvbuf`, and data was written in large **16KB** chunks to maximize the efficiency of the underlying flash memory.
* **Large Network & Stream Buffers:** A **24KB** buffer for the HTTP client and a **36KB** stream buffer were used to handle large segments of data efficiently and reduce system call overhead.

#### 3. Library & API Usage
* **HTTP Keep-Alive:** This was enabled in the HTTP client configuration to reuse the secure TLS connection, dramatically reducing latency for sustained data transfer.
* **Robust Wi-Fi Connection:** A FreeRTOS Event Group was used to reliably wait for a confirmed IP address before starting the download, eliminating potential race conditions.
* **System-Managed File Buffer:** The `setvbuf` function was configured to let the system manage memory, which proved to be more stable under high load.

---

## How to Compile and Flash

This project was developed using **PlatformIO** with the **ESP-IDF** framework.

1.  Clone the repository.
2.  Open the project in Visual Studio Code with the PlatformIO extension.
3.  Connect an ESP32 Dev Kit to your computer.
4.  Build and Upload the project using the PlatformIO controls.

---

## Performance Testing & Validation
The final optimized firmware was deployed to an ESP32 DevKitV1 for performance validation. The application successfully met and exceeded the performance target.

* **Achieved Throughput:** **582.45 KB/s**
* **Conclusion:** The dual-core architecture combined with aggressive buffering and system-level optimizations allowed the firmware to successfully achieve the high-speed download and write requirement.

#### Final Log Output:
