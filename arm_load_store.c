/*
Armator - simulateur de jeu d'instruction ARMv5T � but p�dagogique
Copyright (C) 2011 Guillaume Huard
Ce programme est libre, vous pouvez le redistribuer et/ou le modifier selon les
termes de la Licence Publique G�n�rale GNU publi�e par la Free Software
Foundation (version 2 ou bien toute autre version ult�rieure choisie par vous).

Ce programme est distribu� car potentiellement utile, mais SANS AUCUNE
GARANTIE, ni explicite ni implicite, y compris les garanties de
commercialisation ou d'adaptation dans un but sp�cifique. Reportez-vous � la
Licence Publique G�n�rale GNU pour plus de d�tails.

Vous devez avoir re�u une copie de la Licence Publique G�n�rale GNU en m�me
temps que ce programme ; si ce n'est pas le cas, �crivez � la Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
�tats-Unis.

Contact: Guillaume.Huard@imag.fr
         ENSIMAG - Laboratoire LIG
         51 avenue Jean Kuntzmann
         38330 Montbonnot Saint-Martin
*/

#include "arm_load_store.h"
#include "arm_exception.h"
#include "arm_constants.h"
#include "util.h"
#include "debug.h"
#include <assert.h>

///////////////////////////////////////////////////////////////////////////////
// Available instructions
///////////////////////////////////////////////////////////////////////////////

static int ldr(arm_core p, uint8_t rd, uint32_t address) {
	debug("LDR rd:r%d %x\n", rd, address);
    uint32_t value;
    int result = arm_read_word(p, address, &value);
    if (result == NO_EXCEPTION) {
        if (rd == PC) {
            update_flag_t(p, get_bit(value, 0));
            value &= 0xFFFFFFFE;
        }
        arm_write_register(p, rd, value);
    }
    return result;
}

static int str(arm_core p, uint8_t rd, uint32_t address) {
	debug("STR rd:r%d %x\n", rd, address);
    uint32_t value = arm_read_register(p, rd);
    return arm_write_word(p, address, value);
}

static int ldrb(arm_core p, uint8_t rd, uint32_t address) {
	debug("LDRB rd:r%d %x\n", rd, address);
    uint8_t value;
    int result = arm_read_byte(p, address, &value);
    if (result == NO_EXCEPTION)
        arm_write_register(p, rd, value);
    return result;
}

static int strb(arm_core p, uint8_t rd, uint32_t address) {
	debug("STRB rd:r%d %x\n", rd, address);
    uint8_t value = (arm_read_register(p, rd) & 0xFF);
    return arm_write_byte(p, address, value);
}

static int ldrt(arm_core p, uint8_t rd, uint32_t address) {
	debug("LDRT rd:r%d %x\n", rd, address);
    uint32_t value;
    int result = arm_read_word(p, address, &value);
    if (result == NO_EXCEPTION)
        arm_write_register(p, rd, value);
    return result;
}

static int strt(arm_core p, uint8_t rd, uint32_t address) {
	debug("STRT rd:r%d %x\n", rd, address);
    uint32_t value = arm_read_register(p, rd);
    return arm_write_word(p, address, value);
}

static int ldrbt(arm_core p, uint8_t rd, uint32_t address) {
	debug("LBRBT rd:r%d %x\n", rd, address);
    uint8_t value;
    int result = arm_read_byte(p, address, &value);
    if (result == NO_EXCEPTION)
        arm_write_register(p, rd, (uint32_t)value);
    return result;
}

static int strbt(arm_core p, uint8_t rd, uint32_t address) {
	debug("STRBT rd:r%d %x\n", rd, address);
    uint8_t value;
    int result = arm_read_byte(p, address, &value);
    if (result == NO_EXCEPTION)
        arm_write_register(p, rd, (uint32_t)value);
    return result;
}

static int ldrh(arm_core p, uint8_t rd, uint32_t address) {
	debug("LDRH rd:r%d %x\n", rd, address);
    uint16_t value;
    int result = arm_read_half(p, address, &value);
    if (result == NO_EXCEPTION)
        arm_write_register(p, rd, (uint32_t)value);
    return result;
}

static int strh(arm_core p, uint8_t rd, uint32_t address) {
	debug("STRH rd:r%d %x\n", rd, address);
    uint16_t value = (arm_read_register(p, rd) & 0xFFFF);
    return arm_write_half(p, address, value);
}

static int ldrsh(arm_core p, uint8_t rd, uint32_t address) {
	debug("LDRSH rd:r%d %x\n", rd, address);
    uint32_t value;
    int result = arm_read_half(p, address, (uint16_t*)&value);
    if (result == NO_EXCEPTION) {
        if (value & (1<<15)) {
            value |= 0xFFFF0000; 
        } else {
            value &= 0X0000FFFFF;
        } 
        arm_write_register(p, rd, value);
    }
    return result;
}

