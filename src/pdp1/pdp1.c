/*
 * Note: Original Java source written by:
 *
 * Barry Silverman mailto:barry@disus.com or mailto:bss@media.mit.edu
 * Vadim Gerasimov mailto:vadim@media.mit.edu
 * (not much was changed, only the IOT stuff and the 64 bit integer shifts)
 *
 * Note: I removed the IOT function to be external, it is
 * set at emulation initiation, the IOT function is part of
 * the machine emulation...
 *
 *
 * for the runnable java applet go to:
 * http://lcs.www.media.mit.edu/groups/el/projects/spacewar/
 *
 * for a complete html version of the pdp1 handbook go to:
 * http://www.dbit.com/~greeng3/pdp1/index.html
 *
 * there is another java simulator (by the same people) which runs the
 * original pdp1 LISP interpreter, go to:
 * http://lcs.www.media.mit.edu/groups/el/projects/pdp1
 *
 * and finally, there is a nice article about SPACEWAR!, go to:
 * http://ars-www.uchicago.edu/~eric/lore/spacewar/spacewar.html
 *
 * following is an extract from the handbook:
 *
 * INTRODUCTION
 *
 * The Programmed Data Processor (PDP-1) is a high speed, solid state digital computer designed to
 * operate with many types of input-output devices with no internal machine changes. It is a single
 * address, single instruction, stored program computer with powerful program features. Five-megacycle
 * circuits, a magnetic core memory and fully parallel processing make possible a computation rate of
 * 100,000 additions per second. The PDP-1 is unusually versatile. It is easy to install, operate and
 * maintain. Conventional 110-volt power is used, neither air conditioning nor floor reinforcement is
 * necessary, and preventive maintenance is provided for by built-in marginal checking circuits.
 *
 * PDP-1 circuits are based on the designs of DEC's highly successful and reliable System Modules.
 * Flip-flops and most switches use saturating transistors. Primary active elements are
 * Micro-Alloy-Diffused transistors.
 *
 * The entire computer occupies only 17 square feet of floor space. It consists of four equipment frames,
 * one of which is used as the operating station.
 *
 * CENTRAL PROCESSOR
 *
 * The Central Processor contains the control, arithmetic and memory addressing elements, and the memory
 * buffer register. The word length is 18 binary digits. Instructions are performed in multiples of the
 * memory cycle time of five microseconds. Add, subtract, deposit, and load, for example, are two-cycle
 * instructions requiring 10 microseconds. Multiplication requires and average of 20 microseconds.
 * Program features include: single address instructions, multiple step indirect addressing and logical
 * arithmetic commands. Console features include: flip-flop indicators grouped for convenient octal
 * reading, six program flags for automatic setting and computer sensing, and six sense switches for
 * manual setting and computer sensing.
 *
 * MEMORY SYSTEM
 *
 * The coincident-current, magnetic core memory of a standard PDP-1 holds 4096 words of 18 bits each.
 * Memory capacity may be readily expanded, in increments of 4096 words, to a maximum of 65,536 words.
 * The read-rewrite time of the memory is five microseconds, the basic computer rate. Driving currents
 * are automatically adjusted to compensate for temperature variations between 50 and 110 degrees
 * Fahrenheit. The core memory storage may be supplemented by up to 24 magnetic tape transports.
 *
 * INPUT-OUTPUT
 *
 * PDP-1 is designed to operate a variety of buffered input-output devices. Standard equipment consistes
 * of a perforated tape reader with a read speed of 400 lines per second, and alphanuermic typewriter for
 * on-line operation in both input and output, and a perforated tape punch (alphanumeric or binary) with
 * a speed of 63 lines per second. A variety of optional equipment is available, including the following:
 *
 *     Precision CRT Display Type 30
 *     Ultra-Precision CRT Display Type 31
 *     Symbol Generator Type 33
 *     Light Pen Type 32
 *     Oscilloscope Display Type 34
 *     Card Punch Control Type 40-1
 *     Card Reader and Control Type 421
 *     Magnetic Tape Transport Type 50
 *     Programmed Magnetic Tape Control Type 51
 *     Automatic Magnetic Tape Control Type 52
 *     Automatic Magnetic Tape Control Type 510
 *     Parallel Drum Type 23
 *     Automatic Line Printer and Control Type 64
 *     18-bit Real Time Clock
 *     18-bit Output Relay Buffer Type 140
 *     Multiplexed A-D Converter Type 138/139
 *
 * All in-out operations are performed through the In-Out Register or through the high speed input-output
 * channels.
 *
 * The PDP-1 is also available with the optional Sequence Break System. This is a multi-channel priority
 * interrupt feature which permits concurrent operation of several in-out devices. A one-channel Sequence
 * Break System is included in the standard PDP-1. Optional Sequence Break Systems consist of 16, 32, 64,
 * 128, and 256 channels.
 *
 * ...
 *
 * BASIC INSTRUCTIONS
 *
 *                                                                    OPER. TIME
 * INSTRUCTION  CODE #  EXPLANATION                                     (usec)
 * ------------------------------------------------------------------------------
 * add Y        40      Add C(Y) to C(AC)                                 10
 * and Y        02      Logical AND C(Y) with C(AC)                       10
 * cal Y        16      Equals jda 100                                    10
 * dac Y        24      Deposit C(AC) in Y                                10
 * dap Y        26      Deposit contents of address part of AC in Y       10
 * dio Y        32      Deposit C(IO) in Y                                10
 * dip Y        30      Deposit contents of instruction part of AC in Y   10
 * div Y        56      Divide                                          40 max
 * dzm Y        34      Deposit zero in Y                                 10
 * idx Y        44      Index (add one) C(Y), leave in Y & AC             10
 * ior Y        04      Inclusive OR C(Y) with C(AC)                      10
 * iot Y        72      In-out transfer, see below
 * isp Y        46      Index and skip if result is positive              10
 * jda Y        17      Equals dac Y and jsp Y+1                          10
 * jmp Y        60      Take next instruction from Y                      5
 * jsp Y        62      Jump to Y and save program counter in AC          5
 * lac Y        20      Load the AC with C(Y)                             10
 * law N        70      Load the AC with the number N                     5
 * law-N        71      Load the AC with the number -N                    5
 * lio Y        22      Load IO with C(Y)                                 10
 * mul Y        54      Multiply                                        25 max
 * opr          76      Operate, see below                                5
 * sad Y        50      Skip next instruction if C(AC) <> C(Y)            10
 * sas Y        52      Skip next instruction if C(AC) = C(Y)             10
 * sft          66      Shift, see below                                  5
 * skp          64      Skip, see below                                   5
 * sub Y        42      Subtract C(Y) from C(AC)                          10
 * xct Y        10      Execute instruction in Y                          5+
 * xor Y        06      Exclusive OR C(Y) with C(AC)                      10
 *
 * OPERATE GROUP
 *
 *                                                                    OPER. TIME
 * INSTRUCTION  CODE #   EXPLANATION                                    (usec)
 * ------------------------------------------------------------------------------
 * cla        760200     Clear AC                                         5
 * clf        76000f     Clear selected Program Flag (f = flag #)         5
 * cli        764000     Clear IO                                         5
 * cma        761000     Complement AC                                    5
 * hlt        760400     Halt                                             5
 * lap        760100     Load AC with Program Counter                     5
 * lat        762200     Load AC from Test Word switches                  5
 * nop        760000     No operation                                     5
 * stf        76001f     Set selected Program Flag                        5
 *
 * IN-OUT TRANSFER GROUP
 *
 * PERFORATED TAPE READER
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * rpa        720001     Read Perforated Tape Alphanumeric
 * rpb        720002     Read Perforated Tape Binary
 * rrb        720030     Read Reader Buffer
 *
 * PERFORATED TAPE PUNCH
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * ppa        720005     Punch Perforated Tape Alphanumeric
 * ppb        720006     Punch Perforated Tape Binary
 *
 * ALPHANUMERIC ON-LINE TYPEWRITER
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * tyo        720003     Type Out
 * tyi        720004     Type In
 *
 * SEQUENCE BREAK SYSTEM TYPE 120
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * esm        720055     Enter Sequence Break Mode
 * lsm        720054     Leave Sequence Break Mode
 * cbs        720056     Clear Sequence Break System
 * dsc        72kn50     Deactivate Sequence Break Channel
 * asc        72kn51     Activate Sequence Break Channel
 * isb        72kn52     Initiate Sequence Break
 * cac        720053     Clear All Channels
 *
 * HIGH SPEED DATA CONTROL TYPE 131
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * swc        72x046     Set Word Counter
 * sia        720346     Set Location Counter
 * sdf        720146     Stop Data Flow
 * rlc        720366     Read Location Counter
 * shr        720446     Set High Speed Channel Request
 *
 * PRECISION CRT DISPLAY TYPE 30
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * dpy        720007     Display One Point
 *
 * SYMBOL GENERATOR TYPE 33
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * gpl        722027     Generator Plot Left
 * gpr        720027     Generator Plot Right
 * glf        722026     Load Format
 * gsp        720026     Space
 * sdb        722007     Load Buffer, No Intensity
 *
 * ULTRA-PRECISION CRT DISPLAY TYPE 31
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * dpp        720407     Display One Point on Ultra Precision CRT
 *
 * CARD PUNCH CONTROL TYPE 40-1
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * lag        720044     Load a Group
 * pac        720043     Punch a Card
 *
 * CARD READER TYPE 421
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * rac        720041     Read Card Alpha
 * rbc        720042     Read Card Binary
 * rcc        720032     Read Card Column
 *
 * PROGRAMMED MAGNETIC TAPE CONTROL TYPE 51
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * msm        720073     Select Mode
 * mcs        720034     Check Status
 * mcb        720070     Clear Buffer
 * mwc        720071     Write a Character
 * mrc        720072     Read Character
 *
 * AUTOMATIC MAGNETIC TAPE CONTROL TYPE 52
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * muf        72ue76     Tape Unit and FinalT
 * mic        72ue75     Initial and Command
 * mrf        72u067     Reset Final
 * mri        72ug66     Reset Initial
 * mes        72u035     Examine States
 * mel        72u036     Examine Location
 * inr        72ur67     Initiate a High Speed Channel Request
 * ccr        72s067     Clear Command Register
 *
 * AUTOMATIC MAGNETIC TAPE CONTROL TYPE 510
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * sfc        720072     Skip if Tape Control Free
 * rsr        720172     Read State Register
 * crf        720272     Clear End-of-Record Flip-Flop
 * cpm        720472     Clear Proceed Mode
 * dur        72xx70     Load Density, Unit, Rewind
 * mtf        73xx71     Load Tape Function Register
 * cgo        720073     Clear Go
 *
 * MULTIPLEXED A-D CONVERTER TYPE 138/139
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * rcb        720031     Read Converter Buffer
 * cad        720040     Convert a Voltage
 * scv        72mm47     Select Multiplexer (1 of 64 Channels)
 * icv        720060     Index Multiplexer
 *
 * AUTOMATIC LINE PRINTER TYPE 64
 *
 * INSTRUCTION  CODE #   EXPLANATION
 * ------------------------------------------------------------------------------
 * clrbuf     722045     Clear Buffer
 * lpb        720045     Load Printer Buffer
 * pas        721x45     Print and Space
 *
 * SKIP GROUP
 *
 *                                                                    OPER. TIME
 * INSTRUCTION  CODE #   EXPLANATION                                    (usec)
 * ------------------------------------------------------------------------------
 * sma        640400     Dkip on minus AC                                 5
 * spa        640200     Skip on plus AC                                  5
 * spi        642000     Skip on plus IO                                  5
 * sza        640100     Skip on ZERO (+0) AC                             5
 * szf        6400f      Skip on ZERO flag                                5
 * szo        641000     Skip on ZERO overflow (and clear overflow)       5
 * szs        6400s0     Skip on ZERO sense switch                        5
 *
 * SHIFT/ROTATE GROUP
 *
 *                                                                      OPER. TIME
 * INSTRUCTION  CODE #   EXPLANATION                                      (usec)
 * ------------------------------------------------------------------------------
 *   ral        661      Rotate AC left                                     5
 *   rar        671      Rotate AC right                                    5
 *   rcl        663      Rotate Combined AC & IO left                       5
 *   rcr        673      Rotate Combined AC & IO right                      5
 *   ril        662      Rotate IO left                                     5
 *   rir        672      Rotate IO right                                    5
 *   sal        665      Shift AC left                                      5
 *   sar        675      Shift AC right                                     5
 *   scl        667      Shift Combined AC & IO left                        5
 *   scr        677      Shift Combined AC & IO right                       5
 *   sil        666      Shift IO left                                      5
 *   sir        676      Shift IO right                                     5
 */

