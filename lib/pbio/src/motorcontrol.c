#include <pbio/motorcontrol.h>
#include <stdatomic.h>
#include <pbdrv/time.h>
#include <stdlib.h>
#include <math.h>

// A "don't care" constant for readibility of the code, but which is never used after assignment
// Typically used for parameters that have no effect for the selected maneuvers.
#define NONE (0)
#define max_abs_accl (1000000)

// Units and prescalers to enable integer divisions
#define NUM_SCALE (10000)
#define DEN_SCALE (US_PER_SECOND / NUM_SCALE)

/**
 * Unsigned integer type with units of microseconds
 */
typedef uint32_t ustime_t;

/**
 * Integer type with units of encoder counts
 */
typedef int32_t count_t;

/**
 * Integer type with units of encoder counts per second
 */
typedef int32_t rate_t;

/**
 * Integer type with units of encoder counts per second per second
 */
typedef int32_t accl_t;

/**
 * Motor control actions
 */
typedef enum {
    IDLE,
    RUN,
    STOP,
    RUN_TIME,
    RUN_STALLED,
    RUN_ANGLE,
    RUN_TARGET,
    TRACK_TARGET,
} pbio_motor_action_t;


// Atomic flag, one for each motor, that is set when the trajectory structure is currently being read or being written. It is clear when it is free.
volatile atomic_flag claimed_trajectory[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        false
    }
};

// Atomic flag that is set when the motor is currently busy with a run command
volatile atomic_flag motor_busy[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        false
    }
};

/**
 * Motor trajectory parameters for an ideal maneuver without disturbances
 */
typedef struct _pbio_motor_trajectory_t {
    pbio_motor_action_t action; // Motor action type
    pbio_motor_after_stop_t after_stop; // BRAKE, COAST or HOLD after maneuver
    ustime_t time_start;        // Time at start of maneuver
    ustime_t time_in;           // Time after the acceleration in-phase
    ustime_t time_out;          // Time at start of acceleration out-phase
    ustime_t time_end;          // Time at end of maneuver
    count_t count_start;        // Encoder count at start of maneuver
    count_t count_in;           // Encoder count after the acceleration in-phase
    count_t count_out;          // Encoder count at start of acceleration out-phase
    count_t count_end;          // Encoder count at end of maneuver
    rate_t rate_start;          // Encoder rate at start of maneuver
    rate_t rate_target;         // Encoder rate target when not accelerating
    accl_t accl_start;          // Encoder acceleration during in-phase
    accl_t accl_end;            // Encoder acceleration during out-phase
} pbio_motor_trajectory_t;

// Initialize current command to idle
pbio_motor_trajectory_t trajectories[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]{
        .action = IDLE,
        .time_start = NONE,
    }
};

// Wait for completion if requested.
void wait_for_completion(pbio_port_t port, pbio_motor_wait_t wait){
    // Set the flag that the motor is running
    while(!atomic_flag_test_and_set(&motor_busy[PORT_TO_IDX(port)]));
    // Check if we want to pause until motion is complete
    if (wait == PBIO_MOTOR_WAIT_COMPLETION) {
        // Wait until the running flag is cleared by the control task
        while(atomic_flag_test_and_set(&motor_busy[PORT_TO_IDX(port)])){
            // Sleep to give the motor control task time and clear the flag when done
            pbdrv_time_sleep_msec(1);
        }
        // In the process of reading above, the flag was set, so we need to clear it
        atomic_flag_clear(&motor_busy[PORT_TO_IDX(port)]);
    }
}

pbio_error_t make_motor_command(pbio_port_t port,
                                pbio_motor_action_t action,
                                float_t rate_target,
                                float_t duration_or_target_count,
                                pbio_motor_after_stop_t after_stop);

pbio_error_t pbio_encmotor_run(pbio_port_t port, float_t speed){
    pbio_error_t err = make_motor_command(port, RUN, speed, NONE, NONE);
    if (err != PBIO_SUCCESS){
        return err;
    }
    wait_for_completion(port, PBIO_MOTOR_WAIT_NONE);
    return PBIO_SUCCESS;    
}

