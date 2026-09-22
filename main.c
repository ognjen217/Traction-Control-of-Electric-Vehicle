#include "driverlib.h"
#include "device.h"

#include "hardware/hardware_io.h"
#include "app/controller.h"

/*
 * Test 1:
 * TMS320F28379D -> PWM-DAC3/PWM-DAC4 -> HIL404 AI5/AI6
 *
 * ADC vrednosti se i dalje citaju samo radi dijagnostike,
 * ali se NE koriste za formiranje izlazne komande.
 *
 * Ocekivano za TEST_COMMAND_A = 4 A:
 *
 *   PWM compare = 4 / 60 * 1000 ≈ 67
 *
 * Na HIL strani:
 *   FL_input_raw ≈ 0.20 ... 0.22 V
 *   FR_input_raw ≈ 0.20 ... 0.22 V
 *
 * Nakon Gain = 19:
 *   iq_ref_FL_safe ≈ 4 A
 *   iq_ref_FR_safe ≈ 4 A
 */



static ControllerState g_controller_state;

volatile ControllerInputs g_inputs;
volatile ControllerOutputs g_outputs;

volatile uint32_t g_isr_count = 0UL;
volatile uint16_t g_fault_flags = 0U;

/* Dijagnostika zadatih izlaza. */
volatile float g_command_fl_a = 0.0f;
volatile float g_command_fr_a = 0.0f;

/* Dijagnostika PWM compare vrednosti je vec dostupna kroz:
 *
 * g_hardware_pwm_fl_compare
 * g_hardware_pwm_fr_compare
 */

/* ADC dijagnostika ostaje aktivna. */
volatile float g_measured_slip_fl = 0.0f;
volatile float g_measured_slip_fr = 0.0f;
volatile float g_measured_fx_fl_n = 0.0f;
volatile float g_measured_fx_fr_n = 0.0f;
volatile float g_measured_yaw_rate_rad_s = 0.0f;
volatile float g_measured_yaw_angle_rad = 0.0f;
volatile float g_measured_iq_driver_fl_a = 0.0f;
volatile float g_measured_iq_driver_fr_a = 0.0f;

__interrupt void controlISR(void);

int main(void)
{
    /*
     * Inicijalizacija sistemskog takta, watchdog-a
     * i osnovne konfiguracije mikrokontrolera.
     */
    Device_init();

    /*
     * Inicijalizacija GPIO podsistema.
     */
    Device_initGPIO();

    /*
     * Inicijalizacija PIE interrupt kontrolera
     * i interrupt vector tabele.
     */
    Interrupt_initModule();
    Interrupt_initVectorTable();

    /*
     * Inicijalizacija:
     *
     * - ADCA, ADCB i ADCC
     * - ePWM1 kao ADC trigger
     * - ePWM7A/ePWM7B kao PWM-DAC izlazi
     * - GPIO157/GPIO158
     */
    HardwareIO_init();
    Controller_init(&g_controller_state);

    /*
     * ADCC SOC3 je poslednja konverzija u ADC sekvenci,
     * pa se ADCC1 interrupt koristi kao control interrupt.
     */
    Interrupt_register(
        INT_ADCC1,
        &controlISR);

    Interrupt_enable(INT_ADCC1);

    /*
     * Global interrupt enable i real-time debug enable.
     */
    EINT;
    ERTM;

    /*
     * Omogucava TBCLKSYNC, odnosno pokrece ePWM brojače.
     */
    HardwareIO_start();

    while (1)
    {
        /*
         * Sva test logika se izvrsava u ADC interrupt rutini.
         */
        NOP;
    }
}

__interrupt void controlISR(void)
{
    ControllerInputs inputs;
    ControllerOutputs outputs = {0};

    g_isr_count++;

    /*
     * HIL404 -> ADC -> controller inputs.
     */
    HardwareIO_readInputs(&inputs);

    /*
     * Traction controller and safety allocator.
     *
     * Yaw controller is temporarily disabled in controller.c.
     */
    Controller_step(
        &inputs,
        &g_controller_state,
        &outputs);

    /*
     * Controller -> PWM-DAC -> HIL404.
     */
    HardwareIO_writeOutputs(&outputs);

    /*
     * CCS diagnostics.
     */
    g_inputs = inputs;
    g_outputs = outputs;

    g_fault_flags = inputs.fault_flags;

    g_command_fl_a = outputs.iq_safe_fl_a;
    g_command_fr_a = outputs.iq_safe_fr_a;

    g_measured_slip_fl = inputs.slip_fl;
    g_measured_slip_fr = inputs.slip_fr;

    g_measured_fx_fl_n = inputs.fx_fl_n;
    g_measured_fx_fr_n = inputs.fx_fr_n;

    g_measured_yaw_rate_rad_s =
        inputs.yaw_rate_rad_s;

    g_measured_yaw_angle_rad =
        inputs.yaw_angle_rad;

    g_measured_iq_driver_fl_a =
        inputs.iq_driver_fl_a;

    g_measured_iq_driver_fr_a =
        inputs.iq_driver_fr_a;

    HardwareIO_clearControlInterrupt();
}
