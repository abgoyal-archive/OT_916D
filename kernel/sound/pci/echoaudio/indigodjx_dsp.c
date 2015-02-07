

static int update_vmixer_level(struct echoaudio *chip);
static int set_vmixer_gain(struct echoaudio *chip, u16 output,
			   u16 pipe, int gain);


static int init_hw(struct echoaudio *chip, u16 device_id, u16 subdevice_id)
{
	int err;

	DE_INIT(("init_hw() - Indigo DJx\n"));
	if (snd_BUG_ON((subdevice_id & 0xfff0) != INDIGO_DJX))
		return -ENODEV;

	err = init_dsp_comm_page(chip);
	if (err < 0) {
		DE_INIT(("init_hw - could not initialize DSP comm page\n"));
		return err;
	}

	chip->device_id = device_id;
	chip->subdevice_id = subdevice_id;
	chip->bad_board = TRUE;
	chip->dsp_code_to_load = FW_INDIGO_DJX_DSP;
	/* Since this card has no ASIC, mark it as loaded so everything
	   works OK */
	chip->asic_loaded = TRUE;
	chip->input_clock_types = ECHO_CLOCK_BIT_INTERNAL;

	err = load_firmware(chip);
	if (err < 0)
		return err;
	chip->bad_board = FALSE;

	DE_INIT(("init_hw done\n"));
	return err;
}



static int set_mixer_defaults(struct echoaudio *chip)
{
	return init_line_levels(chip);
}
