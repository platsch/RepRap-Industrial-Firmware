#include "Repetier.h"

//extern "C" void __cxa_pure_virtual() { }

HAL::HAL()
{
    //ctor
}

HAL::~HAL()
{
    //dtor
}


/** \brief Optimized division

Normally the C compiler will compute a long/long division, which takes ~670 Ticks.
This version is optimized for a 16 bit dividend and recognises the special cases
of a 24 bit and 16 bit dividend, which offen, but not always occur in updating the
interval.
*/
inline long Div4U2U(unsigned long a,unsigned int b)
{
#if CPU_ARCH==ARCH_AVR
    // r14/r15 remainder
    // r16 counter
    __asm__ __volatile__ (
        "clr r14 \n\t"
        "sub r15,r15 \n\t"
        "tst %D0 \n\t"
        "brne do32%= \n\t"
        "tst %C0 \n\t"
        "breq donot24%= \n\t"
        "rjmp do24%= \n\t"
        "donot24%=:" "ldi r16,17 \n\t" // 16 Bit divide
        "d16u_1%=:" "rol %A0 \n\t"
        "rol %B0 \n\t"
        "dec r16 \n\t"
        "brne	d16u_2%= \n\t"
        "rjmp end%= \n\t"
        "d16u_2%=:" "rol r14 \n\t"
        "rol r15 \n\t"
        "sub r14,%A2 \n\t"
        "sbc r15,%B2 \n\t"
        "brcc	d16u_3%= \n\t"
        "add r14,%A2 \n\t"
        "adc r15,%B2 \n\t"
        "clc \n\t"
        "rjmp d16u_1%= \n\t"
        "d16u_3%=:" "sec \n\t"
        "rjmp d16u_1%= \n\t"
        "do32%=:" // divide full 32 bit
        "rjmp do32B%= \n\t"
        "do24%=:" // divide 24 bit

        "ldi r16,25 \n\t" // 24 Bit divide
        "d24u_1%=:" "rol %A0 \n\t"
        "rol %B0 \n\t"
        "rol %C0 \n\t"
        "dec r16 \n\t"
        "brne	d24u_2%= \n\t"
        "rjmp end%= \n\t"
        "d24u_2%=:" "rol r14 \n\t"
        "rol r15 \n\t"
        "sub r14,%A2 \n\t"
        "sbc r15,%B2 \n\t"
        "brcc	d24u_3%= \n\t"
        "add r14,%A2 \n\t"
        "adc r15,%B2 \n\t"
        "clc \n\t"
        "rjmp d24u_1%= \n\t"
        "d24u_3%=:" "sec \n\t"
        "rjmp d24u_1%= \n\t"

        "do32B%=:" // divide full 32 bit

        "ldi r16,33 \n\t" // 32 Bit divide
        "d32u_1%=:" "rol %A0 \n\t"
        "rol %B0 \n\t"
        "rol %C0 \n\t"
        "rol %D0 \n\t"
        "dec r16 \n\t"
        "brne	d32u_2%= \n\t"
        "rjmp end%= \n\t"
        "d32u_2%=:" "rol r14 \n\t"
        "rol r15 \n\t"
        "sub r14,%A2 \n\t"
        "sbc r15,%B2 \n\t"
        "brcc	d32u_3%= \n\t"
        "add r14,%A2 \n\t"
        "adc r15,%B2 \n\t"
        "clc \n\t"
        "rjmp d32u_1%= \n\t"
        "d32u_3%=:" "sec \n\t"
        "rjmp d32u_1%= \n\t"

        "end%=:" // end
        :"=&r"(a)
        :"0"(a),"r"(b)
        :"r14","r15","r16"
    );
    return a;
#else
    return a/b;
#endif
}

const uint16_t fast_div_lut[17] PROGMEM = {0,F_CPU/4096,F_CPU/8192,F_CPU/12288,F_CPU/16384,F_CPU/20480,F_CPU/24576,F_CPU/28672,F_CPU/32768,F_CPU/36864
        ,F_CPU/40960,F_CPU/45056,F_CPU/49152,F_CPU/53248,F_CPU/57344,F_CPU/61440,F_CPU/65536
                                          };

