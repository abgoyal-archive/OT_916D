

#include "APCI1710_Dig_io.h"


int i_APCI1710_InsnConfigDigitalIO(struct comedi_device *dev, struct comedi_subdevice *s,
	struct comedi_insn *insn, unsigned int *data)
{
	unsigned char b_ModulNbr, b_ChannelAMode, b_ChannelBMode;
	unsigned char b_MemoryOnOff, b_ConfigType;
	int i_ReturnValue = 0;
	unsigned int dw_WriteConfig = 0;

	b_ModulNbr = (unsigned char) CR_AREF(insn->chanspec);
	b_ConfigType = (unsigned char) data[0];	/*  Memory or  Init */
	b_ChannelAMode = (unsigned char) data[1];
	b_ChannelBMode = (unsigned char) data[2];
	b_MemoryOnOff = (unsigned char) data[1];	/*  if memory operation */
	i_ReturnValue = insn->n;

		/**************************/
	/* Test the module number */
		/**************************/

	if (b_ModulNbr >= 4) {
		DPRINTK("Module Number invalid\n");
		i_ReturnValue = -2;
		return i_ReturnValue;
	}
	switch (b_ConfigType) {
	case APCI1710_DIGIO_MEMORYONOFF:

		if (b_MemoryOnOff)	/*  If Memory ON */
		{
		 /****************************/
			/* Set the output memory on */
		 /****************************/

			devpriv->s_ModuleInfo[b_ModulNbr].
				s_DigitalIOInfo.b_OutputMemoryEnabled = 1;

		 /***************************/
			/* Clear the output memory */
		 /***************************/
			devpriv->s_ModuleInfo[b_ModulNbr].
				s_DigitalIOInfo.dw_OutputMemory = 0;
		} else		/*  If memory off */
		{
		 /*****************************/
			/* Set the output memory off */
		 /*****************************/

			devpriv->s_ModuleInfo[b_ModulNbr].
				s_DigitalIOInfo.b_OutputMemoryEnabled = 0;
		}
		break;

	case APCI1710_DIGIO_INIT:

	/*******************************/
		/* Test if digital I/O counter */
	/*******************************/

		if ((devpriv->s_BoardInfos.
				dw_MolduleConfiguration[b_ModulNbr] &
				0xFFFF0000UL) == APCI1710_DIGITAL_IO) {

	/***************************************************/
			/* Test the bi-directional channel A configuration */
	/***************************************************/

			if ((b_ChannelAMode == 0) || (b_ChannelAMode == 1)) {
	/***************************************************/
				/* Test the bi-directional channel B configuration */
	/***************************************************/

				if ((b_ChannelBMode == 0)
					|| (b_ChannelBMode == 1)) {
					devpriv->s_ModuleInfo[b_ModulNbr].
						s_DigitalIOInfo.b_DigitalInit =
						1;

	/********************************/
					/* Save channel A configuration */
	/********************************/

					devpriv->s_ModuleInfo[b_ModulNbr].
						s_DigitalIOInfo.
						b_ChannelAMode = b_ChannelAMode;

	/********************************/
					/* Save channel B configuration */
	/********************************/

					devpriv->s_ModuleInfo[b_ModulNbr].
						s_DigitalIOInfo.
						b_ChannelBMode = b_ChannelBMode;

	/*****************************************/
					/* Set the channel A and B configuration */
	/*****************************************/

					dw_WriteConfig =
						(unsigned int) (b_ChannelAMode |
						(b_ChannelBMode * 2));

	/***************************/
					/* Write the configuration */
	/***************************/

					outl(dw_WriteConfig,
						devpriv->s_BoardInfos.
						ui_Address + 4 +
						(64 * b_ModulNbr));

				} else {
	/************************************************/
					/* Bi-directional channel B configuration error */
	/************************************************/
					DPRINTK("Bi-directional channel B configuration error\n");
					i_ReturnValue = -5;
				}

			} else {
	/************************************************/
				/* Bi-directional channel A configuration error */
	/************************************************/
				DPRINTK("Bi-directional channel A configuration error\n");
				i_ReturnValue = -4;

			}

		} else {
	/******************************************/
			/* The module is not a digital I/O module */
	/******************************************/
			DPRINTK("The module is not a digital I/O module\n");
			i_ReturnValue = -3;
		}
	}			/*  end of Switch */
	printk("Return Value %d\n", i_ReturnValue);
	return i_ReturnValue;
}



