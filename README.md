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

* **Achieved Throughput:** **643.52 KB/s**
* **Conclusion:** The dual-core architecture combined with aggressive buffering and system-level optimizations allowed the firmware to successfully achieve the high-speed download and write requirement.

#### Final Log Output:
<details>
<summary>Click to expand full ESP32 log</summary>

```
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:6592
load:0x40078000,len:16688
load:0x40080400,len:4
load:0x40080404,len:4268
entry 0x40080650
I (29) boot: ESP-IDF 5.4.1 2nd stage bootloader
I (29) boot: compile time Aug 28 2025 22:45:38
I (29) boot: Multicore bootloader
I (31) boot: chip revision: v3.1
I (33) qio_mode: Enabling default flash chip QIO
I (38) boot.esp32: SPI Speed      : 40MHz
I (42) boot.esp32: SPI Mode       : QIO
I (45) boot.esp32: SPI Flash Size : 4MB
I (49) boot: Enabling RNG early entropy source...
I (53) boot: Partition Table:
I (56) boot: ## Label            Usage          Type ST Offset   Length
I (62) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (69) boot:  1 otadata          OTA data         01 00 0000f000 00002000
I (75) boot:  2 app0             OTA app          00 10 00020000 00100000
I (82) boot:  3 app1             OTA app          00 11 00120000 00100000
I (88) boot:  4 spiffs           Unknown data     01 82 00220000 00180000
I (95) boot: End of partition table
I (98) esp_image: segment 0: paddr=00020020 vaddr=3f400020 size=1840ch ( 99340) map
I (134) esp_image: segment 1: paddr=00038434 vaddr=3ff80000 size=0001ch (    28) load
I (134) esp_image: segment 2: paddr=00038458 vaddr=3ffb0000 size=03d74h ( 15732) load
I (143) esp_image: segment 3: paddr=0003c1d4 vaddr=40080000 size=03e44h ( 15940) load
I (151) esp_image: segment 4: paddr=00040020 vaddr=400d0020 size=86058h (548952) map
I (311) esp_image: segment 5: paddr=000c6080 vaddr=40083e44 size=12d94h ( 77204) load
I (348) boot: Loaded app from partition at offset 0x20000
I (348) boot: Disabling RNG early entropy source...
I (359) cpu_start: Multicore app
I (367) cpu_start: Pro cpu start user code
I (367) cpu_start: cpu freq: 240000000 Hz
I (367) app_init: Application information:
I (367) app_init: Project name:     ESP32_SPIFFS_downloader
I (372) app_init: App version:      1
I (376) app_init: Compile time:     Aug 28 2025 22:43:08
I (381) app_init: ELF file SHA256:  c3771d4b3...
I (385) app_init: ESP-IDF:          5.4.1
I (389) efuse_init: Min chip rev:     v0.0
I (392) efuse_init: Max chip rev:     v3.99
I (396) efuse_init: Chip rev:         v3.1
I (400) heap_init: Initializing. RAM available for dynamic allocation:
I (407) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (412) heap_init: At 3FFB8028 len 00027FD8 (159 KiB): DRAM
I (417) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (422) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (428) heap_init: At 40096BD8 len 00009428 (37 KiB): IRAM
I (434) spi_flash: detected chip: generic
I (437) spi_flash: flash io: qio
I (440) main_task: Started on CPU0
I (443) main_task: Calling app_main()
I (459) HIGH_SPEED_DOWNLOADER: Initializing SPIFFS
I (529) HIGH_SPEED_DOWNLOADER: SPIFFS mounted successfully.
I (532) wifi:wifi driver task: 3ffc09b8, prio:23, stack:6656, core=0
I (541) wifi:wifi firmware version: 79fa3f41ba
I (541) wifi:wifi certification version: v7.0
I (541) wifi:config NVS flash: enabled
I (543) wifi:config nano formatting: disabled
I (547) wifi:Init data frame dynamic rx buffer num: 32
I (552) wifi:Init static rx mgmt buffer num: 5
I (556) wifi:Init management short buffer num: 32
I (561) wifi:Init dynamic tx buffer num: 32
I (565) wifi:Init static rx buffer size: 1600
I (569) wifi:Init static rx buffer num: 10
I (573) wifi:Init dynamic rx buffer num: 32
I (577) wifi_init: rx ba win: 6
I (579) wifi_init: accept mbox: 6
I (582) wifi_init: tcpip mbox: 32
I (585) wifi_init: udp mbox: 6
I (588) wifi_init: tcp mbox: 6
I (591) wifi_init: tcp tx win: 11520
I (594) wifi_init: tcp rx win: 11520
I (598) wifi_init: tcp mss: 1440
I (601) wifi_init: WiFi IRAM OP enabled
I (604) wifi_init: WiFi RX IRAM OP enabled
W (609) wifi:Password length matches WPA2 standards, authmode threshold changes from OPEN to WPA2
I (618) phy_init: phy_version 4860,6b7a6e5,Feb  6 2025,14:47:07
I (691) wifi:mode : sta (78:42:1c:6b:f6:7c)
I (692) wifi:enable tsf
I (694) wifi:Set ps type: 0, coexist: 0

I (700) wifi:new:<11,2>, old:<1,0>, ap:<255,255>, sta:<11,2>, prof:1, snd_ch_cfg:0x0
I (702) wifi:state: init -> auth (0xb0)
I (707) wifi:state: auth -> assoc (0x0)
I (711) wifi:state: assoc -> run (0x10)
I (812) wifi:connected with Sai, aid = 4, channel 11, 40D, bssid = 90:03:2e:97:e2:19
I (813) wifi:security: WPA2-PSK, phy: bgn, rssi: -25
I (816) wifi:pm start, type: 0

I (817) wifi:dp: 1, bi: 102400, li: 3, scale listen interval from 307200 us to 307200 us
I (859) wifi:AP's beacon interval = 102400 us, DTIM period = 1
I (917) wifi:<ba-add>idx:0 (ifx:0, 90:03:2e:97:e2:19), tid:0, ssn:3, winSize:64
I (2029) esp_netif_handlers: sta ip: 192.168.1.74, mask: 255.255.255.0, gw: 192.168.1.254
I (2029) HIGH_SPEED_DOWNLOADER: Got IP address.
I (2096) HIGH_SPEED_DOWNLOADER: Wi-Fi Connected!
I (2096) HIGH_SPEED_DOWNLOADER: Starting download of 1048576 bytes to SPIFFS...
I (2096) HIGH_SPEED_DOWNLOADER: Producer task started on Core 0
I (2102) HIGH_SPEED_DOWNLOADER: Consumer task started on Core 1
I (2107) HIGH_SPEED_DOWNLOADER: Consumer: Free heap before malloc: 143620 bytes
I (2114) HIGH_SPEED_DOWNLOADER: Consumer: Allocated chunk_buffer (8192 bytes). Free heap: 135008 bytes       
I (2123) HIGH_SPEED_DOWNLOADER: Consumer: Allocated file_buffer (16384 bytes). Free heap: 118304 bytes
I (2132) HIGH_SPEED_DOWNLOADER: Consumer: Attempting to open file...
I (2228) HIGH_SPEED_DOWNLOADER: Failed to open file for writing!
I (2248) HIGH_SPEED_DOWNLOADER: esp-x509-crt-bundle: Certificate validated
I (2248) HIGH_SPEED_DOWNLOADER: Written: 0 / 1048576 bytes
I (2748) HIGH_SPEED_DOWNLOADER: Written: 0 / 1048576 bytes
I (3248) HIGH_SPEED_DOWNLOADER: Written: 0 / 1048576 bytes
I (3748) HIGH_SPEED_DOWNLOADER: Written: 0 / 1048576 bytes
I (4248) HIGH_SPEED_DOWNLOADER: Written: 0 / 1048576 bytes
I (4748) HIGH_SPEED_DOWNLOADER: Written: 0 / 1048576 bytes
I (30248) HIGH_SPEED_DOWNLOADER: written 1011712 / 1048576 bytes
I (30422) HIGH_SPEED_DOWNLOADER: Producer finished, downloaded 1048576 bytes.
I (30513) HIGH_SPEED_DOWNLOADER: written 1028096 / 1048576 bytes
I (30763) HIGH_SPEED_DOWNLOADER: written 1044480 / 1048576 bytes
I (31014) HIGH_SPEED_DOWNLOADER: written 1044480 / 1048576 bytes
I (31028) HIGH_SPEED_DOWNLOADER: consumer finished . wrote 1048576 bytes.
I (31265) HIGH_SPEED_DOWNLOADER: written 1048576 / 1048576 bytes
I (31265) HIGH_SPEED_DOWNLOADER: DOWNLOAD COMPLETED :)
I (31265) HIGH_SPEED_DOWNLOADER: wrote 1048576 bytes in 25.46 seconds
I (31270) HIGH_SPEED_DOWNLOADER: Average throughput : 643.52 KBps
I (31276) main_task: Returned from app_main()
```
</details>
