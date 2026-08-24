# device_nothing_phone1

Device tree for the **Nothing Phone (1)** (codename `spacewar`), built for [Neoteric OS](https://github.com/Neoteric-OS) — a CLO/AOSP-based Android 16 custom ROM.

## Device specs

| | |
|---|---|
| Device | Nothing Phone (1) |
| Codename | `spacewar` |
| Model | A063 |
| Release date | July 2022 |
| SoC | Qualcomm Snapdragon 778G+ (SM7325 "Lahaina") |
| CPU | Octa-core (1x2.4 GHz Kryo 670 Prime + 3x2.2 GHz Kryo 670 Gold + 4x1.9 GHz Kryo 670 Silver) |
| GPU | Adreno 642L |
| RAM | 8 GB / 12 GB |
| Storage | 128 GB / 256 GB (UFS 3.1) |
| Display | 6.55" FHD+ AMOLED, 120 Hz |
| Battery | 4500 mAh, 33W wired / 15W wireless / 5W reverse wireless |
| Rear camera | 50 MP wide + 50 MP ultrawide |
| Front camera | 16 MP |
| Glyph Interface | Yes |

## Build info

| | |
|---|---|
| Product name | `phone1` |
| Lunch target | `phone1-userdebug` |
| ROM base | Neoteric OS (`bka` branch, Android 16 / "Baklava") |
| Kernel | [android_kernel_nothing_sm7325](https://github.com/itsyvintsyspider/android_kernel_nothing_sm7325) — see kernel README for details |
| Vendor tree | [proprietary_vendor_nothing_phone1](https://github.com/itsyvintsyspider/proprietary_vendor_nothing_phone1) |
| Hardware tree | [android_hardware_nothing](https://github.com/itsyvintsyspider/android_hardware_nothing) |
| Stock blob source | Nothing OS `Spacewar-V3.2-260618` (final EOL build) |

## Highlights

- **BORE-scheduled, EROFS-compressed kernel** — see kernel README.
- **Glyph Interface via NGlyphs** — [NGlyphs](https://github.com/ang3lo-azevedo/android_hardware_nothing) + matching `GlyphAdapter`, sourced from [ang3lo-azevedo](https://github.com/ang3lo-azevedo)'s tree, replacing ParanoidGlyph. Audio-glyph sync, ringtone visualizer, essential lights, recording indicator LED, sleep mode, style selection — built as a system app, no root dependency.
- **Full GNSS** — complete HIDL 2.1 blob set, XTRA/`xtra-daemon` corrected.
- **Working fingerprint (UDFPS)**, Nothing Camera (Night Mode broken atm), Dolby Atmos, torch strength control, per-app haptics tuning.
- **AVB / release-key signing** — proper verified boot chain, not stock's disabled-verification test keys.

## Known issues / in progress

- **PowerShare (wireless reverse charging)** — service package (`vendor.aospa.powershare-service`) and kernel driver are present, but no Settings UI entry currently surfaces the toggle. Under investigation.

## Building

```bash
source build/envsetup.sh
lunch phone1-userdebug
m updatepackage
```

## Credits

- [pa-xe](https://github.com/pa-xe) — original `bka`-native device/vendor tree base
- [Willay24](https://github.com/Willay24) — kernel debugging leads, kernel guidance
- [Nyxalune](https://github.com/Nyxalune) — debugging leads
- [ang3lo-azevedo](https://github.com/ang3lo-azevedo) — NGlyphs, GlyphAdapter
- [Hellboy](https://github.com/Hellboy017) — Pong reference for Neoteric OS
- Neoteric-OS — ROM base
- LineageOS — original Spacewar device tree reference

## Maintainer

[itsyvintsyspider](https://github.com/itsyvintsyspider)
