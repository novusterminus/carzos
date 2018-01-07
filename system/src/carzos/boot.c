// ----------------------------------------------------------------------------

// This module contains the startup code for a portable embedded
// C with no lib
//
// Control reaches here from the reset handler via jump or call.
//
// The actual steps performed by _start are:
// - copy the initialised data region(s)
// - clear the BSS region(s)
// - initialise the system
// - initialise the arc/argv
// - branch to main()
// - call _exit(), directly or via exit()
//
// The normal configuration is standalone, with all support
// functions implemented locally.
//
// For this to be called, the project linker must be configured without
// the startup sequence (-nostartfiles).

// ----------------------------------------------------------------------------

#include <stdint.h>
#include <_ansi.h>
#include <_syslist.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <limits.h>
#include <signal.h>

// ----------------------------------------------------------------------------

#if !defined(DEBUG)
extern void
__attribute__((noreturn))
__reset_hardware(void);
#endif

// ----------------------------------------------------------------------------

// Forward declaration

void
_exit(int code);

// ----------------------------------------------------------------------------

// On Release, call the hardware reset procedure.
// On Debug we just enter an infinite loop, to be used as landmark when halting
// the debugger.
//
// It can be redefined in the application, if more functionality
// is required.

void
__attribute__((weak))
_exit(int code __attribute__((unused)))
{
#if !defined(DEBUG)
  __reset_hardware();
#endif

  // TODO: write on trace
  while (1)
    ;
}

// ----------------------------------------------------------------------------

void
__attribute__((weak,noreturn))
abort(void)
{
  // trace_puts("abort(), exiting...");

  _exit(1);
}

// ----------------------------------------------------------------------------

#if !defined(OS_INCLUDE_STARTUP_GUARD_CHECKS)
#define OS_INCLUDE_STARTUP_GUARD_CHECKS (1)
#endif

// ----------------------------------------------------------------------------

// Begin address for the initialisation values of the .data section.
// defined in linker script
extern unsigned int _data_begin_rom;
// Begin address for the .data section; defined in linker script
extern unsigned int _data_begin_ram;
// End address for the .data section; defined in linker script
extern unsigned int _data_end_ram;

// Begin address for the .bss section; defined in linker script
extern unsigned int _bss_begin_ram;
// End address for the .bss section; defined in linker script
extern unsigned int _bss_end_ram;

void __initialize_args(int* p_argc, char*** p_argv);

// This is the standard default implementation for the routine to
// process args. It returns a single empty arg.
// For semihosting applications, this is redefined to get the real
// args from the debugger. You can also use it if you decide to keep
// some args in a non-volatile memory.

void __attribute__((weak))
__initialize_args(int* p_argc, char*** p_argv)
{
  // By the time we reach this, the data and bss should have been initialised.

  // The strings pointed to by the argv array shall be modifiable by the
  // program, and retain their last-stored values between program startup
  // and program termination. (static, no const)
  static char name[] = "";

  // The string pointed to by argv[0] represents the program name;
  // argv[0][0] shall be the null character if the program name is not
  // available from the host environment. argv[argc] shall be a null pointer.
  // (static, no const)
  static char* argv[2] =
    { name, NULL };

  *p_argc = 1;
  *p_argv = &argv[0];
  return;
}

// main() is the entry point for newlib based applications.
// By default, there are no arguments, but this can be customised
// by redefining __initialize_args(), which is done when the
// semihosting configurations are used.
extern int main(int argc, char* argv[]);

// The implementation for the exit routine; for embedded
// applications, a system reset will be performed.
extern void
__attribute__((noreturn))
_exit(int);

// ----------------------------------------------------------------------------

// Forward declarations

void _start(void);

void __initialize_data(unsigned int* from, unsigned int* region_begin, unsigned int* region_end);

void __initialize_bss(unsigned int* region_begin, unsigned int* region_end);

void __hardware_init_early(void);

void __hardware_init(void);

// ----------------------------------------------------------------------------

inline void
__attribute__((always_inline))
__initialize_data(unsigned int* from, unsigned int* region_begin, unsigned int* region_end)
{
  // Iterate and copy word by word.
  // It is assumed that the pointers are word aligned.
  unsigned int *p = region_begin;
  while (p < region_end)
    *p++ = *from++;
}

