

#include "hwdrv_apci2200.h"

int i_APCI2200_Read1DigitalInput(struct comedi_device *dev, struct comedi_subdevice *s,
	struct comedi_insn *insn, unsigned int *data)
{
	unsigned int ui_TmpValue = 0;
	unsigned int ui_Channel;
	ui_Channel = CR_CHAN(insn->chanspec);
	if (ui_Channel <= 7) {
		ui_TmpValue = (unsigned int) inw(devpriv->iobase + APCI2200_DIGITAL_IP);
		*data = (ui_TmpValue >> ui_Channel) & 0x1;
	}			/* if(ui_Channel >= 0 && ui_Channel <=7) */
	else {
		printk("\nThe specified channel does not exist\n");
		return -EINVAL;	/*  "sorry channel spec wrong " */
	}			/* else if(ui_Channel >= 0 && ui_Channel <=7) */

	return insn->n;
}


int i_APCI2200_ReadMoreDigitalInput(struct comedi_device *dev, struct comedi_subdevice *s,
	struct comedi_insn *insn, unsigned int *data)
{

	unsigned int ui_PortValue = data[0];
	unsigned int ui_Mask = 0;
	unsigned int ui_NoOfChannels;

	ui_NoOfChannels = CR_CHAN(insn->chanspec);

	*data = (unsigned int) inw(devpriv->iobase + APCI2200_DIGITAL_IP);
	switch (ui_NoOfChannels) {
	case 2:
		ui_Mask = 3;
		*data = (*data >> (2 * ui_PortValue)) & ui_Mask;
		break;
	case 4:
		ui_Mask = 15;
		*data = (*data >> (4 * ui_PortValue)) & ui_Mask;
		break;
	case 7:
		break;

	default:
		printk("\nWrong parameters\n");
		return -EINVAL;	/*  "sorry channel spec wrong " */
		break;
	}			/* switch(ui_NoOfChannels) */

	return insn->n;
}

int i_APCI2200_ConfigDigitalOutput(struct comedi_device *dev, struct comedi_subdevice *s,
	struct comedi_insn *insn, unsigned int *data)
{
	devpriv->b_OutputMemoryStatus = data[0];
	return insn->n;
}


