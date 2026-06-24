/* package.js
 * Module namespace for ST-Link library code
 *
 * Copyright Devan Lai 2017
 *
 */

import * as usb from './stlinkusb.js';
import * as exceptions from './stlinkex.js';
import Stlinkv2 from './stlinkv2.js';
import Logger from './dbg.js';
import DEVICES from './stm32devices.js';
import * as semihosting from './semihosting.js';


import { Stm32 } from './stm32.js';
import { Stm32FP, Stm32FPXL } from './stm32fp.js';
import { Stm32FS } from './stm32fs.js';
import { Stm32L4 } from './stm32l4.js';

let drivers = {
    Stm32: Stm32,
    Stm32FP: Stm32FP,
    Stm32FPXL: Stm32FPXL,
    Stm32FS: Stm32FS,
    Stm32L4: Stm32L4
};

export { exceptions, usb, drivers, Stlinkv2, Logger, DEVICES, semihosting };