static int ldrsb(arm_core p, uint8_t rd, uint32_t address) {
	debug("LDRSB rd:r%d %x\n", rd, address);
    uint8_t value;
    int result = arm_read_byte(p, address, &value);
    if (result == NO_EXCEPTION) {
        if (value & (1<<7)) {
            value |= 0xFFFFFF00; 
        } else {
            value &= 0X0000F00FF;
        }
        arm_write_register(p, rd, (uint32_t)value);
    }
    return result;
}

static int ldrd(arm_core p, uint8_t rd, uint32_t address) {
	debug("LDRD rd:r%d %x\n", rd, address);
    if (is_odd(rd) || (rd == LR) || (address & 3)) {
        UNPREDICTABLE();
        return 0;
    }
    uint32_t value;
    int result = arm_read_word(p, address, &value);
    if (result == NO_EXCEPTION) arm_write_register(p, rd, value);
    if (result == NO_EXCEPTION) result = arm_read_word(p, address+4, &value);
    if (result == NO_EXCEPTION) arm_write_register(p, rd+1, value);
    return result;
}

static int strd(arm_core p, uint8_t rd, uint32_t address) {
	debug("STRD rd:r%d %x\n", rd, address);
    if (is_odd(rd) || (rd == 14) || (address & 3) || (address & 4)) {
        UNPREDICTABLE();
        return 0;
    }
    uint32_t value = arm_read_register(p, rd);
    int result = arm_write_word(p, address, value);
    if (result == NO_EXCEPTION) {
        value = arm_read_register(p, rd+1);
        result = arm_write_word(p, address+4, value);
    }
    return result;
}

static int ldm1(arm_core p, int16_t r_list, uint32_t s_add, uint32_t e_add) {
	debug("LDM1 rdList:%x startAdd:%x endAdd:%x\n", r_list, s_add, e_add);
    uint32_t address = s_add;
    int i, result = 0;
    uint32_t value;
    
    for (i=0; i<=14 && result == NO_EXCEPTION; i++) {
        if (get_bit(r_list, i)) {
            result = arm_read_word(p, address, &value);
            if (result == NO_EXCEPTION) arm_write_register(p, i, value);
            address += 4;
        }
    }

    if (result == NO_EXCEPTION && get_bit(r_list, 15)) {
        result = arm_read_word(p, address, &value);
        update_flag_t(p, get_bit(value, 0));
        value &= 0xFFFFFFFE;
        arm_write_register(p, PC, value);
        address += 4;
    }

    assert(e_add == (address - 4));
    return result;
}

static int stm1(arm_core p, int16_t r_list, uint32_t s_add, uint32_t e_add) {
	debug("STM2 rdList:%x startAdd:%x endAdd:%x\n", r_list, s_add, e_add);
    uint32_t address = s_add;
    int i, result = 0;
    uint32_t value;
    
    for (i=0; i<=15 && result == NO_EXCEPTION; i++) {
        if (get_bit(r_list, i)) {
            value = arm_read_register(p, i);
            result = arm_write_word(p, address, value);
            address += 4;
        }
    }

    assert(e_add == (address - 4));
    return result;
}

static int ldm2(arm_core p, int16_t r_list, uint32_t s_add, uint32_t e_add) {
	debug("LDM2 rdList:%x startAdd:%x endAdd:%x\n", r_list, s_add, e_add);
    uint32_t address = s_add;
    int i, result = 0;
    uint32_t value;

    for (i=0; i<=14 && result == NO_EXCEPTION; i++) {
        if (get_bit(r_list, i)) {
            result = arm_read_word(p, address, &value);
            if (result == NO_EXCEPTION) arm_write_usr_register(p, i, value);
            address += 4;
        }
    }

    assert(e_add == (address - 4));
    return result;
}

static int stm2(arm_core p, int16_t r_list, uint32_t s_add, uint32_t e_add) {
	debug("STM2 rdList:%x startAdd:%x endAdd:%x\n", r_list, s_add, e_add);
    uint32_t address = s_add;
    int i, result = 0;
    uint32_t value;

    for (i=0; i<=15 && result == NO_EXCEPTION; i++) {
        if (get_bit(r_list, i)) {
            value = arm_read_usr_register(p, i);
            result = arm_write_word(p, address, value);
            address += 4;
        }
    }

    assert(e_add == (address - 4));
    return result;
}

