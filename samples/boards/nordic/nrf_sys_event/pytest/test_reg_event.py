#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

import re

from twister_harness import DeviceAdapter


def _parse_alarm_timing(output: str, mode_label: str) -> int:
    match = re.search(
        rf"Alarm set for \d+ us, execution took:(\d+) \({re.escape(mode_label)}\)",
        output,
    )
    assert match is not None, f"Timing for {mode_label} was NOT found"
    return int(match.group(1))


def test_irq_latency(dut: DeviceAdapter):
    """
    PyTest code for samples/boards/nordic/nrf_sys_event, sample configurations:
    - sample.boards.nordic.nrf_sys_event.irq_latency,
    - sample.boards.nordic.nrf_sys_event.irq_latency.ppi.
    Parse logs from serial port. If the Register Event API was used correctly,
    code execution shall be faster when low-latency NVM mode is active.
    """

    TIMEOUT = 5
    MIN_DIFF = 10  # Minimal difference to pass the check

    # Get output from serial port
    output = "\n".join(
        dut.readlines_until(
            regex="All done",
            print_output=True,
            timeout=TIMEOUT,
        )
    )

    is_mramc = "NVM controller: MRAMC" in output

    if is_mramc:
        t_default = _parse_alarm_timing(output, "default FLASH_CONTROLLER mode")
        t_standby = _parse_alarm_timing(output, "FLASH_CONTROLLER Standby mode")
        _parse_alarm_timing(output, "MRAMC autopowerdown OffTrimRetain explicit")
        _parse_alarm_timing(output, "MRAMC autopowerdown MramOff explicit")

        assert t_default > t_standby + MIN_DIFF, (
            f"{t_default} is NOT larger than {t_standby} + {MIN_DIFF}"
        )
        return

    t_default = _parse_alarm_timing(output, "default FLASH_CONTROLLER mode")
    t_standby = _parse_alarm_timing(output, "FLASH_CONTROLLER Standby mode")
    t_ppi = _parse_alarm_timing(output, "FLASH_CONTROLLER waken by PPI")

    # Check if Flash Controller standby mode results in faster code execution
    assert t_default > t_standby + MIN_DIFF, (
        f"{t_default} is NOT larger than {t_standby} + {MIN_DIFF}"
    )
    # Check if Flash Controller waken by PPI results in faster code execution
    assert t_default > t_ppi + MIN_DIFF, f"{t_default} is NOT larger than {t_ppi} + {MIN_DIFF}"
