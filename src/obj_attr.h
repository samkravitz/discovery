/* discovery
 * License: GPLv2
 * See LICENSE.txt for full license text
 * 
 * FILE: obj_attr.h
 * DATE: August 31st, 2020
 * DESCRIPTION: representation of Object Attribute Memory (OAM)
 */

#ifndef OBJ_ATTR_H
#define OBJ_ATTR_H

struct obj_attr {
    union attr_0 {
        u16 attr;        
    } attr_0;

    union attr_0 {
        u16 attr;
    } attr_2;

    union attr_0 {
        u16 attr;
    } attr_1;

    union attr_0 {
        u16 attr;
    } attr_3;
}


#endif // OBJ_ATTR_H