/* _INT_   i_APCI1710_ReadDigitalIOChlValue      (unsigned char_    b_BoardHandle, */
int i_APCI1710_InsnReadDigitalIOChlValue(struct comedi_device *dev,
	struct comedi_subdevice *s, struct comedi_insn *insn, unsigned int *data)
{
	int i_ReturnValue = 0;
	unsigned int dw_StatusReg;
	unsigned char b_ModulNbr, b_InputChannel;
	unsigned char *pb_ChannelStatus;
	b_ModulNbr = (unsigned char) CR_AREF(insn->chanspec);
	b_InputChannel = (unsigned char) CR_CHAN(insn->chanspec);
	data[0] = 0;
	pb_ChannelStatus = (unsigned char *) &data[0];
	i_ReturnValue = insn->n;

	/**************************/
	/* Test the module number */
	/**************************/

	if (b_ModulNbr < 4) {
	   /*******************************/
		/* Test if digital I/O counter */
	   /*******************************/

		if ((devpriv->s_BoardInfos.
				dw_MolduleConfiguration[b_ModulNbr] &
				0xFFFF0000UL) == APCI1710_DIGITAL_IO) {
	      /******************************************/
			/* Test the digital imnput channel number */
	      /******************************************/

			if (b_InputChannel <= 6) {
		 /**********************************************/
				/* Test if the digital I/O module initialised */
		 /**********************************************/

				if (devpriv->s_ModuleInfo[b_ModulNbr].
					s_DigitalIOInfo.b_DigitalInit == 1) {
		    /**********************************/
					/* Test if channel A or channel B */
		    /**********************************/

					if (b_InputChannel > 4) {
		       /*********************/
						/* Test if channel A */
		       /*********************/

						if (b_InputChannel == 5) {
			  /***************************/
							/* Test the channel A mode */
			  /***************************/

							if (devpriv->
								s_ModuleInfo
								[b_ModulNbr].
								s_DigitalIOInfo.
								b_ChannelAMode
								!= 0) {
			     /********************************************/
								/* The digital channel A is used for output */
			     /********************************************/

								i_ReturnValue =
									-6;
							}
						}	/*  if (b_InputChannel == 5) */
						else {
			  /***************************/
							/* Test the channel B mode */
			  /***************************/

							if (devpriv->
								s_ModuleInfo
								[b_ModulNbr].
								s_DigitalIOInfo.
								b_ChannelBMode
								!= 0) {
			     /********************************************/
								/* The digital channel B is used for output */
			     /********************************************/

								i_ReturnValue =
									-7;
							}
						}	/*  if (b_InputChannel == 5) */
					}	/*  if (b_InputChannel > 4) */

		    /***********************/
					/* Test if error occur */
		    /***********************/

					if (i_ReturnValue >= 0) {
		       /**************************/
						/* Read all digital input */
		       /**************************/


						dw_StatusReg =
							inl(devpriv->
							s_BoardInfos.
							ui_Address +
							(64 * b_ModulNbr));

						*pb_ChannelStatus =
							(unsigned char) ((dw_StatusReg ^
								0x1C) >>
							b_InputChannel) & 1;

					}	/*  if (i_ReturnValue == 0) */
				} else {
		    /*******************************/
					/* Digital I/O not initialised */
		    /*******************************/
					DPRINTK("Digital I/O not initialised\n");
					i_ReturnValue = -5;
				}
			} else {
		 /********************************/
				/* Selected digital input error */
		 /********************************/
				DPRINTK("Selected digital input error\n");
				i_ReturnValue = -4;
			}
		} else {
	      /******************************************/
			/* The module is not a digital I/O module */
	      /******************************************/
			DPRINTK("The module is not a digital I/O module\n");
			i_ReturnValue = -3;
		}
	} else {
	   /***********************/
		/* Module number error */
	   /***********************/
		DPRINTK("Module number error\n");
		i_ReturnValue = -2;
	}

