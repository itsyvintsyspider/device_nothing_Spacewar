# Neoteric OS 4.0 — Build 230826 (User)

Custom Android build for the Nothing Phone (1), self-signed, KernelSU root.

## Highlights

- **KernelSU root** — root access without sacrificing a `user` build's security posture. No `userdebug`, no `adb root`, no weakened `ro.secure`.
- **WiFi, cellular, and hardware acceleration confirmed working** on real device testing prior to this release.
- **`user` build variant** — production build type, stricter verification, smaller attack surface than `userdebug`.

---

## ⚠️ Bootloader relocking support has been dropped

Previous builds supported flashing a custom AVB key and relocking the bootloader for verified boot. **This is no longer supported or recommended, starting with this build.**

In practice, relocking with a self-signed key never provided meaningful platform integrity by default — the only real-world benefit it delivered was Widevine L1 playback eligibility, and that came at the cost of a much harder recovery path if anything went wrong. That tradeoff isn't worth it, so relocking support has been removed going forward.

**If you have a custom AVB key flashed from a previous build, remove it:**

```
fastboot erase avb_custom_key
```

Run this while unlocked, before updating to this build. Leaving a stale custom key flashed serves no purpose now and only adds a variable to your device's boot chain that this build doesn't use.

---

## ⚠️ Critical warning before you do anything

**Do not disable "OEM unlocking" in Developer Options, under any circumstances.**

This toggle controls whether your bootloader is *allowed* to be unlocked in the future. If you disable it, and something goes wrong later — a bad update, a corrupted partition, anything requiring you to get back into fastboot and reflash — **you will have no way back in**. The device becomes permanently unrecoverable through normal means. Keep this toggle **enabled** at all times, as your safety net.

---

## Which method should you use?

- **Fastboot flashing** — for a fresh install, coming from stock, or switching from another ROM. Requires an unlocked bootloader.
- **OTA/Recovery sideload** — for updating an existing Neoteric OS install.
- **Dirty-flashing a newer build** (updating without wiping) — see below.

All methods require an unlocked bootloader, since relocking is no longer supported.

---

## Method 1: Fastboot Flash (fresh install)

**Requirements:** Unlocked bootloader, `fastboot` installed on your PC, USB debugging enabled if coming from a working Android install.

1. **Back up your data.** This process may wipe user data depending on your starting point.
2. **Boot into fastboot mode:**
   ```
   adb reboot bootloader
   ```
3. **Confirm your device is detected:**
   ```
   fastboot devices
   ```
4. **If you have a custom AVB key flashed from a previous build, remove it first:**
   ```
   fastboot erase avb_custom_key
   ```
5. **Flash the package:**
   ```
   fastboot update <zip>
   ```
   `fastboot update` reads the `android-info.txt` inside the zip, verifies device compatibility, and flashes every included partition image automatically in the correct order. The device reboots on its own once this finishes.
6. Let the first boot sit for a few minutes, then confirm everything works — WiFi, cellular, and general stability.

---

## Method 2: OTA / Recovery Sideload (update existing install)

**Requirements:** Device already running a compatible Neoteric OS self-signed build, recovery access, unlocked bootloader.

1. **Back up anything important.** Sideloads are generally safe for user data, but don't skip this.
2. **Reboot into recovery:**
   ```
   adb reboot recovery
   ```
3. From the recovery menu, select **Apply update > Apply from ADB** (wording may vary).
4. On your PC, sideload the OTA zip:
   ```
   adb sideload <zip>
   ```
5. Wait for the transfer and verification to complete.
6. Once complete, reboot:
   ```
   adb reboot
   ```

---

## Updating to a newer build (dirty flash, no data wipe)

Once you're already running this build, updating to a future release doesn't require starting over:

```
fastboot update <zip>
```

or via recovery sideload:

```
adb reboot recovery
adb sideload <zip>
```

Both paths update in place without wiping user data, provided the new build is compatible with your current install (check the release notes for any breaking changes before updating).

---

## After flashing

Confirm the build booted correctly, check WiFi and cellular connectivity, and verify KernelSU is functioning as expected before making this your daily driver. If you hit issues, **your OEM unlocking toggle being enabled is what lets you recover** — this cannot be overstated.
