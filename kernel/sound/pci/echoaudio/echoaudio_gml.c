


/* These functions are common for Gina24, Layla24 and Mona cards */


static int check_asic_status(struct echoaudio *chip)
{
	u32 asic_status;

	send_vector(chip, DSP_VC_TEST_ASIC);

	/* The DSP will return a value to indicate whether or not the
	   ASIC is currently loaded */
	if (read_dsp(chip, &asic_status) < 0) {
		DE_INIT(("check_asic_status: failed on read_dsp\n"));
		chip->asic_loaded = FALSE;
		return -EIO;
	}

	chip->asic_loaded = (asic_status == ASIC_ALREADY_LOADED);
	return chip->asic_loaded ? 0 : -EIO;
}



static int write_control_reg(struct echoaudio *chip, u32 value, char force)
{
	/* Handle the digital input auto-mute */
	if (chip->digital_in_automute)
		value |= GML_DIGITAL_IN_AUTO_MUTE;
	else
		value &= ~GML_DIGITAL_IN_AUTO_MUTE;

	DE_ACT(("write_control_reg: 0x%x\n", value));

	/* Write the control register */
	value = cpu_to_le32(value);
	if (value != chip->comm_page->control_register || force) {
		if (wait_handshake(chip))
			return -EIO;
		chip->comm_page->control_register = value;
		clear_handshake(chip);
		return send_vector(chip, DSP_VC_WRITE_CONTROL_REG);
	}
	return 0;
}



static int set_input_auto_mute(struct echoaudio *chip, int automute)
{
	DE_ACT(("set_input_auto_mute %d\n", automute));

	chip->digital_in_automute = automute;

	/* Re-set the input clock to the current value - indirectly causes
	the auto-mute flag to be sent to the DSP */
	return set_input_clock(chip, chip->input_clock);
}



/* S/PDIF coax / S/PDIF optical / ADAT - switch */
static int set_digital_mode(struct echoaudio *chip, u8 mode)
{
	u8 previous_mode;
	int err, i, o;

	if (chip->bad_board)
		return -EIO;

	/* All audio channels must be closed before changing the digital mode */
	if (snd_BUG_ON(chip->pipe_alloc_mask))
		return -EAGAIN;

	if (snd_BUG_ON(!(chip->digital_modes & (1 << mode))))
		return -EINVAL;

	previous_mode = chip->digital_mode;
	err = dsp_set_digital_mode(chip, mode);

	/* If we successfully changed the digital mode from or to ADAT,
	   then make sure all output, input and monitor levels are
	   updated by the DSP comm object. */
	if (err >= 0 && previous_mode != mode &&
	    (previous_mode == DIGITAL_MODE_ADAT || mode == DIGITAL_MODE_ADAT)) {
		spin_lock_irq(&chip->lock);
		for (o = 0; o < num_busses_out(chip); o++)
			for (i = 0; i < num_busses_in(chip); i++)
				set_monitor_gain(chip, o, i,
						 chip->monitor_gain[o][i]);

#ifdef ECHOCARD_HAS_INPUT_GAIN
		for (i = 0; i < num_busses_in(chip); i++)
			set_input_gain(chip, i, chip->input_gain[i]);
		update_input_line_level(chip);
#endif

		for (o = 0; o < num_busses_out(chip); o++)
			set_output_gain(chip, o, chip->output_gain[o]);
		update_output_line_level(chip);
		spin_unlock_irq(&chip->lock);
	}

	return err;
}



/* Set the S/PDIF output format */
static int set_professional_spdif(struct echoaudio *chip, char prof)
{
	u32 control_reg;
	int err;

	/* Clear the current S/PDIF flags */
	control_reg = le32_to_cpu(chip->comm_page->control_register);
	control_reg &= GML_SPDIF_FORMAT_CLEAR_MASK;

	/* Set the new S/PDIF flags depending on the mode */
	control_reg |= GML_SPDIF_TWO_CHANNEL | GML_SPDIF_24_BIT |
		GML_SPDIF_COPY_PERMIT;
	if (prof) {
		/* Professional mode */
		control_reg |= GML_SPDIF_PRO_MODE;

		switch (chip->sample_rate) {
		case 32000:
			control_reg |= GML_SPDIF_SAMPLE_RATE0 |
				GML_SPDIF_SAMPLE_RATE1;
			break;
		case 44100:
			control_reg |= GML_SPDIF_SAMPLE_RATE0;
			break;
		case 48000:
			control_reg |= GML_SPDIF_SAMPLE_RATE1;
			break;
		}
	} else {
		/* Consumer mode */
		switch (chip->sample_rate) {
		case 32000:
			control_reg |= GML_SPDIF_SAMPLE_RATE0 |
				GML_SPDIF_SAMPLE_RATE1;
			break;
		case 48000:
			control_reg |= GML_SPDIF_SAMPLE_RATE1;
			break;
		}
	}

	if ((err = write_control_reg(chip, control_reg, FALSE)))
		return err;
	chip->professional_spdif = prof;
	DE_ACT(("set_professional_spdif to %s\n",
		prof ? "Professional" : "Consumer"));
	return 0;
}
