/* Header File */
#include <stdio.h>
#include <stdint.h>

#include "board-peripherals.h"
#include "buzzer.h"
#include "contiki.h"
#include "sys/etimer.h"
#include "sys/rtimer.h"

/*----------------------------------------*/
/* Process Declaration */
PROCESS(task3, "Task3");
AUTOSTART_PROCESSES(&task3);

/*----------------------------------------*/
/* Variable Declaration */
static int mpuReadings[6] = {0};
static int luxReading = 0;
int buzzerFrequency[8] = {2093, 2349, 2637, 2794, 3156, 3520, 3951, 4186}; // hgh notes on a piano

static int state = 0;

static int f;
static struct rtimer rtimerTimer;
static rtimer_clock_t stateBeginTime;
static rtimer_clock_t sampleIntervalDuration = RTIMER_SECOND / 4;
static rtimer_clock_t buzzDuration = RTIMER_SECOND * 2;
static rtimer_clock_t waitDuration = RTIMER_SECOND * 2;

/*----------------------------------------*/
// Function Declaration
static void initOptSensor(void);
static int getLuxReading(void);
static int checkForLuxChange();
static void printOldAndNewLuxReading(int oldLuxReading, int newLuxReading);

static void initMpuSensor(void);
static void getMpuReading(int mpuReading[]);
static int checkForMpuChange();
static void printMpuReading(int reading);
static void printOldAndNewMpuReading(int oldMpuReading[], int newMpuReading[]);

static void transitToIdleState();
static void transitToInterimState();
static void transitToWaitState();
static void transitToBuzzState();

static void printStateDuration(rtimer_clock_t currentTime);

/*----------------------------------------*/
/* Function Definition */

// Init sensors
static void initMpuSensor(void)
{
    mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL); // IMU Sensor
}

// Adopted from examples/Assignment2/rtimer-IMUSensor.c
static void getMpuReading(int mpuReading[])
{
    printf("Reading MPU Sensor\n");
    mpuReading[0] = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
    mpuReading[1] = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
    mpuReading[2] = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);
    mpuReading[3] = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
    mpuReading[4] = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
    mpuReading[5] = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);
}

// Adopted from examples/Assignment2/rtimer-IMUSensor.c
static void printMpuReading(int reading)
{
    if (reading < 0)
    {
        printf("-");
        reading = -reading;
    }

    printf("%d.%02d", reading / 100, reading % 100);
}

static void printOldAndNewMpuReading(int oldMpuReading[], int newMpuReading[])
{
    printf("MPU Gyro: X= ");
    printMpuReading(oldMpuReading[0]);
    printf(" ==> ");
    printMpuReading(newMpuReading[0]);
    printf(" deg/sec\n");

    printf("MPU Gyro: Y= ");
    printMpuReading(oldMpuReading[1]);
    printf(" ==> ");
    printMpuReading(newMpuReading[1]);
    printf(" deg/sec\n");

    printf("MPU Gyro: Z= ");
    printMpuReading(oldMpuReading[2]);
    printf(" ==> ");
    printMpuReading(newMpuReading[2]);
    printf(" deg/sec\n");

    printf("MPU Acc: X= ");
    printMpuReading(oldMpuReading[3]);
    printf(" ==> ");
    printMpuReading(newMpuReading[3]);
    printf(" g\n");

    printf("MPU Acc: Y= ");
    printMpuReading(oldMpuReading[4]);
    printf(" ==> ");
    printMpuReading(newMpuReading[4]);
    printf(" g\n");

    printf("MPU Acc: Z= ");
    printMpuReading(oldMpuReading[5]);
    printf(" ==> ");
    printMpuReading(newMpuReading[5]);
    printf(" g\n");
}

static int checkForMpuChange()
{
    int prevMpuReading[6];
    for (int i = 0; i < 6; i++)
    {
        prevMpuReading[i] = mpuReadings[i];
    }

    getMpuReading(mpuReadings);
    printOldAndNewMpuReading(prevMpuReading, mpuReadings);

    // check for significant change in distance
    for (int i = 0; i < 3; i++)
    {
        if (abs(mpuReadings[i] - prevMpuReading[i]) > 5000)
        {
            printf("MPU: Significant change in distance\n");
            return 1;
        }
    }

    // check for significant change in acceleration
    for (int i = 3; i < 6; i++)
    {
        if (abs(mpuReadings[i] - prevMpuReading[i]) > 1000)
        {
            printf("MPU: Significant change in acceleration\n");
            return 1;
        }
    }

    printf("MPU: No significant change\n");
    return 0;
}

static void initOptSensor(void)
{
    SENSORS_ACTIVATE(opt_3001_sensor); // Light Sensor
}