const uint16_t slow_div_lut[257] PROGMEM = {0,F_CPU/32,F_CPU/64,F_CPU/96,F_CPU/128,F_CPU/160,F_CPU/192,F_CPU/224,F_CPU/256,F_CPU/288,F_CPU/320,F_CPU/352
        ,F_CPU/384,F_CPU/416,F_CPU/448,F_CPU/480,F_CPU/512,F_CPU/544,F_CPU/576,F_CPU/608,F_CPU/640,F_CPU/672,F_CPU/704,F_CPU/736,F_CPU/768,F_CPU/800,F_CPU/832
        ,F_CPU/864,F_CPU/896,F_CPU/928,F_CPU/960,F_CPU/992,F_CPU/1024,F_CPU/1056,F_CPU/1088,F_CPU/1120,F_CPU/1152,F_CPU/1184,F_CPU/1216,F_CPU/1248,F_CPU/1280,F_CPU/1312
        ,F_CPU/1344,F_CPU/1376,F_CPU/1408,F_CPU/1440,F_CPU/1472,F_CPU/1504,F_CPU/1536,F_CPU/1568,F_CPU/1600,F_CPU/1632,F_CPU/1664,F_CPU/1696,F_CPU/1728,F_CPU/1760,F_CPU/1792
        ,F_CPU/1824,F_CPU/1856,F_CPU/1888,F_CPU/1920,F_CPU/1952,F_CPU/1984,F_CPU/2016
        ,F_CPU/2048,F_CPU/2080,F_CPU/2112,F_CPU/2144,F_CPU/2176,F_CPU/2208,F_CPU/2240,F_CPU/2272,F_CPU/2304,F_CPU/2336,F_CPU/2368,F_CPU/2400
        ,F_CPU/2432,F_CPU/2464,F_CPU/2496,F_CPU/2528,F_CPU/2560,F_CPU/2592,F_CPU/2624,F_CPU/2656,F_CPU/2688,F_CPU/2720,F_CPU/2752,F_CPU/2784,F_CPU/2816,F_CPU/2848,F_CPU/2880
        ,F_CPU/2912,F_CPU/2944,F_CPU/2976,F_CPU/3008,F_CPU/3040,F_CPU/3072,F_CPU/3104,F_CPU/3136,F_CPU/3168,F_CPU/3200,F_CPU/3232,F_CPU/3264,F_CPU/3296,F_CPU/3328,F_CPU/3360
        ,F_CPU/3392,F_CPU/3424,F_CPU/3456,F_CPU/3488,F_CPU/3520,F_CPU/3552,F_CPU/3584,F_CPU/3616,F_CPU/3648,F_CPU/3680,F_CPU/3712,F_CPU/3744,F_CPU/3776,F_CPU/3808,F_CPU/3840
        ,F_CPU/3872,F_CPU/3904,F_CPU/3936,F_CPU/3968,F_CPU/4000,F_CPU/4032,F_CPU/4064
        ,F_CPU/4096,F_CPU/4128,F_CPU/4160,F_CPU/4192,F_CPU/4224,F_CPU/4256,F_CPU/4288,F_CPU/4320,F_CPU/4352,F_CPU/4384,F_CPU/4416,F_CPU/4448,F_CPU/4480,F_CPU/4512,F_CPU/4544
        ,F_CPU/4576,F_CPU/4608,F_CPU/4640,F_CPU/4672,F_CPU/4704,F_CPU/4736,F_CPU/4768,F_CPU/4800,F_CPU/4832,F_CPU/4864,F_CPU/4896,F_CPU/4928,F_CPU/4960,F_CPU/4992,F_CPU/5024
        ,F_CPU/5056,F_CPU/5088,F_CPU/5120,F_CPU/5152,F_CPU/5184,F_CPU/5216,F_CPU/5248,F_CPU/5280,F_CPU/5312,F_CPU/5344,F_CPU/5376,F_CPU/5408,F_CPU/5440,F_CPU/5472,F_CPU/5504
        ,F_CPU/5536,F_CPU/5568,F_CPU/5600,F_CPU/5632,F_CPU/5664,F_CPU/5696,F_CPU/5728,F_CPU/5760,F_CPU/5792,F_CPU/5824,F_CPU/5856,F_CPU/5888,F_CPU/5920,F_CPU/5952,F_CPU/5984
        ,F_CPU/6016,F_CPU/6048,F_CPU/6080,F_CPU/6112,F_CPU/6144,F_CPU/6176,F_CPU/6208,F_CPU/6240,F_CPU/6272,F_CPU/6304,F_CPU/6336,F_CPU/6368,F_CPU/6400,F_CPU/6432,F_CPU/6464
        ,F_CPU/6496,F_CPU/6528,F_CPU/6560,F_CPU/6592,F_CPU/6624,F_CPU/6656,F_CPU/6688,F_CPU/6720,F_CPU/6752,F_CPU/6784,F_CPU/6816,F_CPU/6848,F_CPU/6880,F_CPU/6912,F_CPU/6944
        ,F_CPU/6976,F_CPU/7008,F_CPU/7040,F_CPU/7072,F_CPU/7104,F_CPU/7136,F_CPU/7168,F_CPU/7200,F_CPU/7232,F_CPU/7264,F_CPU/7296,F_CPU/7328,F_CPU/7360,F_CPU/7392,F_CPU/7424
        ,F_CPU/7456,F_CPU/7488,F_CPU/7520,F_CPU/7552,F_CPU/7584,F_CPU/7616,F_CPU/7648,F_CPU/7680,F_CPU/7712,F_CPU/7744,F_CPU/7776,F_CPU/7808,F_CPU/7840,F_CPU/7872,F_CPU/7904
        ,F_CPU/7936,F_CPU/7968,F_CPU/8000,F_CPU/8032,F_CPU/8064,F_CPU/8096,F_CPU/8128,F_CPU/8160,F_CPU/8192
                                           };