#include <stdio.h>
#include <stdlib.h>
#include "osd_dbg.h"
#include "pdp1/pdp1.h"
#include "driver.h"

#define READ_PDP_18BIT(A) ((signed)cpu_readmem16(A))
#define WRITE_PDP_18BIT(A,V) (cpu_writemem16(A,V))

int intern_iot(int *io, int md);
int (* extern_iot)(int *, int)=intern_iot;
int execute_instruction(int md);

/* PDP1 registers */
static int pc;      /* Program counter */
static int ac;
static int io;
static int y;
static int ib;
static int ov;
static int f;
static int flag[8];
static int sense[8];
/* not only 6 flags/senses, but we start counting at 1 */

static int pending_interrupts;

/* public globals */
signed int pdp1_ICount=50000;

/****************************************************************************/
/* Set all registers to given values                                        */
/****************************************************************************/
void pdp1_SetRegs(pdp1_Regs *Regs)
{
 int i;
 pc = Regs->pc;
 ac = Regs->ac;
 io = Regs->io;
 ov = Regs->ov;
 y = Regs->y;
 for (i=0;i<7;i++)
 {
  flag[i]=Regs->flag[i];
  sense[i]=Regs->sense[i];
 }
 f=0;
 for (i=1;i<7;i++)
 {
  f+=(1<<(i-1))*(flag[i]!=0);
 }
 pending_interrupts = Regs->pending_interrupts;
}

