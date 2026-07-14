#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

import re

from twister_harness import DeviceAdapter


def test_irq_latency(dut: DeviceAdapter):
    """
    PyTest code for samples/boards/nordic/nrf_sys_event, sample configurations:
    - sample.boards.nordic.nrf_sys_event.irq_latency,
    - sample.boards.nordic.nrf_sys_event.irq_latency.ppi.
    Parse logs from serial port. If the Register Event API was used correctly,
    code execution shall be faster by ~14 us when event was registered.
    If the API works correctly, Flash Controller is woken up just before event occurs.
    Thus, there is no delay resulting from Flash Controller getting ready.
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

    # Get execution times
    t_default_str = re.search(
        r"Alarm set for 100 us, execution took:(.+) \(default Flash Controller mode\)", output
    ).group(1)
    assert t_default_str is not None, "Timing for the default Flash Controller mode was NOT found"
    t_default = int(t_default_str)

    t_standby_str = re.search(
        r"Alarm set for 100 us, execution took:(.+) \(Flash Controller Standby mode\)", output
    ).group(1)
    assert t_standby_str is not None, "Timing for Flash Controller Standby mode was NOT found"
    t_standby = int(t_standby_str)

    t_ppi_str = re.search(
        r"Alarm set for 100 us, execution took:(.+) \(Flash Controller woken by PPI\)", output
    ).group(1)
    assert t_ppi_str is not None, "Timing for Flash Controller Standby mode was NOT found"
    t_ppi = int(t_ppi_str)

    # Check if Flash Controller standby mode results in faster code execution
    assert t_default > t_standby + MIN_DIFF, (
        f"{t_default} is NOT larger than {t_standby} + {MIN_DIFF}"
    )
    # Check if Flash Controller waken by PPI results in faster code execution
    assert t_default > t_ppi + MIN_DIFF, f"{t_default} is NOT larger than {t_ppi} + {MIN_DIFF}"