/** \brief approximates division of F_CPU/divisor

In the stepper interrupt a division is needed, which is a slow operation.
The result is used for timer calculation where small errors are ok. This
function uses lookup tables to find a fast approximation of the result.

*/
long HAL::CPUDivU2(unsigned int divisor)
{
#if CPU_ARCH==ARCH_AVR
    long res;
    unsigned short table;
    if(divisor<8192)
    {
        if(divisor<512)
        {
            if(divisor<10) divisor = 10;
            return Div4U2U(F_CPU,divisor); // These entries have overflows in lookuptable!
        }
        table = (unsigned short)&slow_div_lut[0];
        __asm__ __volatile__( // needs 64 ticks neu 49 Ticks
            "mov r18,%A1 \n\t"
            "andi r18,31 \n\t"  // divisor & 31 in r18
            "lsr %B1 \n\t" // divisor >> 4
            "ror %A1 \n\t"
            "lsr %B1 \n\t"
            "ror %A1 \n\t"
            "lsr %B1 \n\t"
            "ror %A1 \n\t"
            "lsr %B1 \n\t"
            "ror %A1 \n\t"
            "andi %A1,254 \n\t"
            "add %A2,%A1 \n\t" // table+divisor>>3
            "adc %B2,%B1 \n\t"
            "lpm %A0,Z+ \n\t" // y0 in res
            "lpm %B0,Z+ \n\t"  // %C0,%D0 are 0
            "movw r4,%A0 \n\t" // y0 nach gain (r4-r5)
            "lpm r0,Z+ \n\t" // gain = gain-y1
            "sub r4,r0 \n\t"
            "lpm r0,Z+ \n\t"
            "sbc r5,r0 \n\t"
            "mul r18,r4 \n\t" // gain*(divisor & 31)
            "movw %A1,r0 \n\t" // divisor not needed any more, use for byte 0,1 of result
            "mul r18,r5 \n\t"
            "add %B1,r0 \n\t"
            "mov %A2,r1 \n\t"
            "lsl %A1 \n\t"
            "rol %B1 \n\t"
            "rol %A2 \n\t"
            "lsl %A1 \n\t"
            "rol %B1 \n\t"
            "rol %A2 \n\t"
            "lsl %A1 \n\t"
            "rol %B1 \n\t"
            "rol %A2 \n\t"
            "sub %A0,%B1 \n\t"
            "sbc %B0,%A2 \n\t"
            "clr %C0 \n\t"
            "clr %D0 \n\t"
            "clr r1 \n\t"
            : "=&r" (res),"=&d"(divisor),"=&z"(table) : "1"(divisor),"2"(table) : "r18","r4","r5");
        return res;
        /*unsigned short adr0 = (unsigned short)&slow_div_lut+(divisor>>4)&1022;
        long y0=	pgm_read_dword_near(adr0);
        long gain = y0-pgm_read_dword_near(adr0+2);
        return y0-((gain*(divisor & 31))>>5);*/
    }
    else
    {
        table = (unsigned short)&fast_div_lut[0];
        __asm__ __volatile__( // needs 49 ticks
            "movw r18,%A1 \n\t"
            "andi r19,15 \n\t"  // divisor & 4095 in r18,r19
            "lsr %B1 \n\t" // divisor >> 3, then %B1 is 2*(divisor >> 12)
            "lsr %B1 \n\t"
            "lsr %B1 \n\t"
            "andi %B1,254 \n\t"
            "add %A2,%B1 \n\t" // table+divisor>>11
            "adc %B2,r1 \n\t" //
            "lpm %A0,Z+ \n\t" // y0 in res
            "lpm %B0,Z+ \n\t"
            "movw r4,%A0 \n\t" // y0 to gain (r4-r5)
            "lpm r0,Z+ \n\t" // gain = gain-y1
            "sub r4,r0 \n\t"
            "lpm r0,Z+ \n\t"
            "sbc r5,r0 \n\t" // finished - result has max. 16 bit
            "mul r18,r4 \n\t" // gain*(divisor & 4095)
            "movw %A1,r0 \n\t" // divisor not needed any more, use for byte 0,1 of result
            "mul r19,r5 \n\t"
            "mov %A2,r0 \n\t" // %A2 = byte 3 of result
            "mul r18,r5 \n\t"
            "add %B1,r0 \n\t"
            "adc %A2,r1 \n\t"
            "mul r19,r4 \n\t"
            "add %B1,r0 \n\t"
            "adc %A2,r1 \n\t"
            "andi %B1,240 \n\t" // >> 12
            "swap %B1 \n\t"
            "swap %A2 \r\n"
            "mov %A1,%A2 \r\n"
            "andi %A1,240 \r\n"
            "or %B1,%A1 \r\n"
            "andi %A2,15 \r\n"
            "sub %A0,%B1 \n\t"
            "sbc %B0,%A2 \n\t"
            "clr %C0 \n\t"
            "clr %D0 \n\t"
            "clr r1 \n\t"
            : "=&r" (res),"=&d"(divisor),"=&z"(table) : "1"(divisor),"2"(table) : "r18","r19","r4","r5");
        return res;
        /*
        // The asm mimics the following code
        unsigned short adr0 = (unsigned short)&fast_div_lut+(divisor>>11)&254;
        unsigned short y0=	pgm_read_word_near(adr0);
        unsigned short gain = y0-pgm_read_word_near(adr0+2);
        return y0-(((long)gain*(divisor & 4095))>>12);*/
#else
    return F_CPU/divisor;
#endif
    }
}

