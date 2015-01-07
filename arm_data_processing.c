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
#include "arm_instruction.h"
#include "arm_data_processing.h"
#include "arm_exception.h"
#include "arm_constants.h"
#include "arm_branch_other.h"
#include "util.h"
#include "debug.h"

// Condition check
inline uint8_t instruction_get_condition_field(uint32_t instruction) {
    return (uint8_t)(instruction>>28);
}

int instruction_check_condition(arm_core p, uint8_t field) {
    int res;
    switch(field) {
        case 0:  res = is_z_set(p);                                     break;
        case 1:  res = is_z_clear(p);                                   break;
        case 2:  res = is_c_set(p);                                     break;
        case 3:  res = is_c_clear(p);                                   break;
        case 4:  res = is_n_set(p);                                     break;
        case 5:  res = is_n_clear(p);                                   break;
        case 6:  res = is_v_set(p);                                     break;
        case 7:  res = is_v_clear(p);                                   break;
        case 8:  res = is_c_set(p) && is_z_clear(p);                    break;
        case 9:  res = is_c_clear(p) && is_z_set(p);                    break;
        case 10: res = arm_read_n(p) == arm_read_v(p);                  break;
        case 11: res = arm_read_n(p) != arm_read_v(p);                  break;
        case 12: res = is_z_set(p) && arm_read_n(p) == arm_read_v(p);   break;
        case 13: res = is_z_clear(p) && arm_read_n(p) != arm_read_v(p); break;
        case 14: res =  1; break; // always
        case 15: res = -1; break; // undefined
        default: res =  0; break; // impossible
    }

	debug("condition : %x, %d\n", field, res);
    return res;
}



typedef int(* dp_instruction_handler_t)(arm_core, uint8_t, uint32_t, uint32_t, uint8_t, uint8_t);

// Data processing instruction parsing
static inline int get_op_code(uint32_t ins) {
	return (ins >> 21) & 15;
}
static inline uint8_t get_rd(uint32_t ins) {
	return (ins >> 12) & 15;
}
static inline uint8_t get_rn(uint32_t ins) {
	return (ins >> 16) & 15;
}
static inline uint8_t get_rm(uint32_t ins) {
	return ins & 15;
}
static inline uint8_t get_S(uint32_t ins) {
	return (ins >> 20) & 1;
}
static inline uint8_t get_rs(uint32_t ins) {
	return (ins >> 8) & 15;
}
static inline uint8_t get_shift_imm(uint32_t ins) {
	return (ins >> 7) & 31;
}
static inline int get_shift_code(uint32_t ins) {
	return (ins >> 5) & 3;
}

// Decoding
dp_instruction_handler_t decode(int op_code) {
	switch(op_code) {
		case 0: return and; break;
		case 1: return eor; break;
		case 2: return sub; break;
		case 3: return rsb; break;
		case 4: return add; break;
		case 5: return adc; break;
		case 6: return sbc; break;
		case 7: return rsc; break;
		case 8: return tst; break;
		case 9: return teq; break;
		case 10: return cmp; break;
		case 11: return cmn; break;
		case 12: return orr; break;
		case 13: return mov; break;
		case 14: return bic; break;
		case 15: return mvn; break;
	}
}

// Instructions

void and(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {
	uint64_t result = op1 & op2;
	if(S) {
		if(rd != 15){ // � compl�ter
			uint32_t cpsr = arm_read_cpsr(p);
			cpsr = clear_n(cpsr);
			cpsr = clear_z(cpsr);
			cpsr = clear_c(cpsr);
			cpsr = clear_v(cpsr);
			if(get_bit(result,31)) cpsr = set_n(cpsr);
			if(result == 0) cpsr = set_z(cpsr);
			if(result > UINT_MAX) cpsr = set_c(cpsr);
			if(   add && (get_bit(op1,31) == get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31))
				|| !add && (get_bit(op1,31) != get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31)) )
				cpsr = set_v(cpsr);
	
			arm_write_cpsr(p, cpsr);
		}
		else {
			if(arm_current_mode_has_spsr(p)) {
				arm_write_cpsr(p,arm_read_spsr(p));
			}
			else {
				// UNPREDICTABLE
			}
		}
	}
	arm_write_register(p, rd, result);
}