static int ldm3(arm_core p, int16_t r_list, uint32_t s_add, uint32_t e_add) {
	debug("LDM3 rdList:%x startAdd:%x endAdd:%x\n", r_list, s_add, e_add);
    uint32_t address = s_add;
    int i, result = 0;
    uint32_t value;

    for (i=0; i<=14 && result == NO_EXCEPTION; i++) {
        if (get_bit(r_list, i)) {
            result = arm_read_word(p, address, &value);
            if (result == NO_EXCEPTION) arm_write_register(p, i, value);
            address += 4;
        }
    }

    if (result == NO_EXCEPTION) {
        if (arm_current_mode_has_spsr(p)) {
            arm_write_cpsr(p, arm_read_spsr(p));
        } else {
            UNPREDICTABLE();
        }
        result = arm_read_word(p, address, &value);
        if (result == NO_EXCEPTION) arm_write_register(p, 15, value);
        address += 4;
    }
    assert(e_add == (address - 4));
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// Offset
///////////////////////////////////////////////////////////////////////////////

uint32_t scaled_shift(arm_core p, uint8_t shift, uint8_t shift_imm, 
                        uint32_t val_rm) {
    uint32_t index = 0, cpsr;
    switch (shift) {
        case 0: /* LSL */
            index = val_rm << shift_imm;
            break;
        case 1: /* LSR */
            if (shift_imm) index = val_rm >> shift_imm;
            break;
        case 2: /* ASR */
            if (shift_imm) {
                index = asr(val_rm, shift_imm);
            } else if (get_bit(val_rm, 31)) {
                index = 0xFFFFFFFF;
            }
            break;
        case 3: /* ROR or RRX */
            if (shift_imm == 0) { /* RRX */
                cpsr = arm_read_cpsr(p);
                index = (get_bit(cpsr, C) << 31) | (val_rm >> 1);
            } else { /* ROR */
                index = ror(val_rm, shift_imm);
            }
            break;
    }
    return index;
}

///////////////////////////////////////////////////////////////////////////////
// Parsing of different classes
///////////////////////////////////////////////////////////////////////////////

// LDR, STR, LDRB, STRB, LDRT, STRT, LDRBT, STRBT
int arm_load_store(arm_core p, uint32_t ins) {
    debug("arm load store\n");

    uint32_t address;
    int index, result;
    uint8_t rd = get_bits(ins, 15, 12);
    uint8_t rn = get_bits(ins, 19, 16);
    int val_rn = arm_read_register(p, rn);

    if (!get_bit(ins, 25)) { // Immmediate
        uint32_t offset = get_bits(ins, 11, 0);
        if (get_bit(ins, 24) && !get_bit(ins, 21)) {
			debug("immediate offset\n");
            address = (get_bit(ins, 23)) ? val_rn + offset : val_rn - offset;
        } else if (get_bit(ins, 24) && get_bit(ins, 21)) {
			debug("immediate pre indexed\n");
            address = (get_bit(ins, 23)) ? val_rn + offset : val_rn - offset;
            arm_write_register(p, rn, address);
        } else if (!get_bit(ins, 24) && !get_bit(ins, 21)){
			debug("immediate post indexed\n");
            address = val_rn;
            val_rn = (get_bit(ins, 23)) ? val_rn + offset : val_rn - offset;
            arm_write_register(p, rn, val_rn);
        } else {
        	return UNDEFINED_INSTRUCTION;
        }
    } else { // Registers
        int rm = get_bits(ins, 3, 0);
        int val_rm = arm_read_register(p, rm);
        int shift_val = get_bits(ins, 6, 5);
        int shift_imm = get_bits(ins, 11, 7);

        if (get_bit(ins, 24) && !get_bit(ins, 21)) {
        	debug("reg scaled offset\n");
            index = scaled_shift(p, shift_val, shift_imm, val_rm);
            address = (get_bit(ins, 23)) ? val_rn + index : val_rn - index;
        } else if (get_bit(ins, 24) && get_bit(ins, 21)) {
        	debug("reg pre indexed\n");
            index = scaled_shift(p, shift_val, shift_imm, val_rm);
            address = (get_bit(ins, 23)) ? val_rn + index : val_rn - index;
            arm_write_register(p, rn, address);
        } else if (!get_bit(ins, 24) && !get_bit(ins, 21)) {
        	debug("post indexed\n");
            address = val_rn;
            index = scaled_shift(p, shift_val, shift_imm, val_rm);
            val_rn = (get_bit(ins, 23)) ? val_rn + index : val_rn - index;
            arm_write_register(p, rn, val_rn);
        } else {
        	return UNDEFINED_INSTRUCTION;
        }
    }
    
    switch (codage4_bits(ins, 24, 22, 21, 20)) {
        case 0 : result = str(p, rd, address);   break;
        case 1 : result = ldr(p, rd, address);   break;
        case 2 : result = strt(p, rd, address);  break;
        case 3 : result = ldrt(p, rd, address);  break;
        case 4 : result = strb(p, rd, address);  break;
        case 5 : result = ldrb(p, rd, address);  break;
        case 6 : result = strbt(p, rd, address); break;
        case 7 : result = ldrbt(p, rd, address); break;
        case 8 : result = str(p, rd, address);   break;
        case 9 : result = ldr(p, rd, address);   break;
        case 10: result = str(p, rd, address);   break;
        case 11: result = ldr(p, rd, address);   break;
        case 12: result = strb(p, rd, address);  break;
        case 13: result = ldrb(p, rd, address);  break;
        case 14: result = strb(p, rd, address);  break;
        case 15: result = ldrb(p, rd, address);  break;
        default: result =  0; break; // impossible
    }
    return result;
}

// LDRH, STRH, LDRSH, STRSH, LDRSB, STRSB, LDRD, STRD
int arm_load_store_miscellaneous(arm_core p, uint32_t ins) {
    debug("load store miscellaneous\n");

    uint32_t address;
    uint8_t rd = get_bits(ins, 15, 12);
    uint8_t rn = get_bits(ins, 19, 16);
    int val_rn = arm_read_register(p, rn);
    int offset_type = codage2_bits(ins, 24, 21);
    int offset, result;

    if (get_bit(ins, 22)) { // immediate
        offset = (get_bits(ins, 11, 8) << 4) | get_bits(ins, 3, 0);
    } else { // register
        uint8_t rm = get_bits(ins, 3, 0);
        offset = arm_read_register(p, rm);
    }

    if (offset_type == 0) { // post_indexed
        address = val_rn;
        val_rn = (get_bit(ins, 23)) ? val_rn + offset : val_rn - offset;
        arm_write_register(p, rn, val_rn);
    } else if (offset_type == 3) { // pre_indexed
        address = (get_bit(ins, 23)) ? val_rn + offset : val_rn - offset;
        arm_write_register(p, rn, address);
    } else if (offset_type == 2) { // offset
        address = (get_bit(ins, 23)) ? val_rn + offset : val_rn - offset;
    } else {
    	return UNDEFINED_INSTRUCTION;	
    }

    switch (codage3_bits(ins, 20, 6, 5)) {
        case 1 : result = strh(p, rd, address); break;
        case 2 : result = ldrd(p, rd, address); break;
        case 3 : result = strd(p, rd, address); break;
        case 5 : result = ldrh(p, rd, address); break;
        case 6 : result = ldrsb(p, rd, address); break;
        case 7 : result = ldrsh(p, rd, address); break;
        default: result =  0; break; // impossible
    }
    return result;
}

// LDM(1), STM(1), LDM(2), STM(2), LDM(3), STM(3)
int arm_load_store_multiple(arm_core p, uint32_t ins) {
    debug("arm load store multiple\n");

    int result;
    uint8_t rn = get_bits(ins, 19, 16);
    int val_rn = arm_read_register(p, rn);
    int reg_list = get_bits(ins, 15, 0);
    int n = number_of_set_bits(reg_list);
    int32_t start_add, end_add;

    if (get_bit(ins, 23)) { // Increment
        if (get_bit(ins, 24)) { // before
            start_add = val_rn + 4;
            end_add = val_rn + (n * 4);
        } else { // after
            start_add = val_rn;
            end_add = val_rn + (n * 4) - 4;
        }
        if (get_bit(ins, 21)) {
            val_rn = val_rn + (n * 4);
            arm_write_register(p, rn, val_rn);
        }
    } else { // Decrement
        if (get_bit(ins, 24)) { // before
            start_add = val_rn - (n * 4);
            end_add = val_rn - 4;
        } else { // after
            start_add = val_rn - (n * 4) + 4;
            end_add = val_rn;
        }
        if (get_bit(ins, 21)) {
            val_rn = val_rn - (n * 4);
            arm_write_register(p, rn, val_rn);
        }
    }

    switch (codage3_bits(ins, 22, 20, 15)) {
        case 0 :
        case 1 : result = stm1(p, reg_list, start_add, end_add); break;
        case 2 :
        case 3 : result = ldm1(p, reg_list, start_add, end_add); break;
        case 4 : result = stm2(p, reg_list, start_add, end_add); break;
        case 6 : result = ldm2(p, reg_list, start_add, end_add); break;
        case 7 : result = ldm3(p, reg_list, start_add, end_add); break;
        default: result =  0; break; // impossible
    }
    return result;
}

// <opcode>{<cond>}{L} <coproc>,<CRd>,<addressing_mode>
int arm_coprocessor_load_store(arm_core p, uint32_t ins) {
    debug("arm coprocessor load store\n");
    debug("instruction is not implemented");
    return UNDEFINED_INSTRUCTION;
}