	return i_ReturnValue;
}



int i_APCI1710_InsnWriteDigitalIOChlOnOff(struct comedi_device *dev,
	struct comedi_subdevice *s, struct comedi_insn *insn, unsigned int *data)
{
	int i_ReturnValue = 0;
	unsigned int dw_WriteValue = 0;
	unsigned char b_ModulNbr, b_OutputChannel;
	i_ReturnValue = insn->n;
	b_ModulNbr = CR_AREF(insn->chanspec);
	b_OutputChannel = CR_CHAN(insn->chanspec);

	/**************************/
	/* Test the module number */
	/**************************/

	if (b_ModulNbr < 4) {
	   /*******************************/
		/* Test if digital I/O counter */
	   /*******************************/

		if ((devpriv->s_BoardInfos.
				dw_MolduleConfiguration[b_ModulNbr] &
				0xFFFF0000UL) == APCI1710_DIGITAL_IO) {
	      /**********************************************/
			/* Test if the digital I/O module initialised */
	      /**********************************************/

			if (devpriv->s_ModuleInfo[b_ModulNbr].
				s_DigitalIOInfo.b_DigitalInit == 1) {
		 /******************************************/
				/* Test the digital output channel number */
		 /******************************************/

				switch (b_OutputChannel) {
		    /*************/
					/* Channel H */
		    /*************/

				case 0:
					break;

		    /*************/
					/* Channel A */
		    /*************/

				case 1:
					if (devpriv->s_ModuleInfo[b_ModulNbr].
						s_DigitalIOInfo.
						b_ChannelAMode != 1) {
			    /*******************************************/
						/* The digital channel A is used for input */
			    /*******************************************/

						i_ReturnValue = -6;
					}
					break;

		    /*************/
					/* Channel B */
		    /*************/

				case 2:
					if (devpriv->s_ModuleInfo[b_ModulNbr].
						s_DigitalIOInfo.
						b_ChannelBMode != 1) {
			    /*******************************************/
						/* The digital channel B is used for input */
			    /*******************************************/

						i_ReturnValue = -7;
					}
					break;

				default:
			 /****************************************/
					/* The selected digital output is wrong */
			 /****************************************/

					i_ReturnValue = -4;
					break;
				}

		 /***********************/
				/* Test if error occur */
		 /***********************/

				if (i_ReturnValue >= 0) {

			/*********************************/
					/* Test if set channel ON        */
		    /*********************************/
					if (data[0]) {
		    /*********************************/
						/* Test if output memory enabled */
		    /*********************************/

						if (devpriv->
							s_ModuleInfo
							[b_ModulNbr].
							s_DigitalIOInfo.
							b_OutputMemoryEnabled ==
							1) {
							dw_WriteValue =
								devpriv->
								s_ModuleInfo
								[b_ModulNbr].
								s_DigitalIOInfo.
								dw_OutputMemory
								| (1 <<
								b_OutputChannel);

							devpriv->
								s_ModuleInfo
								[b_ModulNbr].
								s_DigitalIOInfo.
								dw_OutputMemory
								= dw_WriteValue;
						} else {
							dw_WriteValue =
								1 <<
								b_OutputChannel;
						}
					}	/*  set channel off */
					else {
						if (devpriv->
							s_ModuleInfo
							[b_ModulNbr].
							s_DigitalIOInfo.
							b_OutputMemoryEnabled ==
							1) {
							dw_WriteValue =
								devpriv->
								s_ModuleInfo
								[b_ModulNbr].
								s_DigitalIOInfo.
								dw_OutputMemory
								& (0xFFFFFFFFUL
								-
								(1 << b_OutputChannel));

							devpriv->
								s_ModuleInfo
								[b_ModulNbr].
								s_DigitalIOInfo.
								dw_OutputMemory
								= dw_WriteValue;
						} else {
							/*****************************/
							/* Digital Output Memory OFF */
							/*****************************/
							/*  +Use previously the function "i_APCI1710_SetDigitalIOMemoryOn" */
							i_ReturnValue = -8;
						}

					}
					/*******************/
					/* Write the value */
					/*******************/

					/* OUTPDW (ps_APCI1710Variable->
					 * s_Board [b_BoardHandle].
					 * s_BoardInfos. ui_Address + (64 * b_ModulNbr),
					 * dw_WriteValue);
					 */
*/
					outl(dw_WriteValue,
						devpriv->s_BoardInfos.
						ui_Address + (64 * b_ModulNbr));
				}
			} else {
		 /*******************************/
				/* Digital I/O not initialised */
		 /*******************************/

				i_ReturnValue = -5;
			}
		} else {
	      /******************************************/
			/* The module is not a digital I/O module */
	      /******************************************/

			i_ReturnValue = -3;
		}
	} else {
	   /***********************/
		/* Module number error */
	   /***********************/

		i_ReturnValue = -2;
	}

