#if	defined(OSS_PLAYER)
/*
 * Copyright 1997 Chris Bagwell And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained.
 * Chris Bagwell And Sundry Contributors are not
 * responsible for the consequences of using this software.
 */

/* Direct to Open Sound System (OSS) sound driver
 * OSS is a popular unix sound driver for Intel x86 unices (eg. Linux)
 * and several other unixes (such as SunOS/Solaris).
 * This driver is compatible with OSS original source that was called
 * USS, Voxware and TASD.
 *
 * added by Chris Bagwell (cbagwell@sprynet.com) on 2/19/96
 * based on info grabed from vplay.c in Voxware snd-utils-3.5 package.
 * and on LINUX_PLAYER patches added by Greg Lee
 * which was originally from Directo to Sound Blaster device driver (sbdsp.c).
 * SBLAST patches by John T. Kohl.
 */

#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "st.h"
#include "libst.h"

/*
 * Do anything required before you start reading samples.
 * Read file header.
 *	Find out sampling rate,
 *	size and style of samples,
 *	mono/stereo/quad.
 */
void ossdspstartread(ft)
ft_t ft;
{
    int tmp;
    int samplesize = 8, dsp_stereo;

    if (ft->info.rate == 0.0) ft->info.rate = 8000;
    if (ft->info.size == -1) ft->info.size = BYTE;
    if (ft->info.size == BYTE) {
	samplesize = 8;
	if (ft->info.style == -1)
	    ft->info.style = UNSIGNED;
	if (ft->info.style != UNSIGNED) {
	    fail("OSS driver only supports unsigned with bytes");
	}
    }
    else if (ft->info.size == WORD) {
	samplesize = 16;
	if (ft->info.style == -1)
	    ft->info.style = SIGN2;
	if (ft->info.style != SIGN2) {
	    fail("OSS driver only supports signed with words");
	}
    }
    else {
	fail("OSS driver only supports bytes and words");
    }

    if (ft->info.channels == -1) ft->info.channels = 1;
    else if (ft->info.channels > 2) ft->info.channels = 2;

    ioctl(fileno(ft->fp), SNDCTL_DSP_RESET, 0);
    ioctl (fileno(ft->fp), SNDCTL_DSP_GETBLKSIZE, &ft->file.size);
    if (ft->file.size < 4 || ft->file.size > 65536) {
	fail("Invalid audio buffer size %d", ft->file.size);
    }
    ft->file.count = 0;
    ft->file.pos = 0;
    ft->file.eof = 0;

    if ((ft->file.buf = malloc (ft->file.size)) == NULL) {
	fail("Unable to allocate input/output buffer of size %d", ft->file.size);
    }

    if (ioctl(fileno(ft->fp), SNDCTL_DSP_SYNC, NULL) < 0) {
	fail("Unable to sync dsp");
    }

    tmp = samplesize;
    ioctl(fileno(ft->fp), SNDCTL_DSP_SAMPLESIZE, &tmp);
    if (tmp != samplesize) {
	fail("Unable to set the sample size to %d", samplesize);
    }

    if (ft->info.channels == 2) dsp_stereo = 1;
    else dsp_stereo = 0;

    tmp = dsp_stereo;
    ioctl(fileno(ft->fp), SNDCTL_DSP_STEREO, &tmp);
    if (tmp != dsp_stereo) {
	ft->info.channels = 1;
	warn("Couldn't set to %s", dsp_stereo?  "stereo":"mono");
	dsp_stereo = 0;
    }

    tmp = ft->info.rate;
    ioctl (fileno(ft->fp), SNDCTL_DSP_SPEED, &tmp);
    if (ft->info.rate != tmp) {
	if (ft->info.rate - tmp > tmp/10 || tmp - ft->info.rate > tmp/10)
	    warn("Unable to set audio speed to %d (set to %d)",
		     ft->info.rate, tmp);
	ft->info.rate = tmp;
    }

    sigintreg(ft);	/* Prepare to catch SIGINT */
}

void ossdspstartwrite(ft)
ft_t ft;
{
    int samplesize = 8, dsp_stereo;
    int tmp;

    if (ft->info.rate == 0.0) ft->info.rate = 8000;
    if (ft->info.size == -1) ft->info.size = BYTE;
    if (ft->info.size == BYTE) {
	samplesize = 8;
	if (ft->info.style == -1)
	    ft->info.style = UNSIGNED;
	if (ft->info.style != UNSIGNED) {
	    report("OSS driver only supports unsigned with bytes");
	    report("Forcing to unsigned");
	    ft->info.style = UNSIGNED;
	}
    }
    else if (ft->info.size == WORD) {
	samplesize = 16;
	if (ft->info.style == -1)
	    ft->info.style = SIGN2;
	if (ft->info.style != SIGN2) {
	    report("OSS driver only supports signed with words");
	    report("Forcing to signed linear");
	    ft->info.style = SIGN2;
	}
    }
    else {
        ft->info.size = WORD;
	ft->info.style = SIGN2;
	report("OSS driver only supports bytes and words");
	report("Forcing to signed linear word");
    }

    if (ft->info.channels == -1) ft->info.channels = 1;
    else if (ft->info.channels > 2) ft->info.channels = 2;

    ioctl(fileno(ft->fp), SNDCTL_DSP_RESET, 0);
    ioctl (fileno(ft->fp), SNDCTL_DSP_GETBLKSIZE, &ft->file.size);
    if (ft->file.size < 4 || ft->file.size > 65536) {
	    fail("Invalid audio buffer size %d", ft->file.size);
    }
    ft->file.count = 0;
    ft->file.pos = 0;
    ft->file.eof = 0;

    if ((ft->file.buf = malloc (ft->file.size)) == NULL) {
	fail("Unable to allocate input/output buffer of size %d", ft->file.size);
    }

    if (ioctl(fileno(ft->fp), SNDCTL_DSP_SYNC, NULL) < 0) {
	fail("Unable to sync dsp");
    }

    tmp = samplesize;
    ioctl(fileno(ft->fp), SNDCTL_DSP_SAMPLESIZE, &tmp);
    if (tmp != samplesize) {
	fail("Unable to set the sample size to %d", samplesize);
    }

    if (ft->info.channels == 2) dsp_stereo = 1;
    else dsp_stereo = 0;

    tmp = dsp_stereo;
    ioctl(fileno(ft->fp), SNDCTL_DSP_STEREO, &tmp);
    if (tmp != dsp_stereo) {
	ft->info.channels = 1;
	warn("Couldn't set to %s", dsp_stereo?  "stereo":"mono");
	dsp_stereo = 0;
    }

    tmp = ft->info.rate;
    ioctl (fileno(ft->fp), SNDCTL_DSP_SPEED, &tmp);
    if (ft->info.rate != tmp) {
	if (ft->info.rate - tmp > tmp/10 || tmp - ft->info.rate > tmp/10)
	    warn("Unable to set audio speed to %d (set to %d)",
		     ft->info.rate, tmp);
	ft->info.rate = tmp;
    }
}

#endif
