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
        struct attr {
            u8 y : 8; // y coordinate
            u8 r : 1; // rotation/scaling on/off
            u8 d : 1; // 0 sprite is single sized 1 sprite is double sized
            u8 t : 2; // 00 - normal, 01 - semi-transparent, 01 obj window, 11 illegal
            u8 m : 1; // mosaic on/off
            u8 a : 1; // 256 color if on, 16 color if off
            u8 s : 2; // see s of attr_2
        } attr;

        u16 _zero;        
    } attr_0;

    union attr_1 {
        struct attr {
            u16 x : 9; // x coordinate
            // index for rotation data
            // will be 5 wide for affine sprites, replacing h & v
            u8 f : 3;
            u8 h : 1; // horizontal flip
            u8 v : 1; // vertical flip

            /*
             * when concactinated with s from attr_0, determines shape:
             * 0000: 8  x 8         
             * 0001: 16 x 16        
             * 0010: 32 x 32        
             * 0011: 64 x 64        
             * 0100: 16 x 8 
             * 0101: 32 x 8 
             * 0110: 32 x 16
             * 0111: 64 x 32
             * 1000: 8  x 16
             * 1001: 8  x 32
             * 1010: 16 x 32
             * 1011: 32 x 64
             * 1100: Not used
             * 1101: Not used
             * 1110: Not used
             * 1111: Not used
             */
            u8 s : 2;
        } attr;

        u16 _one;
    } attr_1;

    union attr_2 {
        struct attr {
            u16 tileno : 10; // tile number
            u8 p       : 2; // priority
            u8 l       : 2; // palette number
        } attr;

        u16 _two;
    } attr_2;

    s16 fill;
};


#endif // OBJ_ATTR_H