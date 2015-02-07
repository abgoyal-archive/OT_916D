



#include <acpi/acpi.h>
#include "accommon.h"

#define _COMPONENT          ACPI_HARDWARE
ACPI_MODULE_NAME("hwtimer")

acpi_status acpi_get_timer_resolution(u32 * resolution)
{
	ACPI_FUNCTION_TRACE(acpi_get_timer_resolution);

	if (!resolution) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if ((acpi_gbl_FADT.flags & ACPI_FADT_32BIT_TIMER) == 0) {
		*resolution = 24;
	} else {
		*resolution = 32;
	}

	return_ACPI_STATUS(AE_OK);
}

ACPI_EXPORT_SYMBOL(acpi_get_timer_resolution)

acpi_status acpi_get_timer(u32 * ticks)
{
	acpi_status status;

	ACPI_FUNCTION_TRACE(acpi_get_timer);

	if (!ticks) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	status =
	    acpi_hw_read(ticks, &acpi_gbl_FADT.xpm_timer_block);

	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_get_timer)

acpi_status
acpi_get_timer_duration(u32 start_ticks, u32 end_ticks, u32 * time_elapsed)
{
	acpi_status status;
	u32 delta_ticks;
	u64 quotient;

	ACPI_FUNCTION_TRACE(acpi_get_timer_duration);

	if (!time_elapsed) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Compute Tick Delta:
	 * Handle (max one) timer rollovers on 24-bit versus 32-bit timers.
	 */
	if (start_ticks < end_ticks) {
		delta_ticks = end_ticks - start_ticks;
	} else if (start_ticks > end_ticks) {
		if ((acpi_gbl_FADT.flags & ACPI_FADT_32BIT_TIMER) == 0) {

			/* 24-bit Timer */

			delta_ticks =
			    (((0x00FFFFFF - start_ticks) +
			      end_ticks) & 0x00FFFFFF);
		} else {
			/* 32-bit Timer */

			delta_ticks = (0xFFFFFFFF - start_ticks) + end_ticks;
		}
	} else {		/* start_ticks == end_ticks */

		*time_elapsed = 0;
		return_ACPI_STATUS(AE_OK);
	}

	/*
	 * Compute Duration (Requires a 64-bit multiply and divide):
	 *
	 * time_elapsed = (delta_ticks * 1000000) / PM_TIMER_FREQUENCY;
	 */
	status = acpi_ut_short_divide(((u64) delta_ticks) * 1000000,
				      PM_TIMER_FREQUENCY, &quotient, NULL);

	*time_elapsed = (u32) quotient;
	return_ACPI_STATUS(status);
}

ACPI_EXPORT_SYMBOL(acpi_get_timer_duration)