	return i_ReturnValue;
}


int i_APCI1710_InsnBitsDigitalIOPortOnOff(struct comedi_device *dev,
	struct comedi_subdevice *s, struct comedi_insn *insn, unsigned int *data)
{
	int i_ReturnValue = 0;
	unsigned int dw_WriteValue = 0;
	unsigned int dw_StatusReg;
	unsigned char b_ModulNbr, b_PortValue;
	unsigned char b_PortOperation, b_PortOnOFF;

	unsigned char *pb_PortValue;

	b_ModulNbr = (unsigned char) CR_AREF(insn->chanspec);
	b_PortOperation = (unsigned char) data[0];	/*  Input or output */
	b_PortOnOFF = (unsigned char) data[1];	/*  if output then On or Off */
	b_PortValue = (unsigned char) data[2];	/*  if out put then Value */
	i_ReturnValue = insn->n;
	pb_PortValue = (unsigned char *) &data[0];
/* if input then read value */

	switch (b_PortOperation) {
	case APCI1710_INPUT:
		/**************************/
		/* Test the module number */
		/**************************/

		if (b_ModulNbr < 4) {
			/*******************************/
			/* Test if digital I/O counter */
			/*******************************/

			if ((devpriv->s_BoardInfos.
					dw_MolduleConfiguration[b_ModulNbr] &
					0xFFFF0000UL) == APCI1710_DIGITAL_IO) {
				/**********************************************/
				/* Test if the digital I/O module initialised */
				/**********************************************/

				if (devpriv->s_ModuleInfo[b_ModulNbr].
					s_DigitalIOInfo.b_DigitalInit == 1) {
					/**************************/
					/* Read all digital input */
					/**************************/

					/* INPDW (ps_APCI1710Variable->
					 * s_Board [b_BoardHandle].
					 * s_BoardInfos.
					 * ui_Address + (64 * b_ModulNbr),
					 * &dw_StatusReg);
					 */

					dw_StatusReg =
						inl(devpriv->s_BoardInfos.
						ui_Address + (64 * b_ModulNbr));
					*pb_PortValue =
						(unsigned char) (dw_StatusReg ^ 0x1C);

				} else {
					/*******************************/
					/* Digital I/O not initialised */
					/*******************************/

					i_ReturnValue = -4;
				}
			} else {
				/******************************************/
				/* The module is not a digital I/O module */
				/******************************************/

				i_ReturnValue = -3;
			}
		} else {
	   /***********************/
			/* Module number error */
	   /***********************/

			i_ReturnValue = -2;
		}

		break;

	case APCI1710_OUTPUT:
	/**************************/
		/* Test the module number */
	/**************************/

		if (b_ModulNbr < 4) {
	   /*******************************/
			/* Test if digital I/O counter */
	   /*******************************/

			if ((devpriv->s_BoardInfos.
					dw_MolduleConfiguration[b_ModulNbr] &
					0xFFFF0000UL) == APCI1710_DIGITAL_IO) {
	      /**********************************************/
				/* Test if the digital I/O module initialised */
	      /**********************************************/

				if (devpriv->s_ModuleInfo[b_ModulNbr].
					s_DigitalIOInfo.b_DigitalInit == 1) {
		 /***********************/
					/* Test the port value */
		 /***********************/

					if (b_PortValue <= 7) {
		    /***********************************/
						/* Test the digital output channel */
		    /***********************************/

		    /**************************/
						/* Test if channel A used */
		    /**************************/

						if ((b_PortValue & 2) == 2) {
							if (devpriv->
								s_ModuleInfo
								[b_ModulNbr].
								s_DigitalIOInfo.
								b_ChannelAMode
								!= 1) {
			  /*******************************************/
								/* The digital channel A is used for input */
			  /*******************************************/

								i_ReturnValue =
									-6;
							}
						}	/*  if ((b_PortValue & 2) == 2) */

						/**************************/
						/* Test if channel B used */
						/**************************/

						if ((b_PortValue & 4) == 4) {
							if (devpriv->
								s_ModuleInfo
								[b_ModulNbr].
								s_DigitalIOInfo.
								b_ChannelBMode
								!= 1) {
								/*******************************************/
								/* The digital channel B is used for input */
								/*******************************************/

								i_ReturnValue =
									-7;
							}
						}	/*  if ((b_PortValue & 4) == 4) */

						/***********************/
						/* Test if error occur */
						/***********************/

						if (i_ReturnValue >= 0) {

							/* if(data[1]) { */

							switch (b_PortOnOFF) {
								/*********************************/
								/* Test if set Port ON                   */
								/*********************************/

							case APCI1710_ON:

								/*********************************/
								/* Test if output memory enabled */
								/*********************************/

								if (devpriv->
									s_ModuleInfo
									[b_ModulNbr].
									s_DigitalIOInfo.
									b_OutputMemoryEnabled
									== 1) {
									dw_WriteValue
										=
										devpriv->
										s_ModuleInfo
										[b_ModulNbr].
										s_DigitalIOInfo.
										dw_OutputMemory
										|
										b_PortValue;

									devpriv->
										s_ModuleInfo
										[b_ModulNbr].
										s_DigitalIOInfo.
										dw_OutputMemory
										=
										dw_WriteValue;
								} else {
									dw_WriteValue
										=
										b_PortValue;
								}
								break;

								/*  If Set PORT  OFF */
							case APCI1710_OFF:

			   /*********************************/
								/* Test if output memory enabled */
		       /*********************************/

								if (devpriv->
									s_ModuleInfo
									[b_ModulNbr].
									s_DigitalIOInfo.
									b_OutputMemoryEnabled
									== 1) {
									dw_WriteValue
										=
										devpriv->
										s_ModuleInfo
										[b_ModulNbr].
										s_DigitalIOInfo.
										dw_OutputMemory
										&
										(0xFFFFFFFFUL
										-
										b_PortValue);

									devpriv->
										s_ModuleInfo
										[b_ModulNbr].
										s_DigitalIOInfo.
										dw_OutputMemory
										=
										dw_WriteValue;
								} else {
									/*****************************/
									/* Digital Output Memory OFF */
									/*****************************/

									i_ReturnValue
										=
										-8;
								}
							}	/*  switch */

							/*******************/
							/* Write the value */
							/*******************/

							/* OUTPDW (ps_APCI1710Variable->
							 * s_Board [b_BoardHandle].
							 * s_BoardInfos.
							 * ui_Address + (64 * b_ModulNbr),
							 * dw_WriteValue); */

							outl(dw_WriteValue,
								devpriv->
								s_BoardInfos.
								ui_Address +
								(64 * b_ModulNbr));
						}
					} else {
						/**********************/
						/* Output value wrong */
						/**********************/

						i_ReturnValue = -4;
					}
				} else {
					/*******************************/
					/* Digital I/O not initialised */
					/*******************************/

					i_ReturnValue = -5;
				}
			} else {
	      /******************************************/
				/* The module is not a digital I/O module */
	      /******************************************/

				i_ReturnValue = -3;
			}
		} else {
	   /***********************/
			/* Module number error */
	   /***********************/

			i_ReturnValue = -2;
		}
		break;

	default:
		i_ReturnValue = -9;
		DPRINTK("NO INPUT/OUTPUT specified\n");
	}			/* switch INPUT / OUTPUT */
	return i_ReturnValue;
}