// ================== Interrupt handling ======================

/** \brief Sets the timer 1 compare value to delay ticks.

This function sets the OCR1A compare counter  to get the next interrupt
at delay ticks measured from the last interrupt. delay must be << 2^24
*/
inline void setTimer(unsigned long delay)
{
    __asm__ __volatile__ (
        "cli \n\t"
        "tst %C[delay] \n\t" //if(delay<65536) {
        "brne else%= \n\t"
        "cpi %B[delay],255 \n\t"
        "breq else%= \n\t" // delay <65280
        "sts stepperWait,r1 \n\t" // stepperWait = 0;
        "sts stepperWait+1,r1 \n\t"
        "sts stepperWait+2,r1 \n\t"
        "lds %C[delay],%[time] \n\t" // Read TCNT1
        "lds %D[delay],%[time]+1 \n\t"
        "ldi r18,100 \n\t" // Add 100 to TCNT1
        "add %C[delay],r18 \n\t"
        "adc %D[delay],r1 \n\t"
        "cp %A[delay],%C[delay] \n\t" // delay<TCNT1+1
        "cpc %B[delay],%D[delay] \n\t"
        "brcc exact%= \n\t"
        "sts %[ocr]+1,%D[delay] \n\t" //  OCR1A = TCNT1+100;
        "sts %[ocr],%C[delay] \n\t"
        "rjmp end%= \n\t"
        "exact%=: sts %[ocr]+1,%B[delay] \n\t" //  OCR1A = delay;
        "sts %[ocr],%A[delay] \n\t"
        "rjmp end%= \n\t"
        "else%=: subi	%B[delay], 0x80 \n\t" //} else { stepperWait = delay-32768;
        "sbci	%C[delay], 0x00 \n\t"
        "sts stepperWait,%A[delay] \n\t"
        "sts stepperWait+1,%B[delay] \n\t"
        "sts stepperWait+2,%C[delay] \n\t"
        "ldi	%D[delay], 0x80 \n\t" //OCR1A = 32768;
        "sts	%[ocr]+1, %D[delay] \n\t"
        "sts	%[ocr], r1 \n\t"
        "end%=: \n\t"
        :[delay]"=&d"(delay) // Output
        :"0"(delay),[ocr]"i" (_SFR_MEM_ADDR(OCR1A)),[time]"i"(_SFR_MEM_ADDR(TCNT1)) // Input
        :"r18" // Clobber
    );
    /* // Assembler above replaced this code
      if(delay<65280) {
        stepperWait = 0;
        unsigned int count = TCNT1+100;
        if(delay<count)
          OCR1A = count;
        else
          OCR1A = delay;
      } else {
        stepperWait = delay-32768;
        OCR1A = 32768;
      }*/
}