int i_APCI2200_WriteDigitalOutput(struct comedi_device *dev, struct comedi_subdevice *s,
	struct comedi_insn *insn, unsigned int *data)
{
	unsigned int ui_Temp, ui_Temp1;
	unsigned int ui_NoOfChannel = CR_CHAN(insn->chanspec);	/*  get the channel */
	if (devpriv->b_OutputMemoryStatus) {
		ui_Temp = inw(devpriv->iobase + APCI2200_DIGITAL_OP);

	}			/* if(devpriv->b_OutputMemoryStatus ) */
	else {
		ui_Temp = 0;
	}			/* if(devpriv->b_OutputMemoryStatus ) */
	if (data[3] == 0) {
		if (data[1] == 0) {
			data[0] = (data[0] << ui_NoOfChannel) | ui_Temp;
			outw(data[0], devpriv->iobase + APCI2200_DIGITAL_OP);
		}		/* if(data[1]==0) */
		else {
			if (data[1] == 1) {
				switch (ui_NoOfChannel) {

				case 2:
					data[0] =
						(data[0] << (2 *
							data[2])) | ui_Temp;
					break;

				case 4:
					data[0] =
						(data[0] << (4 *
							data[2])) | ui_Temp;
					break;

				case 8:
					data[0] =
						(data[0] << (8 *
							data[2])) | ui_Temp;
					break;
				case 15:
					data[0] = data[0] | ui_Temp;
					break;
				default:
					comedi_error(dev, " chan spec wrong");
					return -EINVAL;	/*  "sorry channel spec wrong " */

				}	/* switch(ui_NoOfChannels) */

				outw(data[0],
					devpriv->iobase + APCI2200_DIGITAL_OP);
			}	/*  if(data[1]==1) */
			else {
				printk("\nSpecified channel not supported\n");
			}	/* else if(data[1]==1) */
		}		/* elseif(data[1]==0) */
	}			/* if(data[3]==0) */
	else {
		if (data[3] == 1) {
			if (data[1] == 0) {
				data[0] = ~data[0] & 0x1;
				ui_Temp1 = 1;
				ui_Temp1 = ui_Temp1 << ui_NoOfChannel;
				ui_Temp = ui_Temp | ui_Temp1;
				data[0] = (data[0] << ui_NoOfChannel) ^ 0xffff;
				data[0] = data[0] & ui_Temp;
				outw(data[0],
					devpriv->iobase + APCI2200_DIGITAL_OP);
			}	/* if(data[1]==0) */
			else {
				if (data[1] == 1) {
					switch (ui_NoOfChannel) {

					case 2:
						data[0] = ~data[0] & 0x3;
						ui_Temp1 = 3;
						ui_Temp1 =
							ui_Temp1 << 2 * data[2];
						ui_Temp = ui_Temp | ui_Temp1;
						data[0] =
							((data[0] << (2 *
									data
									[2])) ^
							0xffff) & ui_Temp;
						break;

					case 4:
						data[0] = ~data[0] & 0xf;
						ui_Temp1 = 15;
						ui_Temp1 =
							ui_Temp1 << 4 * data[2];
						ui_Temp = ui_Temp | ui_Temp1;
						data[0] =
							((data[0] << (4 *
									data
									[2])) ^
							0xffff) & ui_Temp;
						break;

					case 8:
						data[0] = ~data[0] & 0xff;
						ui_Temp1 = 255;
						ui_Temp1 =
							ui_Temp1 << 8 * data[2];
						ui_Temp = ui_Temp | ui_Temp1;
						data[0] =
							((data[0] << (8 *
									data
									[2])) ^
							0xffff) & ui_Temp;
						break;
					case 15:
						break;

					default:
						comedi_error(dev,
							" chan spec wrong");
						return -EINVAL;	/*  "sorry channel spec wrong " */

					}	/* switch(ui_NoOfChannels) */

					outw(data[0],
						devpriv->iobase +
						APCI2200_DIGITAL_OP);
				}	/*  if(data[1]==1) */
				else {
					printk("\nSpecified channel not supported\n");
				}	/* else if(data[1]==1) */
			}	/* elseif(data[1]==0) */
		}		/* if(data[3]==1); */
		else {
			printk("\nSpecified functionality does not exist\n");
			return -EINVAL;
		}		/* if else data[3]==1) */
	}			/* if else data[3]==0) */
	return insn->n;
}


int i_APCI2200_ReadDigitalOutput(struct comedi_device *dev, struct comedi_subdevice *s,
	struct comedi_insn *insn, unsigned int *data)
{

	unsigned int ui_Temp;
	unsigned int ui_NoOfChannel = CR_CHAN(insn->chanspec);	/*  get the channel */
	ui_Temp = data[0];
	*data = inw(devpriv->iobase + APCI2200_DIGITAL_OP);
	if (ui_Temp == 0) {
		*data = (*data >> ui_NoOfChannel) & 0x1;
	}			/* if(ui_Temp==0) */
	else {
		if (ui_Temp == 1) {
			switch (ui_NoOfChannel) {

			case 2:
				*data = (*data >> (2 * data[1])) & 3;
				break;

			case 4:
				*data = (*data >> (4 * data[1])) & 15;
				break;

			case 8:
				*data = (*data >> (8 * data[1])) & 255;
				break;

			case 15:
				break;

			default:
				comedi_error(dev, " chan spec wrong");
				return -EINVAL;	/*  "sorry channel spec wrong " */

			}	/* switch(ui_NoOfChannels) */
		}		/* if(ui_Temp==1) */
		else {
			printk("\nSpecified channel not supported \n");
		}		/* elseif(ui_Temp==1) */
	}			/* elseif(ui_Temp==0) */
	return insn->n;
}


