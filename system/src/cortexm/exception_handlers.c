// ----------------------------------------------------------------------------

#include <cortexm/exception_handlers.h>
#include <stm32f4xx.h>
#include <string.h>

// ----------------------------------------------------------------------------

extern void __attribute__((noreturn,weak)) _start (void);

// ----------------------------------------------------------------------------
// Default exception handlers. Override the ones here by defining your own
// handler routines in your application code.
// ----------------------------------------------------------------------------

#if defined(DEBUG)

// The DEBUG version is not naked, but has a proper stack frame,
// to allow setting breakpoints at Reset_Handler.
void __attribute__ ((section(".after_vectors"),noreturn))
Reset_Handler (void)
{
  _start ();
}

#else

// The Release version is optimised to a quick branch to _start.
void __attribute__ ((section(".after_vectors"),naked))
Reset_Handler(void)
  {
    asm volatile
    (
        " ldr     r0,=_start \n"
        " bx      r0"
        :
        :
        :
    );
  }

#endif

void __attribute__ ((section(".after_vectors"),weak))
NMI_Handler (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

// ----------------------------------------------------------------------------

#if defined(TRACE)

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

// The values of BFAR and MMFAR stay unchanged if the BFARVALID or
// MMARVALID is set. However, if a new fault occurs during the
// execution of this fault handler, the value of the BFAR and MMFAR
// could potentially be erased. In order to ensure the fault addresses
// accessed are valid, the following procedure should be used:
// 1. Read BFAR/MMFAR.
// 2. Read CFSR to get BFARVALID or MMARVALID. If the value is 0, the
//    value of BFAR or MMFAR accessed can be invalid and can be discarded.
// 3. Optionally clear BFARVALID or MMARVALID.
// (See Joseph Yiu's book).

void
dumpExceptionStack (ExceptionStackFrame* frame,
                uint32_t cfsr, uint32_t mmfar, uint32_t bfar,
                                        uint32_t lr)
{
	/*
  trace_printf ("Stack frame:\n");
  trace_printf (" R0 =  %08X\n", frame->r0);
  trace_printf (" R1 =  %08X\n", frame->r1);
  trace_printf (" R2 =  %08X\n", frame->r2);
  trace_printf (" R3 =  %08X\n", frame->r3);
  trace_printf (" R12 = %08X\n", frame->r12);
  trace_printf (" LR =  %08X\n", frame->lr);
  trace_printf (" PC =  %08X\n", frame->pc);
  trace_printf (" PSR = %08X\n", frame->psr);
  trace_printf ("FSR/FAR:\n");
  trace_printf (" CFSR =  %08X\n", cfsr);
  trace_printf (" HFSR =  %08X\n", SCB->HFSR);
  trace_printf (" trace_printfDFSR =  %08X\n", SCB->DFSR);
   (" AFSR =  %08X\n", SCB->AFSR);
  */

  if (cfsr & (1UL << 7))
    {
//      trace_printf (" MMFAR = %08X\n", mmfar);
    }
  if (cfsr & (1UL << 15))
    {
//      trace_printf (" BFAR =  %08X\n", bfar);
    }
//  trace_printf ("Misc\n");
//  trace_printf (" LR/EXC_RETURN= %08X\n", lr);
}

#endif // defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

#if defined(__ARM_ARCH_6M__)

void
dumpExceptionStack (ExceptionStackFrame* frame, uint32_t lr)
{
	/*
  trace_printf ("Stack frame:\n");
  trace_printf (" R0 =  %08X\n", frame->r0);
  trace_printf (" R1 =  %08X\n", frame->r1);
  trace_printf (" R2 =  %08X\n", frame->r2);
  trace_printf (" R3 =  %08X\n", frame->r3);
  trace_printf (" R12 = %08X\n", frame->r12);
  trace_printf (" LR =  %08X\n", frame->lr);
  trace_printf (" PC =  %08X\n", frame->pc);
  trace_printf (" PSR = %08X\n", frame->psr);
  trace_printf ("Misc\n");
  trace_printf (" LR/EXC_RETURN= %08X\n", lr);
  */
}

#endif // defined(__ARM_ARCH_6M__)

#endif // defined(TRACE)

// ----------------------------------------------------------------------------

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

// Hard Fault handler wrapper in assembly.
// It extracts the location of stack frame and passes it to handler
// in C as a pointer. We also pass the LR value as second
// parameter.
// (Based on Joseph Yiu's, The Definitive Guide to ARM Cortex-M3 and
// Cortex-M4 Processors, Third Edition, Chap. 12.8, page 402).

void __attribute__ ((section(".after_vectors"),weak,naked))
HardFault_Handler (void)
{
  asm volatile(
      " tst lr,#4       \n"
      " ite eq          \n"
      " mrseq r0,msp    \n"
      " mrsne r0,psp    \n"
      " mov r1,lr       \n"
      " ldr r2,=HardFault_Handler_C \n"
      " bx r2"

      : /* Outputs */
      : /* Inputs */
      : /* Clobbers */
  );
}

void __attribute__ ((section(".after_vectors"),weak,used))
HardFault_Handler_C (ExceptionStackFrame* frame __attribute__((unused)),
                     uint32_t lr __attribute__((unused)))
{
#if defined(TRACE)
  uint32_t mmfar = SCB->MMFAR; // MemManage Fault Address
  uint32_t bfar = SCB->BFAR; // Bus Fault Address
  uint32_t cfsr = SCB->CFSR; // Configurable Fault Status Registers
#endif



#if defined(TRACE)
//  trace_printf ("[HardFault]\n");
  dumpExceptionStack (frame, cfsr, mmfar, bfar, lr);
#endif // defined(TRACE)

#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

#endif // defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)


#if defined(__ARM_ARCH_6M__)

// Hard Fault handler wrapper in assembly.
// It extracts the location of stack frame and passes it to handler
// in C as a pointer. We also pass the LR value as second
// parameter.
// (Based on Joseph Yiu's, The Definitive Guide to ARM Cortex-M0
// First Edition, Chap. 12.8, page 402).

void __attribute__ ((section(".after_vectors"),weak,naked))
HardFault_Handler (void)
{
  asm volatile(
      " movs r0,#4      \n"
      " mov r1,lr       \n"
      " tst r0,r1       \n"
      " beq 1f          \n"
      " mrs r0,psp      \n"
      " b   2f          \n"
      "1:               \n"
      " mrs r0,msp      \n"
      "2:"
      " mov r1,lr       \n"
      " ldr r2,=HardFault_Handler_C \n"
      " bx r2"

      : /* Outputs */
      : /* Inputs */
      : /* Clobbers */
  );
}

void __attribute__ ((section(".after_vectors"),weak,used))
HardFault_Handler_C (ExceptionStackFrame* frame __attribute__((unused)),
                     uint32_t lr __attribute__((unused)))
{
  // There is no semihosting support for Cortex-M0, since on ARMv6-M
  // faults are fatal and it is not possible to return from the handler.

#if defined(TRACE)
//  trace_printf ("[HardFault]\n");
  dumpExceptionStack (frame, lr);
#endif // defined(TRACE)

#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

#endif // defined(__ARM_ARCH_6M__)


#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

void __attribute__ ((section(".after_vectors"),weak))
MemManage_Handler (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

void __attribute__ ((section(".after_vectors"),weak,naked))
BusFault_Handler (void)
{
  asm volatile(
      " tst lr,#4       \n"
      " ite eq          \n"
      " mrseq r0,msp    \n"
      " mrsne r0,psp    \n"
      " mov r1,lr       \n"
      " ldr r2,=BusFault_Handler_C \n"
      " bx r2"

      : /* Outputs */
      : /* Inputs */
      : /* Clobbers */
  );
}

void __attribute__ ((section(".after_vectors"),weak,used))
BusFault_Handler_C (ExceptionStackFrame* frame __attribute__((unused)),
                    uint32_t lr __attribute__((unused)))
{
#if defined(TRACE)
  uint32_t mmfar = SCB->MMFAR; // MemManage Fault Address
  uint32_t bfar = SCB->BFAR; // Bus Fault Address
  uint32_t cfsr = SCB->CFSR; // Configurable Fault Status Registers

//  trace_printf ("[BusFault]\n");
  dumpExceptionStack (frame, cfsr, mmfar, bfar, lr);
#endif // defined(TRACE)

#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

void __attribute__ ((section(".after_vectors"),weak,naked))
UsageFault_Handler (void)
{
  asm volatile(
      " tst lr,#4       \n"
      " ite eq          \n"
      " mrseq r0,msp    \n"
      " mrsne r0,psp    \n"
      " mov r1,lr       \n"
      " ldr r2,=UsageFault_Handler_C \n"
      " bx r2"

      : /* Outputs */
      : /* Inputs */
      : /* Clobbers */
  );
}

void __attribute__ ((section(".after_vectors"),weak,used))
UsageFault_Handler_C (ExceptionStackFrame* frame __attribute__((unused)),
                      uint32_t lr __attribute__((unused)))
{
#if defined(TRACE)
  uint32_t mmfar = SCB->MMFAR; // MemManage Fault Address
  uint32_t bfar = SCB->BFAR; // Bus Fault Address
  uint32_t cfsr = SCB->CFSR; // Configurable Fault Status Registers
#endif

#if defined(TRACE)
//  trace_printf ("[UsageFault]\n");
  dumpExceptionStack (frame, cfsr, mmfar, bfar, lr);
#endif // defined(TRACE)

#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

#endif

void __attribute__ ((section(".after_vectors"),weak))
SVC_Handler (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

void __attribute__ ((section(".after_vectors"),weak))
DebugMon_Handler (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

#endif

void __attribute__ ((section(".after_vectors"),weak))
PendSV_Handler (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

void __attribute__ ((section(".after_vectors"),weak))
SysTick_Handler (void)
{
  // DO NOT loop, just return.
  // Useful in case someone (like STM HAL) inadvertently enables SysTick.
  ;
}

// ----------------------------------------------------------------------------