/****************************************************************************/
/* Get all registers in given buffer                                        */
/****************************************************************************/

void pdp1_GetRegs(pdp1_Regs *Regs)
{
 int i;
 Regs->pc = pc;
 Regs->ac = ac;
 Regs->io = io;
 Regs->ov = ov;
 Regs->y = y;
 Regs->f = f;
 for (i=0;i<7;i++)
 {
  Regs->flag[i] =flag[i];
  Regs->sense[i]=sense[i];
 }
 Regs->pending_interrupts=pending_interrupts;
}
static pdp1_Regs Regst;
static int todo=0;
void pdp1_set_regs_true(pdp1_Regs *Regs)
{
 int i;
 Regst.pc = Regs->pc;
 Regst.ac = Regs->ac;
 Regst.io = Regs->io;
 Regst.ov = Regs->ov;
 Regst.y = Regs->y;
 for (i=0;i<7;i++)
 {
  Regst.flag[i]=Regs->flag[i];
  Regst.sense[i]=Regs->sense[i];
 }
 Regst.pending_interrupts = Regs->pending_interrupts;
 todo=1;
}
INLINE void pdp1_set_regs_truei(void)
{
 int i;
 if (todo==0) return;
 todo=0;
 pc = Regst.pc;
 ac = Regst.ac;
 io = Regst.io;
 ov = Regst.ov;
 y = Regst.y;
 for (i=0;i<7;i++)
 {
  flag[i]=Regst.flag[i];
  sense[i]=Regst.sense[i];
 }
 f=0;
 for (i=1;i<7;i++)
 {
  f+=(1<<(i-1))*(flag[i]!=0);
 }
 pending_interrupts = Regst.pending_interrupts;
}