inline void
__attribute__((always_inline))
__initialize_bss(unsigned int* region_begin, unsigned int* region_end)
{
  // Iterate and clear word by word.
  // It is assumed that the pointers are word aligned.
  unsigned int *p = region_begin;
  while (p < region_end)
    *p++ = 0;
}

#if defined(DEBUG) && (OS_INCLUDE_STARTUP_GUARD_CHECKS)

// These definitions are used to check if the routines used to
// clear the BSS and to copy the initialised DATA perform correctly.

#define BSS_GUARD_BAD_VALUE (0xCADEBABA)

static uint32_t volatile __attribute__ ((section(".bss_begin"))) __bss_begin_guard;
static uint32_t volatile __attribute__ ((section(".bss_end"))) __bss_end_guard;

#define DATA_GUARD_BAD_VALUE (0xCADEBABA)
#define DATA_BEGIN_GUARD_VALUE (0x12345678)
#define DATA_END_GUARD_VALUE (0x98765432)

static uint32_t volatile __attribute__ ((section(".data_begin")))
__data_begin_guard = DATA_BEGIN_GUARD_VALUE;

static uint32_t volatile __attribute__ ((section(".data_end")))
__data_end_guard = DATA_END_GUARD_VALUE;

#endif // defined(DEBUG) && (OS_INCLUDE_STARTUP_GUARD_CHECKS)

// This is the place where Cortex-M core will go immediately after reset,
// via a call or jump from the Reset_Handler.
//
// For the call to work, and for the call to __hardware_init_early()
// to work, the reset stack must point to a valid internal RAM area.

void __attribute__ ((section(".after_vectors"),noreturn,weak))
_start(void)
{

  // Initialise hardware right after reset, to switch clock to higher
  // frequency and have the rest of the initialisations run faster.
  //
  // Mandatory on platforms like Kinetis, which start with the watch dog
  // enabled and require an early sequence to disable it.
  //
  // Also useful on platform with external RAM, that need to be
  // initialised before filling the BSS section.

	__hardware_init_early();

  // Use Old Style DATA and BSS section initialisation,
  // that will manage a single BSS sections.

#if defined(DEBUG) && (OS_INCLUDE_STARTUP_GUARD_CHECKS)
  __data_begin_guard = DATA_GUARD_BAD_VALUE;
  __data_end_guard = DATA_GUARD_BAD_VALUE;
#endif

  // Copy the DATA segment from Flash to RAM (inlined).
  __initialize_data(&_data_begin_rom, &_data_begin_ram, &_data_end_ram);

#if defined(DEBUG) && (OS_INCLUDE_STARTUP_GUARD_CHECKS)
  if ((__data_begin_guard != DATA_BEGIN_GUARD_VALUE)
      || (__data_end_guard != DATA_END_GUARD_VALUE))
    {
      for (;;)
	;
    }
#endif

#if defined(DEBUG) && (OS_INCLUDE_STARTUP_GUARD_CHECKS)
  __bss_begin_guard = BSS_GUARD_BAD_VALUE;
  __bss_end_guard = BSS_GUARD_BAD_VALUE;
#endif

  // Zero fill the BSS section (inlined).
  __initialize_bss(&_bss_begin_ram, &_bss_end_ram);

#if defined(DEBUG) && (OS_INCLUDE_STARTUP_GUARD_CHECKS)
  if ((__bss_begin_guard != 0) || (__bss_end_guard != 0))
    {
      for (;;)
	;
    }
#endif

  // Hook to continue the initialisations. Usually compute and store the
  // clock frequency in the global CMSIS variable, cleared above.
  __hardware_init();

  // Get the argc/argv (useful in semihosting configurations).
  int argc;
  char** argv;
  __initialize_args(&argc, &argv);

  // Call the main entry point, and save the exit code.
  int code = main(argc, argv);

  _exit(code);

  // Should never reach this, _exit() should have already
  // performed a reset.
  for (;;)
    ;
}

// ----------------------------------------------------------------------------