pbio_error_t pbio_encmotor_stop(pbio_port_t port, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    return PBIO_ERROR_FAILED; // Not yet implemented in theory
}

pbio_error_t pbio_encmotor_run_time(pbio_port_t port, float_t speed, float_t duration, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    pbio_error_t err = make_motor_command(port, RUN_TIME, speed, duration, after_stop);
    if (err != PBIO_SUCCESS){
        return err;
    }
    wait_for_completion(port, wait);
    return PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_run_stalled(pbio_port_t port, float_t speed, float_t *stallpoint, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    pbio_error_t err = make_motor_command(port, RUN_STALLED, speed, NONE, after_stop);
    if (err != PBIO_SUCCESS){
        return err;
    }
    wait_for_completion(port, wait);
    if (wait == PBIO_MOTOR_WAIT_COMPLETION) { 
        pbio_encmotor_get_angle(port, stallpoint);
    };
    return PBIO_SUCCESS;    
}

pbio_error_t pbio_encmotor_run_angle(pbio_port_t port, float_t speed, float_t angle, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    pbio_error_t err = make_motor_command(port, RUN_ANGLE, speed, angle, after_stop);
    if (err != PBIO_SUCCESS){
        return err;
    }
    wait_for_completion(port, wait);
    return PBIO_SUCCESS;    
}

pbio_error_t pbio_encmotor_run_target(pbio_port_t port, float_t speed, float_t target, pbio_motor_after_stop_t after_stop, pbio_motor_wait_t wait){
    pbio_error_t err = make_motor_command(port, RUN_TARGET, speed, target, after_stop);
    if (err != PBIO_SUCCESS){
        return err;
    }
    wait_for_completion(port, wait);
    return PBIO_SUCCESS;    
}

pbio_error_t pbio_encmotor_track_target(pbio_port_t port, float_t target){
    return PBIO_ERROR_FAILED; // Not yet implemented in theory
}

void debug_trajectory(pbio_port_t port){
    pbio_motor_trajectory_t *traject = &trajectories[PORT_TO_IDX(port)];
    printf("\nPort       : %c\nAction     : %d\nAfter stop : %d\ntime_start : %u\ntime_in    : %u\ntime_out   : %u\ntime_end   : %u\ncount_start: %d\ncount_in   : %d\ncount_out  : %d\ncount_end  : %d\nrate_start : %d\nrate_target: %d\naccl_start : %d\naccl_end   : %d\n", 
        port,
        (int)traject->action,
        (int)traject->after_stop,
        (unsigned int)traject->time_start,
        (unsigned int)(traject->time_in-traject->time_start),
        (unsigned int)(traject->time_out-traject->time_start),
        (unsigned int)(traject->time_end-traject->time_start),
        (int)traject->count_start,
        (int)traject->count_in,
        (int)traject->count_out,
        (int)traject->count_end,
        (int)traject->rate_start,
        (int)traject->rate_target,
        (int)traject->accl_start,
        (int)traject->accl_end
    );
}

// Return max(-limit, min(value, limit)): Limit the magnitude of value to be equal to or less than provided limit
float_t limit(float_t value, float_t limit){
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

// Return 'value' with the sign of 'signof'. Equivalent to: sgn(signof)*abs(value)
float_t signval(float_t signof, float_t value) {
    if (signof > 0) {
        return abs(value);
    }
    if (signof < 0){
        return -abs(value);
    }
    return 0;
}

// Calculate the characteristic time values, encoder values, rate values and accelerations that uniquely define the rate and count trajectories
pbio_error_t make_motor_command(pbio_port_t port,
                                pbio_motor_action_t action,
                                float_t rate_target,
                                float_t duration_or_target_count,
                                pbio_motor_after_stop_t after_stop){    

    // Read the current system state for this motor
    ustime_t time_start = pbdrv_time_get_usec();
    count_t count_start;
    rate_t rate_start;
    pbio_encmotor_get_encoder_count(port, &count_start);
    pbio_encmotor_get_encoder_rate(port, &rate_start);
    pbio_motor_trajectory_t *traject = &trajectories[PORT_TO_IDX(port)];
    pbio_encmotor_settings_t *settings = &encmotor_settings[PORT_TO_IDX(port)];


    // Work with floats for now (only in this function, which is called only once when a maneuver is called.)
    float_t _time_start = ((float_t) time_start)/US_PER_SECOND;
    float_t _time_in = 0;
    float_t _time_out = 0;
    float_t _time_end = 0;
    float_t _count_start = count_start;
    float_t _count_in = 0;
    float_t _count_out = 0;
    float_t _count_end = 0;
    float_t _rate_start = rate_start;
    float_t _rate_target = rate_target * settings->counts_per_output_unit;
    float_t _accl_start = 0;
    float_t _accl_end = 0;

    // Set endpoint (time or angle), depending on selected action
    switch(action){
        case IDLE:
            // TODO
            break;           
        case STOP:
            // TODO
            break;            
        case RUN:
            // FOR RUN and RUN_STALLED, we specify no end time
            _time_end = NONE;        
            break;
        case RUN_TIME:
            // Do not allow negative time
            if (duration_or_target_count < 0) {
                return PBIO_ERROR_INVALID_ARG;
            }
            // For RUN_TIME, the end time is the current time plus the duration
            _time_end = _time_start + duration_or_target_count / US_PER_SECOND;
            break;
        case RUN_STALLED:
            // FOR RUN and RUN_STALLED, we specify no end time
            _time_end = NONE;        
            break;
        case RUN_ANGLE:
            // For RUN_ANGLE, we specify instead the end count value as the current value plus the requested angle
            _count_end = _count_start + duration_or_target_count * settings->counts_per_output_unit;
            // If the goal is to reach a relative target, the speed cannot not be zero
            if (rate_target == 0) {
                return PBIO_ERROR_INVALID_ARG;
            }
            break;
        case RUN_TARGET:
            // For RUN_TARGET, we specify instead the end count value
            _count_end = duration_or_target_count * settings->counts_per_output_unit;
            // If the goal is to reach a position target, the speed cannot not be zero
            if (rate_target == 0) {
                return PBIO_ERROR_INVALID_ARG;
            }        
            break;
        case TRACK_TARGET:
            // TODO
            break;
    }

    // If the specified endpoint (angle or finite time) is equal to the corresponding starting value, return an empty maneuver.
    if ( ((action == RUN_TARGET  || action == RUN_ANGLE) && ((count_t) _count_end) == count_start) ||
          (action == RUN_TIME  && ((ustime_t) (_time_end*US_PER_SECOND)) <= time_start)) {
        while(atomic_flag_test_and_set(&claimed_trajectory[PORT_TO_IDX(port)])); // Remove once we remove multithreading
        traject->time_in = time_start;
        traject->time_out = time_start;
        traject->time_end = time_start;
        traject->count_in = count_start;
        traject->count_out = count_start;
        traject->count_end = count_start;
        traject->rate_start = 0;
        traject->rate_target = 0;
        traject->accl_start = 0;
        traject->accl_end = 0;
        traject->action = action;
        traject->after_stop = after_stop;
        atomic_flag_clear(&claimed_trajectory[PORT_TO_IDX(port)]);  // Remove once we remove multithreading
        return PBIO_SUCCESS;
    }

    // Limit reference rates
    _rate_start = limit(_rate_start, settings->max_rate);
    _rate_target = limit(_rate_target, settings->max_rate);

    // Determine sign of reference rate in case of position target. The rate sign specified by the user is ignored
    if (action == RUN_TARGET) {
        // If the target is ahead of us, go forward. Otherwise go backward.
        _rate_target = (_count_end > _count_start) ? abs(_rate_target) : -abs(_rate_target);
    }

    // For time based control, the direction is taken into account as well
    if (action == RUN || action == RUN_TIME || action == RUN_STALLED) {
        _rate_target = _rate_target;
    }
    
    // To reduce complexity for now, we assume that the direction does not change during the acceleration phase.
    // If a reversal is requested, this therefore means an immediate reveral, and then a smooth acceleration to the
    // desired rate. This can be improved in future versions.
    if ((_rate_target < 0 && _rate_start > 0) || (_rate_target > 0 && _rate_start < 0)){
        _rate_start = 0;
    }

    // Accelerations with sign
    _accl_start = (_rate_target > _rate_start) ? settings->abs_accl_start : -settings->abs_accl_start;
    _accl_end = _rate_target > 0 ? -settings->abs_accl_end : settings->abs_accl_end;
    
    // Limit reference speeds if move is shorter than full in/out phase (time_based case)
    if (action == RUN_TIME && _time_end - _time_start < (_rate_target-_rate_start)/_accl_start - _rate_target/_accl_end) {
        // If we are here, there is not enough time to fully accelerate and decelerate as desired.
        // If the initial rate is less than the target rate, we can reduce the target rate to account for this.
        if (abs(_rate_start) < abs(_rate_target)) {
            _rate_target = _accl_end*_accl_start/(_accl_end-_accl_start)*(_time_end-_time_start + _rate_start/_accl_start);
        }
        // Otherwise, disable the initial acceleration phase, and check if this gives enough time to decelerate
        else {
            // Set to maximum initial acceleration
            _accl_start = signval(_accl_start, settings->abs_accl_start);

            // If there is not even enough time for just the out-phase, reduce that phase too.
            if ( _time_end - _time_start < -_rate_target/_accl_end) {
                // Limit the target speed such that if we decellerate at the desired rate, we reach zero speed at the end time.
                _rate_target = -_accl_end*(_time_end-_time_start);
            }
            // Limit the start rate by the reduced target rate
            _rate_start = _rate_target;
        }

    }

    // Limit reference speeds if move is shorter than full in/out phase (RUN_TARGET case)
    if (action == RUN_TARGET && abs(_count_end-_count_start) < abs((_rate_target*_rate_target-_rate_start*_rate_start)/(2*_accl_start)) + abs(_rate_target*_rate_target/(2*_accl_end))) {
        // There is not enough angle for the in and out phase
        if (abs(_rate_start) < abs(_rate_target)) {
            // Limit _rate_target to make in-and-out intersect because _rate_start is low enough
            _rate_target = signval(_rate_target, sqrt(abs(_accl_start*_accl_end/(_accl_end-_accl_start)*(2*_count_end-2*_count_start+_rate_start*_rate_start/_accl_start))));
        }
        else {
            // Let us disable the in-phase, and check if there is sufficient angle for out-phase
            _accl_start = signval(_accl_start, max_abs_accl);
            if (abs(_count_end-_count_start) < abs(_rate_target*_rate_target/_accl_end/2)) {
                // Limit _rate_target as well to at least make the out-phase feasible
                _rate_target = signval(_rate_target, sqrt(2*_accl_end*(_count_start-_count_end)));
            }
            // Limit the start rate by the reduced target rate
            _rate_start = _rate_target;
        }   
    }

    // Compute intermediate time and angle values just after initial acceleration
    _time_in = _time_start + (_rate_target-_rate_start)/_accl_start;
    _count_in = _count_start + ((_rate_target*_rate_target)-(_rate_start*_rate_start))/_accl_start/2;

    // Compute intermediate time and angle values just before deceleration and end time or end angle, depending on which is already given
    if (action == RUN_TIME) {
        _time_out = _time_end + _rate_target/_accl_end;
        _count_out = _count_in + _rate_target*(_time_out-_time_in);
        _count_end = _count_out - _rate_target*_rate_target/_accl_end/2;
    }
    else if (action == RUN_TARGET) {
        _time_out = _time_in + (_count_end-_count_in)/_rate_target + _rate_target/_accl_end/2;
        _count_out = _count_in + _rate_target*(_time_out-_time_in);
        _time_end = _time_out - _rate_target/_accl_end;
    }    
    else {
        _time_out = NONE;
        _time_end = NONE;
        _count_out = NONE;
        _count_end = NONE;        
    }

    // Convert temporary float results back to integers 
    while(atomic_flag_test_and_set(&claimed_trajectory[PORT_TO_IDX(port)])); // Remove once we remove multithreading
    traject->time_start = _time_start * US_PER_SECOND;
    traject->time_in = _time_in * US_PER_SECOND;
    traject->time_out = _time_out * US_PER_SECOND;
    traject->time_end = _time_end * US_PER_SECOND;
    traject->count_start = _count_start;
    traject->count_in = _count_in;
    traject->count_out = _count_out;
    traject->count_end = _count_end;
    traject->rate_start = _rate_start;
    traject->rate_target = _rate_target;
    traject->accl_start = _accl_start;
    traject->accl_end = _accl_end;
    traject->action = action;
    traject->after_stop = after_stop;
    atomic_flag_clear(&claimed_trajectory[PORT_TO_IDX(port)]);  // Remove once we remove multithreading
    return PBIO_SUCCESS;
}



// Several of the formulas below contain expressions such as b*t, where b is a speed or acceleration, and t is a time interval.
// Because our time values are expressed in microseconds, we actually have to evaluate b*t/1000000. To avoid both excessive
// round-off errors as well as to avoid overflows, we perform this calculation as (b*(t/1000))/1000, with the following macro:
#define timest(b, t) ((b * (t/US_PER_MS))/MS_PER_SECOND)
// We use the same trick to evaluate formulas of the form 1/2*b*t^2
#define timest2(b, t) ((timest(timest(b, t),t))/2)

// Evaluate the reference speed and velocity at the (shifted) time
void get_reference(ustime_t time_ref, pbio_motor_trajectory_t *traject, count_t *count_ref, rate_t *rate_ref){

    // For RUN and RUN_STALLED, the end time is infinite, meaning that the reference signals do not have a deceleration phase
    bool infinite = (traject->action == RUN) || (traject->action == RUN_STALLED);

    if (time_ref - traject->time_in < 0) {
        // If we are here, then we are still in the acceleration phase. Includes conversion from microseconds to seconds, in two steps to avoid overflows and round off errors
        *rate_ref = traject->rate_start   + timest(traject->accl_start, time_ref-traject->time_start);
        *count_ref = traject->count_start + timest(traject->rate_start, time_ref-traject->time_start) + timest2(traject->accl_start, time_ref-traject->time_start);
    }
    else if (infinite || time_ref - traject->time_out <= 0) {
        // If we are here, then we are in the constant speed phase
        *rate_ref = traject->rate_target;
        *count_ref = traject->count_in + timest(traject->rate_target, time_ref-traject->time_out);
    }
    else if (time_ref - traject->time_end <= 0) {
        // If we are here, then we are in the deceleration phase
        *rate_ref = traject->rate_target + timest(traject->accl_end,    time_ref-traject->time_out);
        *count_ref = traject->count_out  + timest(traject->rate_target, time_ref-traject->time_out) + timest2(traject->accl_end, time_ref-traject->time_out);       
    }
    else {
        // If we are here, we are in the zero speed phase (relevant when holding position)
        *rate_ref = 0;
        *count_ref = traject->count_end;  
    } 
}


/**
 * Status of the time at which we evaluate the count and rate reference signals
 */
typedef enum {
    TIME_NOT_INITIALIZED,
    TIME_PAUSED,
    TIME_RUNNING
} time_run_status_t;

time_run_status_t time_run_status[] = {
    [PORT_TO_IDX(PBDRV_CONFIG_FIRST_MOTOR_PORT) ... PORT_TO_IDX(PBDRV_CONFIG_LAST_MOTOR_PORT)]
        TIME_NOT_INITIALIZED
};

// Persistent PID related variables for each motor
count_t count_err_integral[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];
ustime_t maneuver_started[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];
ustime_t time_paused[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];
ustime_t time_stopped[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];

void control_update(pbio_port_t port){
    // Port index
    uint8_t idx = PORT_TO_IDX(port);

    // Trajectory and setting shortcuts for this motor
    pbio_motor_trajectory_t *traject = &trajectories[idx];
    pbio_encmotor_settings_t *settings = &encmotor_settings[idx];

    // Return immediately if the action is idle; then there is nothing we need to do
    if (traject->action == IDLE) {
        return;
    }

    // The very first time this is called, we initialize a fictitious previous maneuver
    if (time_run_status[idx] == TIME_NOT_INITIALIZED) {
        maneuver_started[idx] = pbdrv_time_get_usec() - US_PER_SECOND;
    }

    // Check if the trajectory starting time equals the current maneuver start time
    if (traject->time_start != maneuver_started[idx]) {
        // If not, then we are starting a new maneuver, and we update its starting time
        maneuver_started[idx] = traject->time_start;

        // For this new maneuver, we reset PID variables and related persistent control settings
        count_err_integral[idx] = 0;
        time_paused[idx] = 0;
        time_stopped[idx] = 0;
        time_run_status[idx] = TIME_RUNNING;
    }    

    // Declare current time, positions, rates, and their reference value and error
    ustime_t time_now, time_ref;
    count_t count_now, count_ref, count_err;
    rate_t rate_now, rate_ref, rate_err; 
    int16_t duty;          

    // Read current state of this motor: current time, speed, and position
    time_now = pbdrv_time_get_usec();
    pbio_encmotor_get_encoder_count(port, &count_now);
    pbio_encmotor_get_encoder_rate(port, &rate_now);   

    // Calculate control signal for current state for position based commands
    if (traject->action == RUN_TARGET || traject->action == RUN_ANGLE){
        // Get the time at which we want to evaluate the reference position/velocities
        // In nominal operation, take the current time, minus the amount of time we have stalled
        if (time_run_status[idx] == TIME_RUNNING) {
            time_ref = time_now - time_paused[idx];
        }
        else {
        // When the motor stalls, we keep the time constant. This way, the position reference does
        // not continue to grow unboundedly, thus preventing a form of wind-up
            time_ref = time_stopped[idx] - time_paused[idx];
        }
        // Get reference signals
        get_reference(time_ref, traject, &count_ref, &rate_ref);
        // Position and speed error
        count_err = count_ref - count_now;
        rate_err = rate_ref - rate_now;

        // TODO: translate anti-windup implementation in the position sense

        // TODO: translate anti-windup implementation in the integrator sense

        // TODO: translate stalled detection

        // Calculate duty signal
        duty = settings->pid_kp*count_err + settings->pid_ki*count_err_integral[idx] + settings->pid_kd*rate_err;

        // TODO: translate integrate position error

        // Check if we are at the target and standing still.
        if (time_ref >= traject->time_end && count_ref - settings->tolerance <= count_now && count_now <= count_ref + settings->tolerance && rate_now == 0) {
        // If so, we have reached our goal. We can keep running this loop in order to hold this position. 
        // But if brake was specified instead, we trigger that. Also clear the running flag to stop waiting for completion.      
            if (traject->after_stop == PBIO_MOTOR_STOP_COAST){
                pbio_dcmotor_coast(port);
                traject->action = IDLE;
            }
            else if (traject->after_stop == PBIO_MOTOR_STOP_BRAKE){
                pbio_dcmotor_brake(port);
                traject->action = IDLE;
            }
            else if (traject->after_stop == PBIO_MOTOR_STOP_HOLD) {
                // Holding just means that we continue the position control loop without changes
                pbio_dcmotor_set_duty_cycle_int(port, duty);
            }
            atomic_flag_clear(&motor_busy[PORT_TO_IDX(port)]);
        }
        // If we are not standing still at the target yet, actuate with the calculated signal
        else {
            pbio_dcmotor_set_duty_cycle_int(port, duty);
        }      
    }
    // Calculate control signal for current state for time based commands
    else if (traject->action == RUN || traject->action == RUN_TIME || traject->action == RUN_STALLED || traject->action == STOP){
        // TODO
        duty = 0;
    }    
}



void motor_control_update(){
    // Do the update for each motor
    for (pbio_port_t port = PBDRV_CONFIG_FIRST_MOTOR_PORT; port <= PBDRV_CONFIG_LAST_MOTOR_PORT; port++){
        // If we have read access for this motor, do the update
        if (!atomic_flag_test_and_set(&claimed_trajectory[PORT_TO_IDX(port)])) { // Remove once we remove multithreading
            // Do the control update for this motor
            control_update(port);
            // Release claim on control task
            atomic_flag_clear(&claimed_trajectory[PORT_TO_IDX(port)]); // Remove once we remove multithreading
        }
    }
}