volatile byte insideTimer1=0;
long stepperWait = 0;
extern long bresenham_step();
/** \brief Timer interrupt routine to drive the stepper motors.
*/
ISR(TIMER1_COMPA_vect)
{
    if(insideTimer1) return;
    byte doExit;
    __asm__ __volatile__ (
        "ldi %[ex],0 \n\t"
        "lds r23,stepperWait+2 \n\t"
        "tst r23 \n\t" //if(stepperWait<65536) {
        "brne else%= \n\t" // Still > 65535
        "lds r23,stepperWait+1 \n\t"
        "tst r23 \n\t"
        "brne last%= \n\t" // Still not 0, go ahead
        "lds r22,stepperWait \n\t"
        "breq end%= \n\t" // stepperWait is 0, do your work
        "last%=: \n\t"
        "sts %[ocr]+1,r23 \n\t" //  OCR1A = stepper wait;
        "sts %[ocr],r22 \n\t"
        "sts stepperWait,r1 \n\t"
        "sts stepperWait+1,r1 \n\t"
        "rjmp end1%= \n\t"
        "else%=: lds r22,stepperWait+1 \n\t" //} else { stepperWait = stepperWait-32768;
        "subi	r22, 0x80 \n\t"
        "sbci	r23, 0x00 \n\t"
        "sts stepperWait+1,r22 \n\t"	// ocr1a stays 32768
        "sts stepperWait+2,r23 \n\t"
        "end1%=: ldi %[ex],1 \n\t"
        "end%=: \n\t"
        :[ex]"=&d"(doExit):[ocr]"i" (_SFR_MEM_ADDR(OCR1A)):"r22","r23" );
    if(doExit) return;
    insideTimer1=1;
    OCR1A=61000;
    if(PrintLine::hasLines())
    {
        setTimer(PrintLine::bresenhamStep());
    }
    else
    {
        if(waitRelax==0)
        {
#ifdef USE_ADVANCE
            if(printer.advance_steps_set)
            {
                printer.extruderStepsNeeded-=printer.advance_steps_set;
#ifdef ENABLE_QUADRATIC_ADVANCE
                printer.advance_executed = 0;
#endif
                printer.advance_steps_set = 0;
            }
#endif
#if defined(USE_ADVANCE)
            if(!printer.extruderStepsNeeded) if(DISABLE_E) Extruder::disableCurrentExtruderMotor();
#else
            if(DISABLE_E) extruder_disable();
#endif
        }
        else waitRelax--;
        stepperWait = 0; // Importent becaus of optimization in asm at begin
        OCR1A = 65500; // Wait for next move
    }
    DEBUG_MEMORY;
    insideTimer1=0;
}

