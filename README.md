https://github.com/pantaluna/support_esp_espidf2

nvs_set_blob() hangs the MCU when +-91% of the total space of a 512KB data-nvs partition is used.

## Environment
- Development Kit:      [Adafruit HUZZAH32]
- Core (if using chip or module): [ESP-WROOM32]
- IDF version (commit id): ESP-IDF v3.2-dev-518-g020ade65 Aug18,2018
- Development Env:      [Eclipse]
- Operating System:     [Windows]
- Power Supply:         [USB]

## Problem Description
nvs_set_blob() hangs the MCU when +-91% of the total space of a 512KB data-nvs partition is used.

Hanging means that the call to nvs_set_blob() is executing (according to the logs) but it never returns.

The Memory Stats just before the failing method call:
```
I (51352) mjd:   ESP free HEAP space: 263932 bytes | FreeRTOS free STACK space (calling task): 20992 bytes
```

The NVS Stats just before the failing method call:
```
myapp: NVS STATS partition nvsmeteohub: used_entries=14734  free_entries=1394  total_entries=16128
```

In this configuration the hangup always happens when writing the 509th key-value pair to NVS.

This is a data-nvs partition of 512KB named "nvsmeteohub". The blob key length is always 11. The blob value length is always 860 bytes. Each blob value uses 29 entries in NVS.

@note This reproducible case contains a loop to write about 550 blob key-values (knowing it will fail at the 509th). In production that loop is not there of course; the application will add only one data record during each wakeup cycle and in exceptional situations it will go up to the 509th after a few weeks.

### Expected Behavior
nvs_set_blob() should work fine until not enough space/entries are available in the partition namespace for that operation. If no more space is available then it should return the existing error ESP_ERR_NVS_NOT_ENOUGH_SPACE "there is not enough space in the underlying storage to save the value".

### Actual Behavior
The call to nvs_set_blob() hangs the system and never returns. Also no CPU Watchdog messages.

### Workaround
None.

### Steps to repropduce
@important Erase the flash first.

No wiring needed.

```
cd ~
git clone --recursive https://github.com/pantaluna/support_esp_espidf2.git
cd  support_esp_espidf2
make erase_flash; make flash monitor
```

### Code to reproduce this issue
https://github.com/pantaluna/support_esp_espidf2


## Debug Logs
```
...
...
***SECTION: Save (1) data record struct to flash***
D (51242) mjd: mjd_log_memory_statistics()
D (51252) mjd: mjd_get_memory_statistics()
I (51252) mjd:   ESP free HEAP space: 263932 bytes | FreeRTOS free STACK space (calling task): 20992 bytes
D (51262) myapp: exec nvs_open_from_partition()
D (51272) nvs: nvs_open_from_partition uploadqueue 1
D (51272) myapp: _log_nvs_stats()
I (51282) myapp: NVS STATS partition nvsmeteohub: used_entries=14705  free_entries=1423  total_entries=16128
D (51292) nvs: nvs_get lastrecordid 4
D (51292) myapp: NVS key=lastrecordid val=507
I (51292) myapp: _save_data_record_to_flash(): Storing the data record in the NVS nvs_key=record00508 ...
D (51302) nvs: nvs_set_blob record00508 860
D (51322) nvs: nvs_set lastrecordid 4 508
D (51322) myapp: exec nvs_commit()
D (51322) myapp: _log_nvs_stats()
I (51322) myapp: NVS STATS partition nvsmeteohub: used_entries=14734  free_entries=1394  total_entries=16128
D (51332) nvs: nvs_close 508
D (51332) myapp: _save_data_record_to_flash()

I (51342) myapp: ***SECTION: Save (1) data record struct to flash***
D (51342) mjd: mjd_log_memory_statistics()
D (51352) mjd: mjd_get_memory_statistics()
I (51352) mjd:   ESP free HEAP space: 263932 bytes | FreeRTOS free STACK space (calling task): 20992 bytes
D (51362) myapp: exec nvs_open_from_partition()
D (51372) nvs: nvs_open_from_partition uploadqueue 1
D (51372) myapp: _log_nvs_stats()
I (51382) myapp: NVS STATS partition nvsmeteohub: used_entries=14734  free_entries=1394  total_entries=16128
D (51392) nvs: nvs_get lastrecordid 4
D (51392) myapp: NVS key=lastrecordid val=508
I (51392) myapp: _save_data_record_to_flash(): Storing the data record in the NVS nvs_key=record00509 ...
D (51402) nvs: nvs_set_blob record00509 860
```


##################################################################################################
# APPENDICES

# 1. SOP for upload to GITHUB
https://github.com/pantaluna/support_esp_espidf2

## 1.a: BROWSER: create github public repo support_esp_espidf2 of pantaluna at Github.com

## 1.b: MSYS2 git
```
git config --system --unset credential.helper
git config credential.helper store
git config --list

cd  /c/myiot/esp/support_esp_espidf2
git init
git add .
git commit -m "First commit"
git remote add origin https://github.com/pantaluna/support_esp_espidf2.git

git push --set-upstream origin master

git remote show origin
git status

git tag --annotate v1.0 --message "First release"
git push origin --tags

```

# 2. SOP for source updates
```
cd  /c/myiot/esp/support_esp_espidf2
git add .
git commit -m "Updated after testing xxx"

git push --set-upstream origin master

git status
```
