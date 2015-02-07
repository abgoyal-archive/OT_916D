
#ifndef __SOUND_ASOUND_FM_H
#define __SOUND_ASOUND_FM_H


#define SNDRV_DM_FM_MODE_OPL2	0x00
#define SNDRV_DM_FM_MODE_OPL3	0x01

struct snd_dm_fm_info {
	unsigned char fm_mode;		/* OPL mode, see SNDRV_DM_FM_MODE_XXX */
	unsigned char rhythm;		/* percussion mode flag */
};


struct snd_dm_fm_voice {
	unsigned char op;		/* operator cell (0 or 1) */
	unsigned char voice;		/* FM voice (0 to 17) */

	unsigned char am;		/* amplitude modulation */
	unsigned char vibrato;		/* vibrato effect */
	unsigned char do_sustain;	/* sustain phase */
	unsigned char kbd_scale;	/* keyboard scaling */
	unsigned char harmonic;		/* 4 bits: harmonic and multiplier */
	unsigned char scale_level;	/* 2 bits: decrease output freq rises */
	unsigned char volume;		/* 6 bits: volume */

	unsigned char attack;		/* 4 bits: attack rate */
	unsigned char decay;		/* 4 bits: decay rate */
	unsigned char sustain;		/* 4 bits: sustain level */
	unsigned char release;		/* 4 bits: release rate */

	unsigned char feedback;		/* 3 bits: feedback for op0 */
	unsigned char connection;	/* 0 for serial, 1 for parallel */
	unsigned char left;		/* stereo left */
	unsigned char right;		/* stereo right */
	unsigned char waveform;		/* 3 bits: waveform shape */
};


struct snd_dm_fm_note {
	unsigned char voice;	/* 0-17 voice channel */
	unsigned char octave;	/* 3 bits: what octave to play */
	unsigned int fnum;	/* 10 bits: frequency number */
	unsigned char key_on;	/* set for active, clear for silent */
};


struct snd_dm_fm_params {
	unsigned char am_depth;		/* amplitude modulation depth (1=hi) */
	unsigned char vib_depth;	/* vibrato depth (1=hi) */
	unsigned char kbd_split;	/* keyboard split */
	unsigned char rhythm;		/* percussion mode select */

	/* This block is the percussion instrument data */
	unsigned char bass;
	unsigned char snare;
	unsigned char tomtom;
	unsigned char cymbal;
	unsigned char hihat;
};


#define SNDRV_DM_FM_IOCTL_INFO		_IOR('H', 0x20, struct snd_dm_fm_info)
#define SNDRV_DM_FM_IOCTL_RESET		_IO ('H', 0x21)
#define SNDRV_DM_FM_IOCTL_PLAY_NOTE	_IOW('H', 0x22, struct snd_dm_fm_note)
#define SNDRV_DM_FM_IOCTL_SET_VOICE	_IOW('H', 0x23, struct snd_dm_fm_voice)
#define SNDRV_DM_FM_IOCTL_SET_PARAMS	_IOW('H', 0x24, struct snd_dm_fm_params)
#define SNDRV_DM_FM_IOCTL_SET_MODE	_IOW('H', 0x25, int)
/* for OPL3 only */
#define SNDRV_DM_FM_IOCTL_SET_CONNECTION	_IOW('H', 0x26, int)
/* SBI patch management */
#define SNDRV_DM_FM_IOCTL_CLEAR_PATCHES	_IO ('H', 0x40)

#define SNDRV_DM_FM_OSS_IOCTL_RESET		0x20
#define SNDRV_DM_FM_OSS_IOCTL_PLAY_NOTE		0x21
#define SNDRV_DM_FM_OSS_IOCTL_SET_VOICE		0x22
#define SNDRV_DM_FM_OSS_IOCTL_SET_PARAMS	0x23
#define SNDRV_DM_FM_OSS_IOCTL_SET_MODE		0x24
#define SNDRV_DM_FM_OSS_IOCTL_SET_OPL		0x25


#define FM_KEY_SBI	"SBI\032"
#define FM_KEY_2OP	"2OP\032"
#define FM_KEY_4OP	"4OP\032"

struct sbi_patch {
	unsigned char prog;
	unsigned char bank;
	char key[4];
	char name[25];
	char extension[7];
	unsigned char data[32];
};

#endif /* __SOUND_ASOUND_FM_H */