/**
This timer is called 3906 timer per second. It is used to update pwm values for heater and some other frequent jobs.
*/
ISR(PWM_TIMER_VECTOR)
{
    static byte pwm_count = 0;
    static byte pwm_pos_set[NUM_EXTRUDER+3];
    static byte pwm_cooler_pos_set[NUM_EXTRUDER];
    PWM_OCR += 64;
    if(pwm_count==0)
    {
#if EXT0_HEATER_PIN>-1
        if((pwm_pos_set[0] = pwm_pos[0])>0) WRITE(EXT0_HEATER_PIN,1);
#if EXT0_EXTRUDER_COOLER_PIN>-1
        if((pwm_cooler_pos_set[0] = extruder[0].coolerPWM)>0) WRITE(EXT0_EXTRUDER_COOLER_PIN,1);
#endif
#endif
#if defined(EXT1_HEATER_PIN) && EXT1_HEATER_PIN>-1
        if((pwm_pos_set[1] = pwm_pos[1])>0) WRITE(EXT1_HEATER_PIN,1);
#if EXT1_EXTRUDER_COOLER_PIN>-1
        if((pwm_cooler_pos_set[1] = extruder[1].coolerPWM)>0) WRITE(EXT1_EXTRUDER_COOLER_PIN,1);
#endif
#endif
#if defined(EXT2_HEATER_PIN) && EXT2_HEATER_PIN>-1
        if((pwm_pos_set[2] = pwm_pos[2])>0) WRITE(EXT2_HEATER_PIN,1);
#if EXT2_EXTRUDER_COOLER_PIN>-1
        if((pwm_cooler_pos_set[2] = extruder[2].coolerPWM)>0) WRITE(EXT2_EXTRUDER_COOLER_PIN,1);
#endif
#endif
#if defined(EXT3_HEATER_PIN) && EXT3_HEATER_PIN>-1
        if((pwm_pos_set[3] = pwm_pos[3])>0) WRITE(EXT3_HEATER_PIN,1);
#if EXT3_EXTRUDER_COOLER_PIN>-1
        if((pwm_cooler_pos_set[3] = extruder[3].coolerPWM)>0) WRITE(EXT3_EXTRUDER_COOLER_PIN,1);
#endif
#endif
#if defined(EXT4_HEATER_PIN) && EXT4_HEATER_PIN>-1
        if((pwm_pos_set[4] = pwm_pos[4])>0) WRITE(EXT4_HEATER_PIN,1);
#if EXT4_EXTRUDER_COOLER_PIN>-1
        if((pwm_cooler_pos_set[4] = pwm_pos[4].coolerPWM)>0) WRITE(EXT4_EXTRUDER_COOLER_PIN,1);
#endif
#endif
#if defined(EXT5_HEATER_PIN) && EXT5_HEATER_PIN>-1
        if((pwm_pos_set[5] = pwm_pos[5])>0) WRITE(EXT5_HEATER_PIN,1);
#if EXT5_EXTRUDER_COOLER_PIN>-1
        if((pwm_cooler_pos_set[5] = extruder[5].coolerPWM)>0) WRITE(EXT5_EXTRUDER_COOLER_PIN,1);
#endif
#endif
#if FAN_BOARD_PIN>-1
        if((pwm_pos_set[NUM_EXTRUDER+1] = pwm_pos[NUM_EXTRUDER+1])>0) WRITE(FAN_BOARD_PIN,1);
#endif
#if FAN_PIN>-1 && FEATURE_FAN_CONTROL
        if((pwm_pos_set[NUM_EXTRUDER+2] = pwm_pos[NUM_EXTRUDER+2])>0) WRITE(FAN_PIN,1);
#endif
#if HEATED_BED_HEATER_PIN>-1 && HAVE_HEATED_BED
        if((pwm_pos_set[NUM_EXTRUDER] = pwm_pos[NUM_EXTRUDER])>0) WRITE(HEATED_BED_HEATER_PIN,1);
#endif
    }
#if EXT0_HEATER_PIN>-1
    if(pwm_pos_set[0] == pwm_count && pwm_pos_set[0]!=255) WRITE(EXT0_HEATER_PIN,0);
#if EXT0_EXTRUDER_COOLER_PIN>-1
    if(pwm_cooler_pos_set[0] == pwm_count && pwm_cooler_pos_set[0]!=255) WRITE(EXT0_EXTRUDER_COOLER_PIN,0);
#endif
#endif
#if defined(EXT1_HEATER_PIN) && EXT1_HEATER_PIN>-1
    if(pwm_pos_set[1] == pwm_count && pwm_pos_set[1]!=255) WRITE(EXT1_HEATER_PIN,0);
#if EXT1_EXTRUDER_COOLER_PIN>-1
    if(pwm_cooler_pos_set[1] == pwm_count && pwm_cooler_pos_set[1]!=255) WRITE(EXT1_EXTRUDER_COOLER_PIN,0);
#endif
#endif
#if defined(EXT2_HEATER_PIN) && EXT2_HEATER_PIN>-1
    if(pwm_pos_set[2] == pwm_count && pwm_pos_set[2]!=255) WRITE(EXT2_HEATER_PIN,0);
#if EXT2_EXTRUDER_COOLER_PIN>-1
    if(pwm_cooler_pos_set[2] == pwm_count && pwm_cooler_pos_set[2]!=255) WRITE(EXT2_EXTRUDER_COOLER_PIN,0);
#endif
#endif
#if defined(EXT3_HEATER_PIN) && EXT3_HEATER_PIN>-1
    if(pwm_pos_set[3] == pwm_count && pwm_pos_set[3]!=255) WRITE(EXT3_HEATER_PIN,0);
#if EXT3_EXTRUDER_COOLER_PIN>-1
    if(pwm_cooler_pos_set[3] == pwm_count && pwm_cooler_pos_set[3]!=255) WRITE(EXT3_EXTRUDER_COOLER_PIN,0);
#endif
#endif
#if defined(EXT4_HEATER_PIN) && EXT4_HEATER_PIN>-1
    if(pwm_pos_set[4] == pwm_count && pwm_pos_set[4]!=255) WRITE(EXT4_HEATER_PIN,0);
#if EXT4_EXTRUDER_COOLER_PIN>-1
    if(pwm_cooler_pos_set[4] == pwm_count && pwm_cooler_pos_set[4]!=255) WRITE(EXT4_EXTRUDER_COOLER_PIN,0);
#endif
#endif
#if defined(EXT5_HEATER_PIN) && EXT5_HEATER_PIN>-1
    if(pwm_pos_set[5] == pwm_count && pwm_pos_set[5]!=255) WRITE(EXT5_HEATER_PIN,0);
#if EXT5_EXTRUDER_COOLER_PIN>-1
    if(pwm_cooler_pos_set[5] == pwm_count && pwm_cooler_pos_set[5]!=255) WRITE(EXT5_EXTRUDER_COOLER_PIN,0);
#endif
#endif
#if FAN_BOARD_PIN>-1
    if(pwm_pos_set[NUM_EXTRUDER+2] == pwm_count && pwm_pos_set[NUM_EXTRUDER+2]!=255) WRITE(FAN_BOARD_PIN,0);
#endif
#if FAN_PIN>-1 && FEATURE_FAN_CONTROL
    if(pwm_pos_set[NUM_EXTRUDER+2] == pwm_count && pwm_pos_set[NUM_EXTRUDER+2]!=255) WRITE(FAN_PIN,0);
#endif
#if HEATED_BED_HEATER_PIN>-1 && HAVE_HEATED_BED
    if(pwm_pos_set[NUM_EXTRUDER] == pwm_count && pwm_pos_set[NUM_EXTRUDER]!=255) WRITE(HEATED_BED_HEATER_PIN,0);
#endif
    HAL::allowInterrupts();
    counter_periodical++; // Appxoimate a 100ms timer
    if(counter_periodical>=(int)(F_CPU/40960))
    {
        counter_periodical=0;
        execute_periodical=1;
    }
// read analog values
#if ANALOG_INPUTS>0
    if((ADCSRA & _BV(ADSC))==0)   // Conversion finished?
    {
        osAnalogInputBuildup[osAnalogInputPos] += ADCW;
        if(++osAnalogInputCounter[osAnalogInputPos]>=_BV(ANALOG_INPUT_SAMPLE))
        {
#if ANALOG_INPUT_BITS+ANALOG_INPUT_SAMPLE<12
            osAnalogInputValues[osAnalogInputPos] =
                osAnalogInputBuildup[osAnalogInputPos] <<
                (12-ANALOG_INPUT_BITS-ANALOG_INPUT_SAMPLE);
#endif
#if ANALOG_INPUT_BITS+ANALOG_INPUT_SAMPLE>12
            osAnalogInputValues[osAnalogInputPos] =
                osAnalogInputBuildup[osAnalogInputPos] >>
                (ANALOG_INPUT_BITS+ANALOG_INPUT_SAMPLE-12);
#endif
#if ANALOG_INPUT_BITS+ANALOG_INPUT_SAMPLE==12
            osAnalogInputValues[osAnalogInputPos] =
                osAnalogInputBuildup[osAnalogInputPos];
#endif
            osAnalogInputBuildup[osAnalogInputPos] = 0;
            osAnalogInputCounter[osAnalogInputPos] = 0;
            // Start next conversion
            if(++osAnalogInputPos>=ANALOG_INPUTS) osAnalogInputPos = 0;
            byte channel = pgm_read_byte(&osAnalogInputChannels[osAnalogInputPos]);
#if defined(ADCSRB) && defined(MUX5)
            if(channel & 8)  // Reading channel 0-7 or 8-15?
                ADCSRB |= _BV(MUX5);
            else
                ADCSRB &= ~_BV(MUX5);
#endif
            ADMUX = (ADMUX & ~(0x1F)) | (channel & 7);
        }
        ADCSRA |= _BV(ADSC);  // start next conversion
    }
#endif

    UI_FAST; // Short timed user interface action
    pwm_count++;
}
#if defined(USE_ADVANCE)
byte extruder_wait_dirchange=0; ///< Wait cycles, if direction changes. Prevents stepper from loosing steps.
char extruder_last_dir = 0;
byte extruder_speed = 0;
#endif

