

/* Testing/Debugging */
int cx18_v4l2_open(struct file *filp);
ssize_t cx18_v4l2_read(struct file *filp, char __user *buf, size_t count,
		      loff_t *pos);
ssize_t cx18_v4l2_write(struct file *filp, const char __user *buf, size_t count,
		       loff_t *pos);
int cx18_v4l2_close(struct file *filp);
unsigned int cx18_v4l2_enc_poll(struct file *filp, poll_table *wait);
int cx18_start_capture(struct cx18_open_id *id);
void cx18_stop_capture(struct cx18_open_id *id, int gop_end);
void cx18_mute(struct cx18 *cx);
void cx18_unmute(struct cx18 *cx);

/* Shared with cx18-alsa module */
int cx18_claim_stream(struct cx18_open_id *id, int type);
void cx18_release_stream(struct cx18_stream *s);
