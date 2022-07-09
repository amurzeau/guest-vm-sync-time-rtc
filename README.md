# guest-vm-sync-time-rtc

This tool allow synchronizing a guest VM system time running Linux with the RTC.
This is used to keep the guest time synchronized with the host time when:

    - The host was suspended
    - The guest was suspended
    - Any other case where the host clock time jump

When this happen, the guest's system time will be in the past and won't be updated with the hardware time.

Solutions to this are:

    - Run a NTP daemon to keep the system time in sync with the real time
    - If internet is not available, you can try to sync the system time with a PTP device
    - If a PTP is not available, you can use this tool to synchronize with the hardware RTC
      (which is kept up to date with the host's system time by the hypervisor). This can be
      needed with qemu on Windows for example.

# Algorithm

This tool does this:

    - Every minute, read the RTC clock, and check if it jumped since previous read (with a margin of
      +/-10%)
    - If the RTC clock jumped forward, set the system time with the current RTC time

Note: if the RTC time jump but its time is lower than the system time, the system time won't be updated
to avoid jumping the system time back in the past. This is not handled in most application and will cause
crashes.