void eor(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {
	uint64_t result = op1 ^ op2;
	if(S) {
	uint32_t cpsr = arm_read_cpsr(p);
	cpsr = clear_n(cpsr);
	cpsr = clear_z(cpsr);
	cpsr = clear_c(cpsr);
	cpsr = clear_v(cpsr);
	if(get_bit(result,31)) cpsr = set_n(cpsr);
	if(result == 0) cpsr = set_z(cpsr);
	if(result > UINT_MAX) cpsr = set_c(cpsr);
	if(   add && (get_bit(op1,31) == get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31))
		|| !add && (get_bit(op1,31) != get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31)) )
		cpsr = set_v(cpsr);
	
	arm_write_cpsr(p, cpsr);
	}
	arm_write_register(p, rd, result);
}

void sub(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {
	uint64_t result = op1 - op2;
	if(S) {
	uint32_t cpsr = arm_read_cpsr(p);
	cpsr = clear_n(cpsr);
	cpsr = clear_z(cpsr);
	cpsr = clear_c(cpsr);
	cpsr = clear_v(cpsr);
	if(get_bit(result,31)) cpsr = set_n(cpsr);
	if(result == 0) cpsr = set_z(cpsr);
	if(result > UINT_MAX) cpsr = set_c(cpsr);
	if(   add && (get_bit(op1,31) == get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31))
		|| !add && (get_bit(op1,31) != get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31)) )
		cpsr = set_v(cpsr);
	
	arm_write_cpsr(p, cpsr);
	}
	arm_write_register(p, rd, result);
}

void rsb(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {
	uint64_t result = op2 - op1;
	if(S) {
	uint32_t cpsr = arm_read_cpsr(p);
	cpsr = clear_n(cpsr);
	cpsr = clear_z(cpsr);
	cpsr = clear_c(cpsr);
	cpsr = clear_v(cpsr);
	if(get_bit(result,31)) cpsr = set_n(cpsr);
	if(result == 0) cpsr = set_z(cpsr);
	if(result > UINT_MAX) cpsr = set_c(cpsr);
	if(   add && (get_bit(op1,31) == get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31))
		|| !add && (get_bit(op1,31) != get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31)) )
		cpsr = set_v(cpsr);
	
	arm_write_cpsr(p, cpsr);
	}
	arm_write_register(p, rd, result);
}

void add(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {
	uint64_t result = op1 + op2;
	if(S) {
	uint32_t cpsr = arm_read_cpsr(p);
	cpsr = clear_n(cpsr);
	cpsr = clear_z(cpsr);
	cpsr = clear_c(cpsr);
	cpsr = clear_v(cpsr);
	if(get_bit(result,31)) cpsr = set_n(cpsr);
	if(result == 0) cpsr = set_z(cpsr);
	if(result > UINT_MAX) cpsr = set_c(cpsr);
	if(   add && (get_bit(op1,31) == get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31))
		|| !add && (get_bit(op1,31) != get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31)) )
		cpsr = set_v(cpsr);
	
	arm_write_cpsr(p, cpsr);
	}
	arm_write_register(p, rd, result);
}

void adc(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {
	uint64_t result = op1 + op2 + is_c_set(p);
	if(S) {
	uint32_t cpsr = arm_read_cpsr(p);
	cpsr = clear_n(cpsr);
	cpsr = clear_z(cpsr);
	cpsr = clear_c(cpsr);
	cpsr = clear_v(cpsr);
	if(get_bit(result,31)) cpsr = set_n(cpsr);
	if(result == 0) cpsr = set_z(cpsr);
	if(result > UINT_MAX) cpsr = set_c(cpsr);
	if(   add && (get_bit(op1,31) == get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31))
		|| !add && (get_bit(op1,31) != get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31)) )
		cpsr = set_v(cpsr);
	
	arm_write_cpsr(p, cpsr);
	}
	arm_write_register(p, rd, result);
}

