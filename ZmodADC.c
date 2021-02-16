/************************************************************************/
/*                                                                      */
/*  ZmodADC.c - ZmodADC functions implementation                        */
/*                                                                      */
/************************************************************************/
/*  Author: Michael T. Alexander                                        */
/*  Copyright 2020, Digilent Inc.                                       */
/************************************************************************/
/*  Module Description:                                                 */
/*                                                                      */
/*  This source file contains the implementation of functions that can  */
/*  be used to calculate and display the calibration constants          */
/*  associated with a Zmod ADC.                                         */
/*                                                                      */
/*  Note: the code for calculating the coefficients was provided by     */
/*  Tudor Gherman.                                                      */
/*                                                                      */
/************************************************************************/
/*  Revision History:                                                   */
/*                                                                      */
/*  01/13/2020 (MichaelA): created                                      */
/*  01/14/2020 (MichaelA): modified the way that coefficients are       */
/*      displayed based on feedback                                     */
/*  01/15/2020 (MichaelA): modified FDisplayZmodADCCal to display the   */
/*      raw calibration constants as retrieved from flash               */
/*                                                                      */
/************************************************************************/

/* ------------------------------------------------------------ */
/*              Include File Definitions                        */
/* ------------------------------------------------------------ */

#if defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/i2c-dev.h>
#endif
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "stdtypes.h"
#include "syzygy.h"
#include "ZmodADC.h"

/* ------------------------------------------------------------ */
/*              Miscellaneous Declarations                      */
/* ------------------------------------------------------------ */

#define ADC1410_IDEAL_RANGE_ADC_HIGH    1.0
#define ADC1410_IDEAL_RANGE_ADC_LOW     25.0
#define ADC1410_REAL_RANGE_ADC_HIGH     1.086
#define ADC1410_REAL_RANGE_ADC_LOW      26.25

/* ------------------------------------------------------------ */
/*              Local Type Definitions                          */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Global Variables                                */
/* ------------------------------------------------------------ */

extern BOOL dpmutilfVerbose;

/* ------------------------------------------------------------ */
/*              Local Variables                                 */
/* ------------------------------------------------------------ */


/* ------------------------------------------------------------ */
/*              Forward Declarations                            */
/* ------------------------------------------------------------ */

int32_t ComputeMultCoefADC1410(float cg, BOOL fHighGain);
int32_t ComputeAddCoefADC1410(float ca, BOOL fHighGain);

/* ------------------------------------------------------------ */
/*              Procedure Definitions                           */
/* ------------------------------------------------------------ */

