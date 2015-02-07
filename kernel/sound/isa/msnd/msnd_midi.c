

#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/errno.h>
#include <sound/core.h>
#include <sound/rawmidi.h>

#include "msnd.h"

#define MSNDMIDI_MODE_BIT_INPUT		0
#define MSNDMIDI_MODE_BIT_OUTPUT		1
#define MSNDMIDI_MODE_BIT_INPUT_TRIGGER	2
#define MSNDMIDI_MODE_BIT_OUTPUT_TRIGGER	3

struct snd_msndmidi {
	struct snd_msnd *dev;

	unsigned long mode;		/* MSNDMIDI_MODE_XXXX */

	struct snd_rawmidi_substream *substream_input;

	spinlock_t input_lock;
};

static int snd_msndmidi_input_open(struct snd_rawmidi_substream *substream)
{
	struct snd_msndmidi *mpu;

	snd_printdd("snd_msndmidi_input_open()\n");

	mpu = substream->rmidi->private_data;

	mpu->substream_input = substream;

	snd_msnd_enable_irq(mpu->dev);

	snd_msnd_send_dsp_cmd(mpu->dev, HDEX_MIDI_IN_START);
	set_bit(MSNDMIDI_MODE_BIT_INPUT, &mpu->mode);
	return 0;
}

static int snd_msndmidi_input_close(struct snd_rawmidi_substream *substream)
{
	struct snd_msndmidi *mpu;

	mpu = substream->rmidi->private_data;
	snd_msnd_send_dsp_cmd(mpu->dev, HDEX_MIDI_IN_STOP);
	clear_bit(MSNDMIDI_MODE_BIT_INPUT, &mpu->mode);
	mpu->substream_input = NULL;
	snd_msnd_disable_irq(mpu->dev);
	return 0;
}

static void snd_msndmidi_input_drop(struct snd_msndmidi *mpu)
{
	u16 tail;

	tail = readw(mpu->dev->MIDQ + JQS_wTail);
	writew(tail, mpu->dev->MIDQ + JQS_wHead);
}

static void snd_msndmidi_input_trigger(struct snd_rawmidi_substream *substream,
					int up)
{
	unsigned long flags;
	struct snd_msndmidi *mpu;

	snd_printdd("snd_msndmidi_input_trigger(, %i)\n", up);

	mpu = substream->rmidi->private_data;
	spin_lock_irqsave(&mpu->input_lock, flags);
	if (up) {
		if (!test_and_set_bit(MSNDMIDI_MODE_BIT_INPUT_TRIGGER,
				      &mpu->mode))
			snd_msndmidi_input_drop(mpu);
	} else {
		clear_bit(MSNDMIDI_MODE_BIT_INPUT_TRIGGER, &mpu->mode);
	}
	spin_unlock_irqrestore(&mpu->input_lock, flags);
	if (up)
		snd_msndmidi_input_read(mpu);
}

void snd_msndmidi_input_read(void *mpuv)
{
	unsigned long flags;
	struct snd_msndmidi *mpu = mpuv;
	void *pwMIDQData = mpu->dev->mappedbase + MIDQ_DATA_BUFF;

	spin_lock_irqsave(&mpu->input_lock, flags);
	while (readw(mpu->dev->MIDQ + JQS_wTail) !=
	       readw(mpu->dev->MIDQ + JQS_wHead)) {
		u16 wTmp, val;
		val = readw(pwMIDQData + 2 * readw(mpu->dev->MIDQ + JQS_wHead));

			if (test_bit(MSNDMIDI_MODE_BIT_INPUT_TRIGGER,
				     &mpu->mode))
				snd_rawmidi_receive(mpu->substream_input,
						    (unsigned char *)&val, 1);

		wTmp = readw(mpu->dev->MIDQ + JQS_wHead) + 1;
		if (wTmp > readw(mpu->dev->MIDQ + JQS_wSize))
			writew(0,  mpu->dev->MIDQ + JQS_wHead);
		else
			writew(wTmp,  mpu->dev->MIDQ + JQS_wHead);
	}
	spin_unlock_irqrestore(&mpu->input_lock, flags);
}
EXPORT_SYMBOL(snd_msndmidi_input_read);

static struct snd_rawmidi_ops snd_msndmidi_input = {
	.open =		snd_msndmidi_input_open,
	.close =	snd_msndmidi_input_close,
	.trigger =	snd_msndmidi_input_trigger,
};

static void snd_msndmidi_free(struct snd_rawmidi *rmidi)
{
	struct snd_msndmidi *mpu = rmidi->private_data;
	kfree(mpu);
}

int snd_msndmidi_new(struct snd_card *card, int device)
{
	struct snd_msnd *chip = card->private_data;
	struct snd_msndmidi *mpu;
	struct snd_rawmidi *rmidi;
	int err;

	err = snd_rawmidi_new(card, "MSND-MIDI", device, 1, 1, &rmidi);
	if (err < 0)
		return err;
	mpu = kzalloc(sizeof(*mpu), GFP_KERNEL);
	if (mpu == NULL) {
		snd_device_free(card, rmidi);
		return -ENOMEM;
	}
	mpu->dev = chip;
	chip->msndmidi_mpu = mpu;
	rmidi->private_data = mpu;
	rmidi->private_free = snd_msndmidi_free;
	spin_lock_init(&mpu->input_lock);
	strcpy(rmidi->name, "MSND MIDI");
	snd_rawmidi_set_ops(rmidi, SNDRV_RAWMIDI_STREAM_INPUT,
			    &snd_msndmidi_input);
	rmidi->info_flags |= SNDRV_RAWMIDI_INFO_INPUT;
	return 0;
}