void sbc(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {
	uint64_t result = op1 - op2 - is_c_clear(p);
	if(S) {
	uint32_t cpsr = arm_read_cpsr(p);
	cpsr = clear_n(cpsr);
	cpsr = clear_z(cpsr);
	cpsr = clear_c(cpsr);
	cpsr = clear_v(cpsr);
	if(get_bit(result,31)) cpsr = set_n(cpsr);
	if(result == 0) cpsr = set_z(cpsr);
	if(result > UINT_MAX) cpsr = set_c(cpsr);
	if(   add && (get_bit(op1,31) == get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31))
		|| !add && (get_bit(op1,31) != get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31)) )
		cpsr = set_v(cpsr);
	
	arm_write_cpsr(p, cpsr);
	}
	arm_write_register(p, rd, result);
}

void rsc(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {
	uint64_t result = op2 - op1 - is_c_clear(p);
	if(S) {
	uint32_t cpsr = arm_read_cpsr(p);
	cpsr = clear_n(cpsr);
	cpsr = clear_z(cpsr);
	cpsr = clear_c(cpsr);
	cpsr = clear_v(cpsr);
	if(get_bit(result,31)) cpsr = set_n(cpsr);
	if(result == 0) cpsr = set_z(cpsr);
	if(result > UINT_MAX) cpsr = set_c(cpsr);
	if(   add && (get_bit(op1,31) == get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31))
		|| !add && (get_bit(op1,31) != get_bit(op2,31) && get_bit(op1,31) != get_bit(result,31)) )
		cpsr = set_v(cpsr);
	
	arm_write_cpsr(p, cpsr);
	}
	arm_write_register(p, rd, result);
}

void tst(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {

}

void teq(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {

}

void cmp(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {

}

void cmn(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {

}

void orr(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {

}

void mov(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {

}

void bic(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {

}

void mvn(arm_core p,uint8_t rd,int op1,int op2,uint8_t S, uint8_t shift_C) {

}


// Decoding functions for various classes of instructions
int arm_data_processing_shift(arm_core p, uint32_t ins) {
    debug("arm_data_processing_shift: %d\n", (int)ins);    
    
		uint8_t rd, S, shift_C;
		int op1, op2;
    uint8_t cond_field = instruction_get_condition_field(ins);
    int result = instruction_check_condition(p, cond_field);
    if(result) return result;
    
    // Parsing the instruction
    int op_code = get_op_code(ins);
    dp_instruction_handler_t handler = decode(op_code);
    
    rd = get_rd(ins);
    
    if(op_code != MOV && op_code != MVN) {
    	op1 = arm_read_register(p, get_rn(ins));
    }
    
    op2 = get_shifted(ins, &shift_C);
    
    S = get_S(ins);
    
		// Redirecting on MRS and MSR
		if ((op_code == TST || op_code == TEQ || op_code == CMP || op_code == CMN) && !S)
		{
			if (get_bit(ins, 21))
				arm_msr(p, ins);
			else
				arm_mrs(p, ins);
		}
    else {
    // DP instruction call
    handler(p, rd, op1, op2, S, shift_C);
    }
    return result;
}


int arm_data_processing_immediate(arm_core p, uint32_t ins) {
    debug("arm_data_processing_immediate: %d\n", (int)ins);

	uint8_t rd, rn, rm, S, rs, shift_imm, shift_code, shift_C;
	int op1, op2;

    int op_code = get_op_code(ins);
	//recuperation de l'opperande
	if (get_bit(ins, 25)) //immediate
		op2 = ror(get_bits(ins, 7, 0), get_bits(ins, 8, 11) * 2); 
	else //register
	{
		rm = get_bits(3, 0);
		op2 = arm_read_register(p, rm);
	}

	
		dp_instruction_handler_t handler = decode(op_code);

		rd = get_rd(ins);

		if(op_code != MOV && op_code != MVN) {
			rn = get_rn(ins);
			op1 = arm_read_register(p, rn);
		}
		
		if(op_code == CMP || op_code == CMN  || op_code == TST || op_code == TEQ) {
			S = 1;
		}
		else {
			S = get_S(ins);
		}

		handler(p, rd, op1, op2, S, shift_C);
	


    /*
    	Attention : bit25 == 0 &&	bit7 == 1 && bit4 == 1 	=> load/store 
    	cf. p.443, 144 et 146
    	=> appel arm_load_store_miscellaneous(arm_core p, uint32_t ins)
    */


    
    return 0;
}