/****************************************************************************/
/* Return program counter                                                   */
/****************************************************************************/
unsigned pdp1_GetPC(void)
{
 return pc;
}

/* Generate interrupts */
static void Interrupt(void)
{
}

void pdp1_Cause_Interrupt(int type)
{
}

void pdp1_Clear_Pending_Interrupts(void)
{
}

void pdp1_reset(void)
{
 int i;
 pc = 4;
 ac = 0;
 io = 0;
 ov = 0;
 y = 0;
 f = 0;
 for (i=0;i<7;i++)
 {
  flag[i]=0;
  sense[i]=0;
 }
 sense[5]=1; /* for lisp... typewriter input */
 pdp1_Clear_Pending_Interrupts();
}

/* execute instructions on this CPU until icount expires */
int pdp1_execute(int cycles)
{
 int word18;
 pdp1_ICount = cycles;
 pdp1_set_regs_truei();
 do
 {
  if (pending_interrupts != 0)
   Interrupt();
  #ifdef  MAME_DEBUG
  {
   extern int mame_debug;
   if (mame_debug)
    MAME_Debug();
  }
  #endif
  word18=READ_PDP_18BIT(pc++);
/*
  if (errorlog)
  {
   fprintf(errorlog, "PC:0%06o ",pc-1);
   fprintf(errorlog, "I:0%06o  ",word18);
   fprintf(errorlog, "1:%i "    ,flag[1]);
   fprintf(errorlog, "2:%i "    ,flag[2]);
   fprintf(errorlog, "3:%i "    ,flag[3]);
   fprintf(errorlog, "4:%i "    ,flag[4]);
   fprintf(errorlog, "5:%i "    ,flag[5]);
   fprintf(errorlog, "6:%i \n"  ,flag[6]);
  }
*/
  pdp1_ICount -= execute_instruction(word18);
 } while( pdp1_ICount>0 );
 return cycles - pdp1_ICount;
}