/* ------------------------------------------------------------ */
/***    FDisplayZmodADCCal
**
**  Parameters:
**  	fdI2cDev        - open file descriptor for underlying I2C device (linux only)
**      addrI2cSlave    - I2C bus address for the slave
**
**  Return Value:
**      fTrue for success, fFalse otherwise
**
**  Errors:
**      none
**
**  Description:
**      This function reads the factory calibration and user calibration
**      areas from the ZmodADC with the specified I2C bus address,
**      computes the multiplicative and additive coefficients, and then
**      displays them using stdout.
*/
BOOL
FDisplayZmodADCCal(int fdI2cDev, BYTE addrI2cSlave) {

    ZMOD_ADC_CAL    adcal;
    WORD            cbRead;
    time_t          t;
    struct tm       time;
    char            szDate[256];

    if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrAdcFactCalStart, (BYTE*)&adcal, sizeof(ZMOD_ADC_CAL), &cbRead) ) {
        printf("Error: failed to read ZmodADC factory calibration from 0x%02X\n", addrI2cSlave);
        printf("Error: received %d of %d bytes\n", cbRead, sizeof(ZMOD_ADC_CAL));
        return fFalse;
    }

    t = (time_t)adcal.date;
    localtime_r(&t, &time);
    if ( 0 != strftime(szDate, sizeof(szDate), "%B %d, %Y at %T", &time) ) {
        printf("\n    Factory Calibration:   %s\n", szDate);
    }

    printf("    CHAN_1_LG_GAIN:        %f\n", adcal.cal[0][0][0]);
    printf("    CHAN_1_LG_OFFSET:      %f\n", adcal.cal[0][0][1]);
    printf("    CHAN_1_HG_GAIN:        %f\n", adcal.cal[0][1][0]);
    printf("    CHAN_1_HG_OFFSET:      %f\n", adcal.cal[0][1][1]);
    printf("    CHAN_2_LG_GAIN:        %f\n", adcal.cal[1][0][0]);
    printf("    CHAN_2_LG_OFFSET:      %f\n", adcal.cal[1][0][1]);
    printf("    CHAN_2_HG_GAIN:        %f\n", adcal.cal[1][1][0]);
    printf("    CHAN_2_HG_OFFSET:      %f\n", adcal.cal[1][1][1]);

    printf("    Ch1LgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[0][0][0], fFalse));
    printf("    Ch1LgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[0][0][1], fFalse));
    printf("    Ch1HgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[0][1][0], fTrue));
    printf("    Ch1HgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[0][1][1], fTrue));
    printf("    Ch2LgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[1][0][0], fFalse));
    printf("    Ch2LgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[1][0][1], fFalse));
    printf("    Ch2HgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[1][1][0], fTrue));
    printf("    Ch2HgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[1][1][1], fTrue));

    if ( ! SyzygyI2cRead(fdI2cDev, addrI2cSlave, addrAdcUserCalStart, (BYTE*)&adcal, sizeof(ZMOD_ADC_CAL), &cbRead) ) {
        printf("Error: failed to read ZmodADC user calibration from 0x%02X\n", addrI2cSlave);
        printf("Error: received %d of %d bytes\n", cbRead, sizeof(ZMOD_ADC_CAL));
        return fFalse;
    }

    t = (time_t)adcal.date;
    localtime_r(&t, &time);
    if ( 0 != strftime(szDate, sizeof(szDate), "%B %d, %Y at %T", &time) ) {
        printf("\n    User Calibration:      %s\n", szDate);
    }

    printf("    CHAN_1_LG_GAIN:        %f\n", adcal.cal[0][0][0]);
    printf("    CHAN_1_LG_OFFSET:      %f\n", adcal.cal[0][0][1]);
    printf("    CHAN_1_HG_GAIN:        %f\n", adcal.cal[0][1][0]);
    printf("    CHAN_1_HG_OFFSET:      %f\n", adcal.cal[0][1][1]);
    printf("    CHAN_2_LG_GAIN:        %f\n", adcal.cal[1][0][0]);
    printf("    CHAN_2_LG_OFFSET:      %f\n", adcal.cal[1][0][1]);
    printf("    CHAN_2_HG_GAIN:        %f\n", adcal.cal[1][1][0]);
    printf("    CHAN_2_HG_OFFSET:      %f\n", adcal.cal[1][1][1]);

    printf("    Ch1LgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[0][0][0], fFalse));
    printf("    Ch1LgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[0][0][1], fFalse));
    printf("    Ch1HgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[0][1][0], fTrue));
    printf("    Ch1HgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[0][1][1], fTrue));
    printf("    Ch2LgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[1][0][0], fFalse));
    printf("    Ch2LgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[1][0][1], fFalse));
    printf("    Ch2HgCoefMultStatic:   0x%05X\n", (unsigned int)ComputeMultCoefADC1410(adcal.cal[1][1][0], fTrue));
    printf("    Ch2HgCoefAddStatic:    0x%05X\n", (unsigned int)ComputeAddCoefADC1410(adcal.cal[1][1][1], fTrue));

    return fTrue;
}

/* ------------------------------------------------------------ */
/***    ComputeMultCoefADC1410
**
**  Parameters:
**      cg              - gain coefficient from ZmodADC flash memory
**      fHighGain       - fTrue for high gain setting, fFalse for low gain
**                        read
**
**  Return Value:
**      signed 32-bit value containing the multiplicative coefficient in the
**      18 least significant bits: bit 17 is the sign, bits 16:0 are the value
**
**  Errors:
**      none
**
**  Description:
**      This function computes a signed 18-bit value corresponding to the
**      multiplicative calibration coefficient of the ZmodADC.
*/
int32_t
ComputeMultCoefADC1410(float cg, BOOL fHighGain) {

    float   fval;
    int32_t ival;

    fval = (fHighGain ? (ADC1410_REAL_RANGE_ADC_HIGH/ADC1410_IDEAL_RANGE_ADC_HIGH):(ADC1410_REAL_RANGE_ADC_LOW/ADC1410_IDEAL_RANGE_ADC_LOW))*(1 + cg)*(float)(1<<16);
    ival = (int32_t)(fval + 0.5);  // round
    ival &= (1<<18) - 1; // keep only 18 bits

    return ival;
}

/* ------------------------------------------------------------ */
/***    ComputeAddCoefADC1410
**
**  Parameters:
**      ca              - additive coefficient from ZmodADC flash memory
**      fHighGain       - fTrue for high gain setting, fFalse for low gain
**                        read
**
**  Return Value:
**      signed 32-bit value containing the additive coefficient in the 18
**      least significant bits: bit 17 is the sign, bits 16:0 are the value
**
**  Errors:
**      none
**
**  Description:
**      This function computes a signed 18-bit value corresponding to the
**      additive calibration coefficient of the ZmodADC.
*/
int32_t
ComputeAddCoefADC1410(float ca, BOOL fHighGain) {

    float   fval;
    int32_t ival;

    fval = ca / (fHighGain ? ADC1410_IDEAL_RANGE_ADC_HIGH:ADC1410_IDEAL_RANGE_ADC_LOW)*(float)(1<<17);
    ival = (int32_t)(fval + 0.5);  // round
    ival &= (1<<18) - 1; // keep only 18 bits

    return ival;
}

