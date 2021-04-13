/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: Timer.cpp
 * DATE: April 13th, 2021
 * DESCRIPTION: global IRQ manager
 */
#include "IRQ.h"

IRQ::IRQ()
{
    reg_ie  = 0;
    reg_if  = 0;
    reg_ime = 0;
}