static int etime=0;
INLINE void ea(void)
{
 while(1)
 {
  if (ib==0)
   return;
  etime+=5;
  ib=(READ_PDP_18BIT(y)>>12)&1;
  y=READ_PDP_18BIT(y)&07777;
  if (etime>100)
  {
   if (errorlog)
   {
    fprintf(errorlog, "Massive indirection (>20), assuming emulator fault... bye at:\n");
    fprintf(errorlog, "PC:0%06o Y=0%06o\n",pc-1,y);
   }
   exit(1);
  }
 }
}


int execute_instruction(int md)
{
 static char buf[20];
 etime=0;
 y=md&07777;
 ib=(md>>12)&1; /* */
 switch (md>>13)
 {
  case AND: ea(); ac&=READ_PDP_18BIT(y); etime+=10; break;
  case IOR: ea(); ac|=READ_PDP_18BIT(y); etime+=10; break;
  case XOR: ea(); ac^=READ_PDP_18BIT(y); etime+=10; break;
  case XCT: ea(); etime+=5+execute_instruction(READ_PDP_18BIT(y)); break;
  case CALJDA:
  {
   int target=(ib==0)?64:y;
   WRITE_PDP_18BIT(target,ac);
   ac=(ov<<17)+pc;
   pc=target+1;
   etime+=10;
   break;
  }
  case LAC: ea(); ac=READ_PDP_18BIT(y); etime+=10; break;
  case LIO: ea(); io=READ_PDP_18BIT(y); etime+=10; break;
  case DAC: ea(); WRITE_PDP_18BIT(y,ac); etime+=10; break;
  case DAP: ea(); WRITE_PDP_18BIT(y,(READ_PDP_18BIT(y)&0770000)+(ac&07777)); etime+=10; break;
  case DIO: ea(); WRITE_PDP_18BIT(y,io); etime+=10; break;
  case DZM: ea(); WRITE_PDP_18BIT(y,0); etime+=10; break;
  case ADD:
   ea();
   ac=ac+READ_PDP_18BIT(y);
   ov=ac>>18;
   ac=(ac+ov)&0777777;
   if (ac==0777777)
    ac=0;
   etime+=10;
   break;
  case SUB:
  {
   int diffsigns;
   ea();
   diffsigns=((ac>>17)^(READ_PDP_18BIT(y)>>17))==1;
   ac=ac+(READ_PDP_18BIT(y)^0777777);
   ac=(ac+(ac>>18))&0777777;
   if (ac==0777777)
    ac=0;
   if (diffsigns&&(READ_PDP_18BIT(y)>>17==ac>>17))
    ov=1;
   etime+=10;
   break;
  }
  case IDX:
   ea();
   ac=READ_PDP_18BIT(y)+1;
   if (ac==0777777)
    ac=0;
   WRITE_PDP_18BIT(y,ac);
   etime+=10;
   break;
  case ISP:
   ea();
   ac=READ_PDP_18BIT(y)+1;
   if (ac==0777777)
    ac=0;
   WRITE_PDP_18BIT(y,ac);
   if ((ac&0400000)==0)
    pc++;
   etime+=10;
   break;
  case SAD:
   ea();
   if (ac!=READ_PDP_18BIT(y))
    pc++;
   etime+=10;
   break;
  case SAS:
   ea();
   if (ac==READ_PDP_18BIT(y))
    pc++;
   etime+=10;
   break;
  case MUS:
   ea();
   if ((io&1)==1)
   {
    ac=ac+READ_PDP_18BIT(y);
    ac=(ac+(ac>>18))&0777777;
    if (ac==0777777)
     ac=0;
   }
   io=(io>>1|ac<<17)&0777777;
   ac>>=1;
   etime+=10;
   break;
  case DIS:
  {
   int acl;
   ea();
   acl=ac>>17;
   ac=(ac<<1|io>>17)&0777777;
   io=((io<<1|acl)&0777777)^1;
   if ((io&1)==1)
   {
    ac=ac+(READ_PDP_18BIT(y)^0777777);
    ac=(ac+(ac>>18))&0777777;
   }
   else
   {
    ac=ac+1+READ_PDP_18BIT(y);
    ac=(ac+(ac>>18))&0777777;
   }
   if (ac==0777777)
    ac=0;
   etime+=10;
   break;
  }
  case JMP:
   ea();
   pc=y;
   etime+=5;
   break;
  case JSP:
   ea();
   ac=(ov<<17)+pc;
   pc=y;
   etime+=5;
   break;
  case SKP:
  {
   int cond =   (((y&0100)==0100)&&(ac==0)) ||
                (((y&0200)==0200)&&(ac>>17==0)) ||
                (((y&0400)==0400)&&(ac>>17==1)) ||
                (((y&01000)==01000)&&(ov==0)) ||
                (((y&02000)==02000)&&(io>>17==0))||
                (((y&7)!=0)&&!flag[y&7])||
                (((y&070)!=0)&&!sense[(y&070)>>3])||
                ((y&070)==010);
   if (ib==0)
   {
    if (cond)
     pc++;
   }
   else
   {
    if (!cond)
     pc++;
   }
   if ((y&01000)==01000)
   ov=0;
   etime+=5;
   break;
  }
  case SFT:
  {
   int nshift=0;
   int mask=md&0777;
   while (mask!=0)
   {
    nshift+=mask&1;
    mask=mask>>1;
   }
   switch ((md>>9)&017)
   {
    int i;
    case 1:
     for (i=0;i<nshift;i++)
      ac=(ac<<1|ac>>17)&0777777;
     break;
    case 2:
     for (i=0;i<nshift;i++)
      io=(io<<1|io>>17)&0777777;
     break;
    case 3:
     for (i=0;i<nshift;i++)
     {
      int tmp=ac;
      ac=(ac<<1|io>>17)&0777777;
      io=(io<<1|tmp>>17)&0777777;
     }
     break;
    case 5:
     for (i=0;i<nshift;i++)
      ac=((ac<<1|ac>>17)&0377777)+(ac&0400000);
     break;
    case 6:
    for (i=0;i<nshift;i++)
     io=((io<<1|io>>17)&0377777)+(io&0400000);
    break;
    case 7:
     for (i=0;i<nshift;i++)
     {
      int tmp=ac;
      ac=((ac<<1|io>>17)&0377777)+(ac&0400000); /* shouldn't that be io?, no it is the sign! */
      io=(io<<1|tmp>>17)&0777777;
     }
     break;
    case 9:
     for (i=0;i<nshift;i++)
      ac=(ac>>1|ac<<17)&0777777;
     break;
    case 10:
     for (i=0;i<nshift;i++)
      io=(io>>1|io<<17)&0777777;
     break;
    case 11:
     for (i=0;i<nshift;i++)
     {
      int tmp=ac;
      ac=(ac>>1|io<<17)&0777777;
      io=(io>>1|tmp<<17)&0777777;
     }
     break;
    case 13:
     for (i=0;i<nshift;i++)
      ac=(ac>>1)+(ac&0400000);
     break;
    case 14:
     for (i=0;i<nshift;i++)
      io=(io>>1)+(io&0400000);
     break;
    case 15:
     for (i=0;i<nshift;i++)
     {
      int tmp=ac;
      ac=(ac>>1)+(ac&0400000); /* shouldn't that be io, no it is the sign */
      io=(io>>1|tmp<<17)&0777777;
     }
     break;
    default:
     if (errorlog)
     {
      fprintf(errorlog,"Undefined shift: ");
      fprintf(errorlog,"0%06o at ",md);
      fprintf(errorlog,"0%06o\n",pc-1);
     }
     exit(1);
   }
   etime+=5;
   break;
  }
  case LAW:
   ac=(ib==0)?y:y^0777777;
   etime+=5;
   break;
  case IOT:
   etime+=extern_iot(&io,md);
   break;
  case OPR:
  {
   int nflag;
   int state;
   int i;
   etime+=5;
   if ((y&0200)==0200)
    ac=0;
   if ((y&04000)==04000)
    io=0;
   if ((y&01000)==01000)
    ac^=0777777;
   if ((y&0400)==0400)
   {
    if (errorlog)
    {
     /* ignored till I emulate the extention switches... with
      * continue...
      */
     fprintf(errorlog,"PDP1 Program executed HALT: at ");
     fprintf(errorlog,"0%06o\n",pc-1);
     fprintf(errorlog,"HALT ignored...\n");
    }
    /* exit(1); */
   }
   nflag=y&7;
   if (nflag<1) /* was 2 */
    break;
   state=(y&010)==010;
   if (nflag==7)
   {
    for (i=1;i<7;i++) /* was 2 */
    {
     flag[i]=state;
    }
    break;
   }
   flag[nflag]=state;
   f=0;
   for (i=1;i<7;i++)
   {
    f+=(1<<(i-1))*(flag[i]!=0);
   }
   break;
  }
  default:
   if (errorlog)
   {
    fprintf(errorlog,"Undefined instruction: ");
    fprintf(errorlog,"0%06o at ",md);
    fprintf(errorlog,"0%06o\n",pc-1);
   }
   exit(1);
 }
 return etime;
}

int intern_iot(int *iio, int md)
{
 if (errorlog)
  fprintf(errorlog, "No external IOT function given (io=0%06o) -> EXIT(1) invoked in PDP1\\PDP1.C\n",*iio);
 exit(1);
}