/** \brief Timer routine for extruder stepper.

Several methods need to move the extruder. To get a optima result,
all methods update the printer_state.extruderStepsNeeded with the
number of additional steps needed. During this interrupt, one step
is executed. This will keep the extruder moving, until the total
wanted movement is achieved. This will be done with the maximum
allowable speed for the extruder.
*/
ISR(EXTRUDER_TIMER_VECTOR)
{
#if defined(USE_ADVANCE)
    if(!printer.isAdvanceActivated()) return; // currently no need
    byte timer = EXTRUDER_OCR;
    bool increasing = printer.extruderStepsNeeded>0;

    // Require at least 2 steps in one direction before going to action
    if(abs(printer.extruderStepsNeeded)<2)
    {
        EXTRUDER_OCR = timer+printer.maxExtruderSpeed;
        ANALYZER_OFF(ANALYZER_CH2);
        extruder_last_dir = 0;
        return;
    }

    /*  if(printer_state.extruderStepsNeeded==0) {
          extruder_last_dir = 0;
      }  else if((increasing>0 && extruder_last_dir<0) || (!increasing && extruder_last_dir>0)) {
        EXTRUDER_OCR = timer+50; // Little delay to accomodate to reversed direction
        extruder_set_direction(increasing ? 1 : 0);
        extruder_last_dir = (increasing ? 1 : -1);
        return;
      } else*/
    {
        if(extruder_last_dir==0)
        {
            Extruder::setDirection(increasing ? 1 : 0);
            extruder_last_dir = (increasing ? 1 : -1);
        }
        Extruder::step();
        printer.extruderStepsNeeded-=extruder_last_dir;
#if STEPPER_HIGH_DELAY>0
        HAL::delayMicroseconds(STEPPER_HIGH_DELAY);
#endif
        Extruder::unstep();
    }
    EXTRUDER_OCR = timer+printer.maxExtruderSpeed;
#endif
}