// Adopted from examples/Assignment2/rtimer-lightSensor.c
static int getLuxReading(void)
{
    int value = opt_3001_sensor.value(0);

    if (value == CC26XX_SENSOR_READING_ERROR)
    {
        printf("OPT: CC26XX_SENSOR_READING_ERROR\n");
        value = -1;
    }
    initOptSensor(); // update new reading
    return value;
}

static int checkForLuxChange()
{
    int prevLuxReading = luxReading;
    luxReading = getLuxReading();
    printOldAndNewLuxReading(prevLuxReading, luxReading);

    if (luxReading == -1)
    { // error
        printf("OPT: Failed\n");
        return 0;
    }

    float prevLuxReadingFloat = prevLuxReading / 100 + (prevLuxReading % 100) / 100.0;
    float luxReadingFloat = luxReading / 100 + (luxReading % 100) / 100.0;

    if (abs(luxReadingFloat - prevLuxReadingFloat) < 300)
    { // check if change in lux is above 300
        printf("OPT: No significant change\n");
        return 0;
    }

    printf("OPT: Significant change\n");
    return 1; // significant change
}

static void printOldAndNewLuxReading(int oldLuxReading, int newLuxReading)
{
    printf("OPT: Light=%d.%02d lux ==> %d.%02d lux\n", oldLuxReading / 100, oldLuxReading % 100, newLuxReading / 100, newLuxReading % 100);
}

void transitToIdleState()
{
    printf("Transit to Idle State\n");
    state = 0;
}

void transitToInterimState()
{
    printf("Transit to Interim State\n");
    state = 1;
}

void transitToBuzzState()
{
    printf("Transit to Buzz State\n");
    state = 2;
}

void transitToWaitState()
{
    printf("Transit to Wait State\n");
    state = 3;
}

void printStateDuration(rtimer_clock_t currentTime)
{
    rtimer_clock_t stateDurationTick;
    rtimer_clock_t stateDurationSecond;

    stateDurationTick = currentTime - stateBeginTime;
    stateDurationSecond = stateDurationTick / RTIMER_SECOND;
    printf("%d ticks, %d seconds\n", stateDurationTick, stateDurationSecond);
}

// Adopted from examples/Assignment2/rtimer-lightSensor.c
void rtimerTimeout(struct rtimer *timer, void *ptr)
{
    rtimer_clock_t currentTime = RTIMER_NOW(); // get current time

    switch (state)
    {
    case 0:
        rtimer_set(&rtimerTimer, currentTime + sampleIntervalDuration, 0, rtimerTimeout, NULL);

        if (checkForMpuChange())
        { // if significant change in mpu
            // reset lux readings for interim state
            luxReading = getLuxReading();
            transitToInterimState();
        }

        printf("Idle State Duration: ");
        printStateDuration(currentTime);

        break;

    case 1:
        rtimer_set(&rtimerTimer, currentTime + sampleIntervalDuration, 0, rtimerTimeout, NULL);

        if (checkForLuxChange())
        { // if significant change in lux
            f = 0;
            transitToBuzzState();
        }

        printf("Interim State Duration: ");
        printStateDuration(currentTime);

        break;

    case 2:
        if (checkForLuxChange())
        { // if significant change in lux
            // reset mpu readings for idle state
            getMpuReading(mpuReadings);
            buzzer_stop();
            transitToIdleState();
        }
        else
        {
            buzzer_start(buzzerFrequency[f]);
            rtimer_set(&rtimerTimer, currentTime + buzzDuration, 0, rtimerTimeout, NULL); // buzz for 2 seconds
            transitToWaitState();
        }

        printf("Buzz State Starts: ");
        printStateDuration(currentTime);
        break;

    case 3:
        buzzer_stop();
        rtimer_set(&rtimerTimer, currentTime + waitDuration, 0, rtimerTimeout, NULL); // wait for 2 seconds

        initOptSensor();                                                              // update new reading
        rtimer_set(&rtimerTimer, currentTime + waitDuration, 0, rtimerTimeout, NULL); // wait for another 2 seconds

        f = (f + 1) % 8;
        transitToBuzzState();

        printf("Wait State Starts: ");
        printStateDuration(currentTime);

        break;
    }
}

/*----------------------------------------*/
/* Main Process */

PROCESS_THREAD(task3, ev, data)
{
    PROCESS_BEGIN();

    initOptSensor(); // init light sensor reading
    initMpuSensor(); // init IMU sensor reading
    stateBeginTime = RTIMER_NOW();

    while (1)
    {
        rtimer_set(&rtimerTimer, RTIMER_NOW() + sampleIntervalDuration, 0, rtimerTimeout, NULL);
        PROCESS_YIELD();
    }

    PROCESS_END();
}
/*----------------------------------------*/