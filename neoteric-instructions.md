# Neoteric OS 4.0 — Build 230826 (User)

Custom Android build for the Nothing Phone (1), self-signed with locked bootloader support, KernelSU root, and full AVB verification intact.

## Highlights

- **Bootloader unlock/lock support** — this build is fully self-signed with a custom AVB key chain, meaning you can flash it with the bootloader **locked** for verified boot, or unlocked for more flexibility.
- **KernelSU root** — root access without sacrificing a `user` build's security posture. No `userdebug`, no `adb root`, no weakened `ro.secure`.
- **WiFi, cellular, and hardware acceleration confirmed working** on real device testing prior to this release.
- **`user` build variant** — production build type, stricter verification, smaller attack surface than `userdebug`.

---

## ⚠️ Critical warning before you do anything

**Do not disable "OEM unlocking" in Developer Options, under any circumstances, even after flashing and locking the bootloader.**

This toggle controls whether your bootloader is *allowed* to be unlocked in the future. If you disable it while locked, and something goes wrong later — a bad update, a corrupted partition, anything requiring you to get back into fastboot and reflash — **you will have no way back in**. The device becomes permanently unrecoverable through normal means. Keep this toggle **enabled** at all times, regardless of your lock state, as your safety net.

---

## Which method should you use?

- **Fastboot flashing** — for a fresh install, coming from stock, or switching from another ROM. Requires an unlocked bootloader.
- **OTA/Recovery sideload** — for updating an existing Neoteric OS install while keeping the bootloader **locked**.
- **Dirty-flashing a newer build** (updating without wiping) — method depends on your current lock state; see below.

---

## Method 1: Fastboot Flash (fresh install / unlocked bootloader)

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
4. **Flash the package:**
   ```
   fastboot update <zip>
   ```
   `fastboot update` reads the `android-info.txt` inside the zip, verifies device compatibility, and flashes every included partition image automatically in the correct order. The device reboots on its own once this finishes.
5. Let the first boot sit for a few minutes, then confirm everything works — WiFi, cellular, and general stability — **before** proceeding to lock the bootloader.

### Optional: locking the bootloader after flashing

Locking gives you AVB-enforced verified boot on this self-signed build. This is optional — you can stay unlocked indefinitely with no downside besides the standard unlocked-bootloader security tradeoffs.

1. Reboot back into fastboot mode:
   ```
   adb reboot bootloader
   ```
2. Flash your custom AVB key (included in the release download — `pkmd.bin`):
   ```
   fastboot flash avb_custom_key pkmd.bin
   ```
3. Lock the bootloader:
   ```
   fastboot flashing lock
   ```
4. Confirm the prompt on-device (this will trigger a data wipe — expected behavior for any lock/unlock transition).
5. Device reboots into your locked, AVB-verified Neoteric OS install.

**Do this only after confirming the unlocked build boots and functions correctly.** Locking with an unverified or broken build gives you a much harder recovery path.

---

## Method 2: OTA / Recovery Sideload (update existing install, locked bootloader)

**Requirements:** Device already running a compatible Neoteric OS self-signed build, recovery access.

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
5. Wait for the transfer and verification to complete — the recovery validates the AVB signature chain before applying.
6. Once complete, reboot:
   ```
   adb reboot
   ```

**Locked bootloader users:** this is your update path. The recovery verifies the package against your device's existing AVB trust chain before allowing the flash to proceed — if verification fails, the package is rejected and your current install stays untouched.

---

## Updating to a newer build (dirty flash, no data wipe)

Once you're already running this build, updating to a future release doesn't require starting over — but **which method you use depends on your bootloader's current lock state**:

- **Unlocked bootloader:** just run the fastboot command again, same as your original install:
  ```
  fastboot update <zip>
  ```
- **Locked bootloader:** you cannot use `fastboot update` while locked — the bootloader will reject unsigned or improperly-flashed partition writes outside the verified OTA path. Instead, use the recovery sideload method:
  ```
  adb reboot recovery
  ```
  then, from the recovery's **Apply from ADB** option:
  ```
  adb sideload <zip>
  ```

Both paths update in place without wiping user data, provided the new build is compatible with your current install (check the release notes for any breaking changes before updating).

---

## After flashing

Confirm the build booted correctly, check WiFi and cellular connectivity, and verify KernelSU is functioning as expected before making this your daily driver. If you hit issues, **your OEM unlocking toggle being enabled is what lets you recover** — this cannot be overstated.