int i_APCI2200_ConfigWatchdog(struct comedi_device *dev, struct comedi_subdevice *s,
	struct comedi_insn *insn, unsigned int *data)
{
	if (data[0] == 0) {
		/* Disable the watchdog */
		outw(0x0,
			devpriv->iobase + APCI2200_WATCHDOG +
			APCI2200_WATCHDOG_ENABLEDISABLE);
		/* Loading the Reload value */
		outw(data[1],
			devpriv->iobase + APCI2200_WATCHDOG +
			APCI2200_WATCHDOG_RELOAD_VALUE);
		data[1] = data[1] >> 16;
		outw(data[1],
			devpriv->iobase + APCI2200_WATCHDOG +
			APCI2200_WATCHDOG_RELOAD_VALUE + 2);
	}			/* if(data[0]==0) */
	else {
		printk("\nThe input parameters are wrong\n");
		return -EINVAL;
	}			/* elseif(data[0]==0) */

	return insn->n;
}

 /*
    +----------------------------------------------------------------------------+
    | Function   Name   : int i_APCI2200_StartStopWriteWatchdog                  |
    |                           (struct comedi_device *dev,struct comedi_subdevice *s,
    struct comedi_insn *insn,unsigned int *data);                      |
    +----------------------------------------------------------------------------+
    | Task              : Start / Stop The Watchdog                              |
    +----------------------------------------------------------------------------+
    | Input Parameters  : struct comedi_device *dev      : Driver handle                |
    |                     struct comedi_subdevice *s,   :pointer to subdevice structure
    struct comedi_insn *insn      :pointer to insn structure      |
    |                     unsigned int *data          : Data Pointer to read status  |
    +----------------------------------------------------------------------------+
    | Output Parameters :       --                                                                                                       |
    +----------------------------------------------------------------------------+
    | Return Value      : TRUE  : No error occur                                 |
    |                       : FALSE : Error occur. Return the error          |
    |                                                                            |
    +----------------------------------------------------------------------------+
  */

int i_APCI2200_StartStopWriteWatchdog(struct comedi_device *dev, struct comedi_subdevice *s,
	struct comedi_insn *insn, unsigned int *data)
{
	switch (data[0]) {
	case 0:		/* stop the watchdog */
		outw(0x0, devpriv->iobase + APCI2200_WATCHDOG + APCI2200_WATCHDOG_ENABLEDISABLE);	/* disable the watchdog */
		break;
	case 1:		/* start the watchdog */
		outw(0x0001,
			devpriv->iobase + APCI2200_WATCHDOG +
			APCI2200_WATCHDOG_ENABLEDISABLE);
		break;
	case 2:		/* Software trigger */
		outw(0x0201,
			devpriv->iobase + APCI2200_WATCHDOG +
			APCI2200_WATCHDOG_ENABLEDISABLE);
		break;
	default:
		printk("\nSpecified functionality does not exist\n");
		return -EINVAL;
	}			/*  switch(data[0]) */
	return insn->n;
}


int i_APCI2200_ReadWatchdog(struct comedi_device *dev, struct comedi_subdevice *s,
	struct comedi_insn *insn, unsigned int *data)
{
	data[0] =
		inw(devpriv->iobase + APCI2200_WATCHDOG +
		APCI2200_WATCHDOG_STATUS) & 0x1;
	return insn->n;
}


int i_APCI2200_Reset(struct comedi_device *dev)
{
	outw(0x0, devpriv->iobase + APCI2200_DIGITAL_OP);	/* RESETS THE DIGITAL OUTPUTS */
	outw(0x0,
		devpriv->iobase + APCI2200_WATCHDOG +
		APCI2200_WATCHDOG_ENABLEDISABLE);
	outw(0x0,
		devpriv->iobase + APCI2200_WATCHDOG +
		APCI2200_WATCHDOG_RELOAD_VALUE);
	outw(0x0,
		devpriv->iobase + APCI2200_WATCHDOG +
		APCI2200_WATCHDOG_RELOAD_VALUE + 2);
	return 0;
}
