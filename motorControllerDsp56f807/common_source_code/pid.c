#include "dsp56f807.h"
#include "options.h"
#include "pid.h"
#include "pwm_interface.h"
#include "currents_interface.h"
#include "trajectory.h"
#include "asc.h"
#include "can1.h"
#include "identification.h"
#include "check_range.h"

/* stable global data */
#ifndef VERSION
#	error "No valid version specified"
#endif
extern byte	_board_ID;	

#if VERSION == 0x0158 || VERSION == 0x0114 || VERSION == 0x0164
/* analog feedback */
#define INPOSITION_THRESHOLD 150
#else
/* digital encoder feedback */
#define INPOSITION_THRESHOLD 			60
#endif
#define INPOSITION_CALIB_THRESHOLD 		 1
bool _in_position[JN] = INIT_ARRAY (true);


// GENERAL VARIABLES
byte    _control_mode[JN] = INIT_ARRAY (MODE_IDLE); // control mode (e.g. position, velocity, etc.) 
Int16   _fault[JN] = INIT_ARRAY (0);				// amp fault memory 
Int16   _counter = 0;								// used to count cycles, it resets now and then to generate periodic events 
Int16   _counter_calib = 0;							// used in calibration to count the number of cycles
Int16   _pwm_calibration[JN] = INIT_ARRAY (0);		// pid value during calibration with hard stops 
Int16  _velocity_calibration[JN]=INIT_ARRAY (0);	// vel value during calibration with hard stops 
bool    _calibrated[JN] = INIT_ARRAY (false);
bool    _ended[];									// trajectory completed flag 
bool    _verbose = false;
bool    _pending_request = false;					// whether a request to another card is pending 
Int16   _timeout = 0;								// used to timeout requests 
Rec_Pid _received_pid[JN];
Int32   _bfc_PWMoutput [JN] = INIT_ARRAY (0);

// DEBUG VARIABLES
Int16 _debug_in0[JN] = INIT_ARRAY (0); 		 		// general purpouse debug
Int16 _debug_in1[JN] = INIT_ARRAY (0);				// general purpouse debug
Int16 _debug_in2[JN] = INIT_ARRAY (0); 		 		// general purpouse debug
Int16 _debug_in3[JN] = INIT_ARRAY (0);				// general purpouse debug
Int16 _debug_in4[JN] = INIT_ARRAY (0); 		 		// general purpouse debug
Int16 _debug_in5[JN] = INIT_ARRAY (0);				// general purpouse debug
Int16 _debug_in6[JN] = INIT_ARRAY (0); 		 		// general purpouse debug
Int16 _debug_in7[JN] = INIT_ARRAY (0);				// general purpouse debug
Int16 _debug_out1[JN] = INIT_ARRAY (0); 		 	// general purpouse debug
Int16 _debug_out2[JN] = INIT_ARRAY (0);				// general purpouse debug
Int16 _debug_out3[JN] = INIT_ARRAY (0); 		 	// general purpouse debug
Int16 _debug_out4[JN] = INIT_ARRAY (0);				// general purpouse debug
Int16 _sacc0[JN] = INIT_ARRAY (0);
Int16 _sacc1[JN] = INIT_ARRAY (0);
Int16 _sacc2[JN] = INIT_ARRAY (0);
byte  _t1c =0;

// BACK-EMF COMPENSATION
Int16 _backemf_gain[JN]  = INIT_ARRAY (0);
Int16 _backemf_shift[JN] = INIT_ARRAY (0);

// DECOUPLING PARAMETERS
float _param_a10_coeff = -1.6455F;
float _param_a11_coeff =  1.6455F;
float _param_a20_coeff = -1.6455F;
float _param_a21_coeff =  1.6455F;
float _param_a22_coeff =  1.6455F;

// POSITION VARIABLES
Int32 _abs_pos_calibration[JN] = INIT_ARRAY (0); // absolute position to be reached during calibration
Int32 _filt_abs_pos[JN] = INIT_ARRAY (0);		 // filtered absolute position sensor position
Int32 _position[JN] = INIT_ARRAY (0);			 // encoder position 
Int32 _motor_position[JN] = INIT_ARRAY (0);		 // hall effect encoder position 
Int32 _position_enc[JN] = INIT_ARRAY (0);		 // incremental encoder position 

Int32 _position_old[JN] = INIT_ARRAY (0);		 // do I need to add bits for taking into account multiple rotations 
Int32 _position_enc_old[JN] = INIT_ARRAY (0);	 // incremental encoder position 

Int32 _real_position[JN]= INIT_ARRAY (0);
Int32 _real_position_old[JN]= INIT_ARRAY (0);
Int32 _desired[JN] = INIT_ARRAY (0);		 
Int16 _desired_absolute[JN] = INIT_ARRAY (0);    // PD ref value for the calibration 
Int32 _set_point[JN] = INIT_ARRAY (0);  	     // set point for position [user specified] 

Int32 _min_position[JN] = INIT_ARRAY (-DEFAULT_MAX_POSITION);
Int32 _max_position[JN] = INIT_ARRAY (DEFAULT_MAX_POSITION);
Int32 _max_real_position[JN]=INIT_ARRAY (0);
Int16 _optical_ratio[JN]=INIT_ARRAY (0);         //  optical encoder ratio	

// SPEED VARIABLES
Int16 _speed[JN] = INIT_ARRAY (0);			 	 		// encoder speed 
Int16 _speed_old[JN] = INIT_ARRAY (0);					// encoder old speed 
Int16 _comm_speed[JN] = INIT_ARRAY (0);		     		// brushless commutation speed 
Int16 _comm_speed_old[JN] = INIT_ARRAY (0);				// previous brushless commutation speed 
Int32 _motor_speed[JN] = INIT_ARRAY (0);		     		// brushless motor speed 
Int32 _motor_speed_old[JN] = INIT_ARRAY (0);				// previous brushless motor speed 
Int16 _accu_desired_vel[JN] = INIT_ARRAY (0);			// accumultor for the fractional part of the desired vel 
Int16 _desired_vel[JN] = INIT_ARRAY (0);				// speed reference value, computed by the trajectory gen. 
Int16 _set_vel[JN] = INIT_ARRAY (DEFAULT_VELOCITY);		// set point for velocity [user specified] 
Int16 _max_vel[JN] = INIT_ARRAY (DEFAULT_MAX_VELOCITY);	// assume this limit is symmetric 
Int32 _vel_shift[JN] = INIT_ARRAY (4);
Int16 _vel_counter[JN] = INIT_ARRAY (0);
Int16 _vel_timeout[JN] = INIT_ARRAY (2000);             // timeout on velocity messages
byte  _jntVel_est_shift[JN] = INIT_ARRAY (5);			// shift of the speed estimator (joint)
byte  _motVel_est_shift[JN] = INIT_ARRAY (5);			// shift of the speed estimator (motor)

// ACCELERATION VARIABLES
Int16  _accel[JN] = INIT_ARRAY (0);			 			 // encoder acceleration 
Int16  _accel_old[JN] = INIT_ARRAY (0); 		       	 // previous acceleration
Int16  _set_acc[JN] = INIT_ARRAY (DEFAULT_ACCELERATION); // set point for acceleration [too low!] 
byte   _jntAcc_est_shift[JN] = INIT_ARRAY (5);			 // shift of the acceleration estimator (joint)
byte   _motAcc_est_shift[JN] = INIT_ARRAY (5);			 // shift of the acceleration estimator (motor)

// TORQUE VARIABLES
Int32 _desired_torque[JN] = INIT_ARRAY (0); 	// PID ref value, computed by the trajectory generator 


// POSITION PID VARIABLES
Int16  _error_position[JN] = INIT_ARRAY (0);	// actual feedback error 
Int16  _error_position_old[JN] = INIT_ARRAY (0);// error at t-1 
Int16  _absolute_error[JN] = INIT_ARRAY (0);	// actual feedback error from absolute sensors
Int16  _absolute_error_old[JN] = INIT_ARRAY (0);// error at t-1 
Int16  _pid[JN] = INIT_ARRAY (0);				// pid result 
Int16  _pid_limit[JN] = INIT_ARRAY (0);			// pid limit 
Int32  _pd[JN] = INIT_ARRAY (0);              	// pd portion of the pid
Int32  _pi[JN] = INIT_ARRAY (0);
Int32  _integral[JN] = INIT_ARRAY (0);	 		// store the sum of the integral component 
Int16  _integral_limit[JN] = INIT_ARRAY (0x7fff);

Int16  _kp[JN] = INIT_ARRAY (10);				// PID gains: proportional... 
Int16  _kd[JN] = INIT_ARRAY (40);				// ... derivative  ...
Int16  _ki[JN] = INIT_ARRAY (0);				// ... integral
Int16  _ko[JN] = INIT_ARRAY (0);				// offset 
Int16  _kr[JN] = INIT_ARRAY (3);				// scale factor (negative power of two) 
Int16  _kstp[JN] = INIT_ARRAY (0);              // stiction compensation: positive val
Int16  _kstn[JN] = INIT_ARRAY (0);              // stiction compensation: negative val


// TORQUE PID
Int16  _strain_val[JN] = INIT_ARRAY (0);
Int16  _error_torque[JN] = INIT_ARRAY (0);		// actual feedback error 
Int16  _error_old_torque[JN] = INIT_ARRAY (0);	// error at t-1  
Int16  _pid_limit_torque[JN] = INIT_ARRAY (0);	// pid limit 
Int16  _integral_limit_torque[JN] = INIT_ARRAY (0x7fff);
Int16  _kp_torque[JN] = INIT_ARRAY (0);			// PID gains: proportional... 
Int16  _kd_torque[JN] = INIT_ARRAY (0);			// ... derivative  ...
Int16  _ki_torque[JN] = INIT_ARRAY (0);			// ... integral
Int16  _ko_torque[JN] = INIT_ARRAY (0);			// offset 
Int16  _kr_torque[JN] = INIT_ARRAY (10);		// scale factor (negative power of two) 
Int16  _kstp_torque[JN] = INIT_ARRAY (0);       // stiction compensation: positive val
Int16  _kstn_torque[JN] = INIT_ARRAY (0);       // stiction compensation: negative val

// SPEED PID (iKart)
Int16  _error_speed[JN] = INIT_ARRAY (0);	    // actual feedback error 
Int16  _error_speed_old[JN] = INIT_ARRAY (0);   // error at t-1

// JOINT IMPEDANCE
Int16  _ks_imp[JN] = INIT_ARRAY (0);			// stiffness coefficient
Int16  _kd_imp[JN] = INIT_ARRAY (0);			// damping coefficient
Int16  _ko_imp[JN] = INIT_ARRAY (0);			// offset
Int16  _kr_imp[JN] = INIT_ARRAY (0);		    // scale factor (negative power of two) 

	
#if VERSION == 0x0156 || VERSION == 0x0166 || VERSION == 0x0116
// CURRENT PID
Int32 _desired_current[JN] = INIT_ARRAY (0);	// PID ref value, computed by the trajectory generator 
Int16  _error_current[JN] = INIT_ARRAY (0);		// current error
Int16  _error_current_old[JN] = INIT_ARRAY (0);	// current error at t-1 
Int16  _kp_current[JN] = INIT_ARRAY (40);		// PID gains: proportional ... 
Int16  _kd_current[JN] = INIT_ARRAY (30);		// ... derivative  ...
Int16  _ki_current[JN] = INIT_ARRAY (1);		// integral
Int16  _kr_current[JN] = INIT_ARRAY (6);		// scale factor (negative power of two) 
Int32  _integral_current[JN] = INIT_ARRAY (0);	// store the sum of the integral component 
Int16  _current_limit[JN] = INIT_ARRAY (250);	// pid current limit 
Int32  _pd_current[JN] = INIT_ARRAY (0);        // pd portion of the current pid
#endif


#if VERSION == 0x0153 || VERSION==0x0157 || VERSION==0x0150 || VERSION==0x0147 || VERSION==0x0140 || VERSION==0x0351 || VERSION==0x0250 || VERSION==0x0257
Int32  _cpl_pos_received[JN] = INIT_ARRAY (0);	// the position of the synchronized card 
Int32  _cpl_pos_prediction[JN] = INIT_ARRAY (0);// the actual adjustment (compensation) 
Int32  _cpl_pos_delta[JN] = INIT_ARRAY (0);		// velocity over the adjustment 
Int16  _cpl_pos_counter = 0;					// counter to check when last _cpl_pos_ was received  
Int16  _cpl_err[JN] = INIT_ARRAY (0);    		// the error of the syncronized card
Int16  _cpl_pid_received[JN] = INIT_ARRAY (0);  // the duty of the syncronized card
Int16  _cpl_pid_prediction[JN] = INIT_ARRAY (0);// the predicted adjustment
Int16  _cpl_pid_delta[JN] = INIT_ARRAY (0);		// the adjustment step
Int16   _cpl_pid_counter = 0;					// counter to check when last _cpl_pid_ was received  
#endif
#if  VERSION == 0x0113
Int32  _other_position[JN] = INIT_ARRAY (0);	// the position of the synchronized card
Int32  _adjustment[JN] = INIT_ARRAY (0);		// the actual adjustment (compensation)
Int32  _delta_adj[JN] = INIT_ARRAY (0);			// velocity over the adjustment
#endif

#if ((VERSION == 0x0121) || (VERSION == 0x0128) || (VERSION == 0x0130) || (VERSION == 0x0228) || (VERSION == 0x0230))
Int32  _adjustment[JN]=INIT_ARRAY (0);          // the sum of the three value coming from the MAIS board
#endif


#ifdef SMOOTH_PID_CTRL
float _pid_old[JN] = INIT_ARRAY (0);			// pid control at previous time step 
float _filt_pid[JN] = INIT_ARRAY (0);			// filtered pid control

//variables for the smooth_pid
Int16  ip[JN] = INIT_ARRAY (0);				// PID gains: proportional... 
Int16  id[JN] = INIT_ARRAY (0);				// ... derivative  ...
Int16  ii[JN] = INIT_ARRAY (0);
byte   is[JN] = INIT_ARRAY (0);				// scale factor (negative power of two) 	
Int16  fp[JN] = INIT_ARRAY (0);				// PID gains: proportional...
Int16  fd[JN] = INIT_ARRAY (0);				// ... derivative  ...
Int16  fi[JN] = INIT_ARRAY (0);
byte   fs[JN] = INIT_ARRAY (0);				// scale factor (negative power of two) 	
Int16 	dp[JN] = INIT_ARRAY (0);
Int16 	dd[JN] = INIT_ARRAY (0);
Int16 	di[JN] = INIT_ARRAY (0);
Int16    n[JN] = INIT_ARRAY (0);
Int16 time[JN] = INIT_ARRAY (0);
Int16  step_duration[JN] = INIT_ARRAY (0);
Int16   pp[4][8];
Int16   pd[4][8];
Int16   pi[4][8];
byte    ss[4][8];
bool  smoothing[JN]={false,false,false,false};
Int16 smoothing_step[JN]=INIT_ARRAY(0);  
Int16 smoothing_tip[JN]=INIT_ARRAY(0);
#endif

#if ((VERSION == 0x0120) || (VERSION == 0x0121) || (VERSION == 0x0128) || (VERSION == 0x0130) || (VERSION == 0x0228) || (VERSION == 0x0230))
// max allowed position for encoder while controlling with absolute position sensors
Int16 _max_position_enc[JN] = INIT_ARRAY (0);
Int16 _min_position_enc[JN] = INIT_ARRAY (0);
#endif


/*
 * compute PWM in different modalities
 */
Int32 compute_pwm(byte j)
{
	Int32 PWMoutput = 0;
	Int32 Ioutput = 0;
	byte  i=0;
	Int32 speed_damping =0;

		/*watchdog check for strain messages in torque control mode + input selection*/
		//the function turns off pwm of joint <jnt> if <strain_num> watchdog is triggered
		//the first number is joint, the second number is the watchdog identifier
#if   (VERSION == 0x150 || VERSION == 0x140 || VERSION == 0x250 )
	  	//arm
	  	read_force_data (0, WDT_JNT_STRAIN_12,0);
	  	read_force_data (1, WDT_JNT_STRAIN_12,1);
#elif (VERSION == 0x151 || VERSION == 0x251 ) 
	  	//legs
	  	if (_board_ID==5)     //left leg
	  	{
		//	read_force_data (0, WDT_6AX_STRAIN_13, 5);
			read_force_data (0, WDT_JNT_STRAIN_12, 2);
	  		read_force_data (1, WDT_JNT_STRAIN_12, 3);   		
	  	}
	  	else if (_board_ID==6) //left leg
	  	{
			read_force_data (0, WDT_JNT_STRAIN_12, 0); 
	  		read_force_data (1, WDT_JNT_STRAIN_12, 1); 	  		
	  	}
	  	else if (_board_ID==7) //left leg
	  	{
			read_force_data (0, WDT_JNT_STRAIN_12, 4); 
	  		read_force_data (1, WDT_JNT_STRAIN_12, 5); 	  		
	  	}
	  	else if (_board_ID==8) //right leg
	  	{
		//	read_force_data (0, WDT_6AX_STRAIN_14, 5);
			read_force_data (0, WDT_JNT_STRAIN_11, 2);
		  	read_force_data (1, WDT_JNT_STRAIN_11, 3);   		
	  	}  
	  	else if (_board_ID==9) //right leg
	  	{
			read_force_data (0, WDT_JNT_STRAIN_11, 0); 
	 	 	read_force_data (1, WDT_JNT_STRAIN_11, 1); 	  		
	  	}
	  	else if (_board_ID==10) //right leg
	  	{
			read_force_data (0, WDT_JNT_STRAIN_11, 4); 
		  	read_force_data (1, WDT_JNT_STRAIN_11, 5); 	  		
	  	}  	  	
	  	else
	  	{
		//you should never execute this code
		if (_control_mode[j] == MODE_TORQUE ||
			_control_mode[j] == MODE_IMPEDANCE_POS ||
			_control_mode[j] == MODE_IMPEDANCE_VEL)
			{	
				_control_mode[j] = MODE_IDLE;	
				_pad_enabled[j] = false;
				PWM_outputPadDisable(j);			
				can_printf("ERR:unknown fc2");
				_strain_val[j]=0;
			}
	  	}
#elif (VERSION == 0x152 || VERSION == 0x252) 
	  	//torso
	  	read_force_data (0, WDT_JNT_STRAIN_12,1); 
	  	read_force_data (1, WDT_JNT_STRAIN_12,2); 
#elif (VERSION == 0x154 || VERSION == 0x254)
	  	//torso
	  	read_force_data (0, WDT_JNT_STRAIN_12,0); 
	  	  
#elif VERSION == 0x157 || VERSION == 0x147 || VERSION == 0x257
      	//coupled joint of the arm
     // read_force_data (0, WDT_6AX_STRAIN_13,5);
	 	read_force_data (0, WDT_JNT_STRAIN_12,2);
	  	read_force_data (1, WDT_JNT_STRAIN_12,3); 
#elif   VERSION == 0x0119
		//arm
		read_force_data (0, WDT_JNT_STRAIN_12,4); //wrist pronosupination
		read_force_data (1, WDT_JNT_STRAIN_11,0); //wrist pitch disabled
		read_force_data (2, WDT_JNT_STRAIN_11,1); //wrist yaw   disabled
		read_force_data (3, -1               ,0); //fingers disabled
#elif   VERSION == 0x0219
    	//armV2
    	read_force_data (0, WDT_JNT_STRAIN_12,4); //wrist pronosupination
		read_force_data (1, WDT_JNT_STRAIN_11,0); //@@@TODO differential 1 for icubV2
		read_force_data (2, WDT_JNT_STRAIN_11,1); //@@@TODO differential wrist 2 for icubV2
		read_force_data (3, -1				 ,0); //fingers disabled
#elif	VERSION == 0x0351
		//iKart has a fake torque mode that just compensates for back-emf
		_strain_val[j]=0;
#else
	  	//other firmwares
		//you should never execute this code
		if (_control_mode[j] == MODE_TORQUE ||
			_control_mode[j] == MODE_IMPEDANCE_POS ||
			_control_mode[j] == MODE_IMPEDANCE_VEL)
			{	
				_control_mode[j] = MODE_IDLE;	
				_pad_enabled[j] = false;
				PWM_outputPadDisable(j);			
				can_printf("ERR:unknown fc2");
				_strain_val[j]=0;
			}
#endif	  
	
	switch (_control_mode[j]) 
	{ 
#if VERSION == 0x0170 || VERSION == 0x0171 || VERSION == 0x0172
	case MODE_POSITION: 
	case MODE_VELOCITY: 
	case MODE_CALIB_ABS_POS_SENS:
		compute_desired(j);
		PWMoutput = compute_pid2(j);
		PWMoutput = PWMoutput + _ko[j];
		_pd[j] = _pd[j] + _ko[j];
		break;		
	case MODE_TORQUE: 
	#ifndef IDENTIF 
		PWMoutput = compute_pid_torque(j, _strain[0][5]);
		PWMoutput = PWMoutput + _ko_torque[j];
		_pd[j] = _pd[j] + _ko_torque[j];
	#endif
	#ifdef IDENTIF 
		compute_identif_wt(j);	 
		if (_ki[0]==0) 
			openloop_identif= true; 
		else 
			openloop_identif= false; 
		if (openloop_identif==true)
		{
		 	//for open   loop transfer function
			_desired_torque[j] = PWMoutput = (Int16)sine_ampl[j]*(sin(wt[j]));      
		}
		else
		{
			//for closed loop transfer function
			_desired_torque[j]=(Int16)_ki[0]*(sin(wt[j]));
			PWMoutput = compute_pid_torque(j, 5); 
		//	PWMoutput -= (-_kr[0] * _speed[1])>>4;
			PWMoutput = PWMoutput + _ko_torque[j];
			_pd[j] = _pd[j] + _ko_torque[j];	
		}	
	#endif

	
		break; 
		
#elif VERSION == 0x0156
	case MODE_POSITION: 
	case MODE_VELOCITY: 
	case MODE_CALIB_ABS_POS_SENS: 
		compute_desired(j); 
		IOUT = compute_pid2(j); 	
		ENFORCE_CURRENT_LIMITS(j, IOUT); 
		PWMoutput = compute_current_pid(j); 		
		break;
	
#elif VERSION == 0x0351 
	//iKart control
 	case MODE_POSITION: 
 		PWMoutput = 0;
 		_pd[j] = 0;
 	break;
	case MODE_VELOCITY: 
		PWMoutput = compute_pid_speed(j);
		PWMoutput = PWMoutput + _ko[j];
		_pd[j] = _pd[j] + _ko[j];
	break;
	case MODE_TORQUE: 
		PWMoutput = compensate_bemf(j, _speed[j]);
		_pd[j] = 0;
	break;
	case MODE_OPENLOOP:
		PWMoutput = _ko[j];
		_pd[j] = 0;
	break;
	
#elif VERSION == 0x0215 
   	case MODE_POSITION:
	case MODE_VELOCITY:
	{
        if (_sacc0[j] == 0)
        {
     		compute_desired(j);
     		PWMoutput = compute_pid2(j);
     		PWMoutput = PWMoutput + _ko[j];
     		_pd[j] = _pd[j] + _ko[j];
        }
        else
        {
       	    static bool up[4] =INIT_ARRAY (1);
       	    static byte time_div[4] = INIT_ARRAY (0);
       	    static Int16 accum[4] = INIT_ARRAY (0);
       		Int32 _des_saved = 0;
       
       	    time_div[j]++;
       	    if (time_div[j] > 4) time_div[j] =0;
       		
       		if (j==2)
       		{
       			if (up[j]) accum[j] += 2;
       			else       accum[j] -= 2;
       			if      (accum[j] >  200) up[j] = false; //280 = 1 deg
       			else if (accum[j] < -200) up[j] = true;				
       		}
       		if (j==1)
       		{
       			if (time_div[j]==0)
       			{
       				if (up[j]) accum[j] += 1;
       				else       accum[j] -= 1;	
       			}
       			if      (accum[j] >  5) up[j] = false; //11 = 1 deg
       			else if (accum[j] < -5) up[j] = true;				
       		}
       		
       		compute_desired(j);
       		//microsaccades
       		_des_saved=_desired[j];	
       		//randomNext();
       		
       		if (j==1 || j == 2) {_desired[j] = _desired[j]+accum[j];}
            //can_printf("%d %f %f %f %f", j, _desired[0], _desired[1], _desired[2], _desired[3]); //jnt == 2 corrisponde al giunto 4
       		
       		PWMoutput = compute_pid2(j);
       		PWMoutput = PWMoutput + _ko[j];
       		_pd[j] = _pd[j] + _ko[j];
       		_desired[j]=_des_saved;
        }
	}
	break;
	case MODE_CALIB_ABS_POS_SENS:
	case MODE_CALIB_ABS_AND_INCREMENTAL:
		compute_desired(j);
		PWMoutput = compute_pid2(j);
		PWMoutput = PWMoutput + _ko[j];
		_pd[j] = _pd[j] + _ko[j];
	break;
	case MODE_OPENLOOP:
		PWMoutput = _ko[j];
		#ifdef IDENTIF 
		if (sine_ampl!=0)
		{
			compute_identif_wt(j);
			PWMoutput = (Int16)sine_ampl[j]*(sin(wt[j]));  	 
		}
		#endif
	break;	   

#else //all other firmware versions
	case MODE_POSITION:
	case MODE_VELOCITY:
	case MODE_CALIB_ABS_POS_SENS:
	case MODE_CALIB_ABS_AND_INCREMENTAL:
		compute_desired(j);
		PWMoutput = compute_pid2(j);
		PWMoutput = PWMoutput + _ko[j];
		_pd[j] = _pd[j] + _ko[j];
	break;
	case MODE_OPENLOOP:
		PWMoutput = _ko[j];
		#ifdef IDENTIF 
		if (sine_ampl!=0)
		{
			compute_identif_wt(j);
			PWMoutput = (Int16)sine_ampl[j]*(sin(wt[j]));  	 
		}
		#endif
	break;
	case MODE_TORQUE: 
		PWMoutput = compute_pid_torque(j, _strain_val[j]);
		
		// additional speed damping. It uses the same shift factor for bemf compensation
		speed_damping = ((Int32) (-_speed[j]) * ((Int32) _debug_in7[j]));
		speed_damping >>= (_backemf_shift[j]+_jntVel_est_shift[j]);
		
		PWMoutput = PWMoutput + _ko_torque[j] + speed_damping;
		_pd[j] = _pd[j] + _ko_torque[j] + speed_damping;

	
	break;
	case MODE_IMPEDANCE_POS:
	case MODE_IMPEDANCE_VEL: 
		compute_desired(j);
		compute_pid_impedance(j);
		PWMoutput = compute_pid_torque(j, _strain_val[j]);
		PWMoutput = PWMoutput + _ko_torque[j];
		_pd[j] = _pd[j] + _ko_torque[j];
	break;
#endif

#if (CURRENT_BOARD_TYPE == BOARD_TYPE_4DC) 
	case MODE_CALIB_HARD_STOPS:
		_desired[j]+=_velocity_calibration[j];
		PWMoutput = compute_pid2(j);
		if 	(PWMoutput > _pwm_calibration[j])
			PWMoutput = _pwm_calibration[j];
		if 	(PWMoutput < -_pwm_calibration[j])	
			PWMoutput = -_pwm_calibration[j];
	break;
#else
	case MODE_CALIB_HARD_STOPS:
		PWMoutput = _pwm_calibration[j];
		_counter_calib +=1;
	break;
#endif

	case MODE_HANDLE_HARD_STOPS:
	#ifdef DEBUG_CAN_MSG
		can_printf("MODE HANDLE HARD STOP");
	#endif
	    _pad_enabled[j] = false;
	    PWM_outputPadDisable(j);
		_control_mode[j] = MODE_IDLE;
		break;
		
	case MODE_IDLE:
	#ifdef IDENTIF 
		//parameters: j,  amp,  start_freq,  step_freq
		reset_identif(j,_debug_in0[j],1,_debug_in1[j]);
	#endif	
		PWMoutput=0;
	break; 
	}
	
	
#ifdef SMOOTH_PID_CTRL
	PWMoutput = compute_filtpid(j, PWMoutput);
#endif
	
	return PWMoutput;
}

/*
 * compute PID impedence (integral is implemented).
 */
Int32 compute_pid_impedance(byte j)
{
	Int32 ImpInputError=0;
	Int32 DampingPortion=0;
	byte k=0;
	static Int32 DmpAccu[JN]=INIT_ARRAY(0);  
	static byte headDmpPor[JN]=INIT_ARRAY(0);  
	static byte tailDmpPor[JN]=INIT_ARRAY(0); 
	static Int32 DmpPort[2][10]={{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0}};	
	
	_error_position_old[j] = _error_position[j];
	ImpInputError = L_sub(_position[j], _desired[j]);				
	if (ImpInputError > MAX_16)
	{
		_error_position[j] = MAX_16;
	}
	else if (ImpInputError < MIN_16) 
	{
		_error_position[j] = MIN_16;	
	}
	else
	{
		_error_position[j] = extract_l(ImpInputError);
	}		
	
	/* Proportional part (stiffness) */
	_desired_torque[j] = -(Int32) _ks_imp[j] * (Int32)(_error_position[j]);

	/* Additional offset (for gravity compensation) */
	_desired_torque[j] += (Int32)_ko_imp[j];
	
	/* Derivative part (damping) */	
	DampingPortion = -((Int32) (_error_position[j]-_error_position_old[j])) * ((Int32) _kd_imp[j]);
	//DampingPortion = -((Int32) (_speed[j])) * ((Int32) _kd_imp[j]) >> _jntVel_est_shift[j];
  
    _debug_out1[j]=_kd_imp[j];
    _debug_out2[j]=DampingPortion;
    
	#if (CURRENT_BOARD_TYPE  == BOARD_TYPE_BLL)
		//derivative filtering in BLL boards
		//this filter performs a mean on last 10 samples
		DmpAccu[j] -= DmpPort[j][tailDmpPor[j]];
		tailDmpPor[j]=headDmpPor[j]+1; if(tailDmpPor[j]>=10) tailDmpPor[j]=0;			
		DmpPort[j][headDmpPor[j]] = DampingPortion;
		DmpAccu[j] += DmpPort[j][headDmpPor[j]];

		headDmpPor[j]=headDmpPor[j]+1; if(headDmpPor[j]>=10) headDmpPor[j]=0;
		DampingPortion=DmpAccu[j]/10;
	#endif
	
	_desired_torque[j] += DampingPortion;	
	
	_debug_out3[j]=DampingPortion;
	_debug_out4[j]=_desired_torque[j];	
	
	return _desired_torque[j];
}

/*
 * compute PID torque
 */
Int32 compute_pid_torque(byte j, Int16 strain_val)
{
	Int32 ProportionalPortion, DerivativePortion, IntegralPortion;
	Int32 IntegralError;
	Int32 PIDoutput;
	Int32 InputError;
	float temp_err[JN] = INIT_ARRAY (0.0);

	byte i=0;
	byte k=0;
	static Int32 DerAccu[JN]=INIT_ARRAY(0);  
	static byte headDerPor[JN]=INIT_ARRAY(0);  
	static byte tailDerPor[JN]=INIT_ARRAY(0); 
	static Int32 DerPort[2][10]={{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0}};	
	
	
	/* the error @ previous cycle */
	
	_error_old_torque[j] = _error_torque[j];

	InputError = L_sub(	_desired_torque[j], (Int32)strain_val);
			
	if (InputError > MAX_16)
		_error_torque[j] = MAX_16;
	else
	if (InputError < MIN_16) 
		_error_torque[j] = MIN_16;
	else
	{
		_error_torque[j] = extract_l(InputError);
	}
			
	/* Proportional */
	ProportionalPortion = ((Int32) _error_torque[j]) * ((Int32)_kp_torque[j]);
	
	if (ProportionalPortion>=0)
	{
		ProportionalPortion = ProportionalPortion >> _kr_torque[j]; 
	}
	else
	{
		ProportionalPortion = -(-ProportionalPortion >> _kr_torque[j]);
	}
	
	/* Force Derivative */	
	DerivativePortion = ((Int32) (_error_torque[j]-_error_old_torque[j])) * ((Int32) _kd_torque[j]);

	if (DerivativePortion>=0)
	{
		DerivativePortion = DerivativePortion >> _kr_torque[j]; 
	}
	else
	{
		DerivativePortion = -(-DerivativePortion >> _kr_torque[j]);
	}
	 
    /* Derivative part filtering */
	//this filter performs a mean on last 10 samples
	DerAccu[j] -= DerPort[j][tailDerPor[j]];
	tailDerPor[j]=headDerPor[j]+1; if(tailDerPor[j]>=10) tailDerPor[j]=0;			
	DerPort[j][headDerPor[j]] = DerivativePortion;
	DerAccu[j] += DerPort[j][headDerPor[j]];
	headDerPor[j]=headDerPor[j]+1; if(headDerPor[j]>=10) headDerPor[j]=0;
	DerivativePortion=DerAccu[j]/10;
		
	/* Integral */
	IntegralError =  ( (Int32) _error_torque[j]) * ((Int32) _ki_torque[j]);
	
	_integral[j] = _integral[j] + IntegralError;
	IntegralPortion = _integral[j];
	
	if (IntegralPortion>=0)
	{
		IntegralPortion = (IntegralPortion >> _kr_torque[j]); // integral reduction 
	}
	else
	{
		IntegralPortion = -((-IntegralPortion) >> _kr_torque[j]); // integral reduction 
	} 

	/* Accumulator saturation */
	if (IntegralPortion >= _integral_limit_torque[j])
	{
		IntegralPortion = _integral_limit_torque[j];
		_integral[j] =  _integral_limit_torque[j];
	}		
	else if (IntegralPortion < - (_integral_limit_torque[j]))
	{
		IntegralPortion = - (_integral_limit_torque[j]);
		_integral[j] = (-_integral_limit_torque[j]);
	}
		
	_pd[j] = L_add(ProportionalPortion, DerivativePortion);
	_pi[j] = IntegralPortion;
	PIDoutput = L_add(_pd[j], IntegralPortion);
	
	return PIDoutput;
}

/*
 * compute PID control (integral is implemented).
 */
Int32 compute_pid2(byte j)
{
	Int32 ProportionalPortion, DerivativePortion, IntegralPortion;
	Int32 IntegralError;
	
	Int32 PIDoutput;
	Int32 InputError;
	byte i=0;
	byte k=0;

	static Int32 DerAccu[JN]=INIT_ARRAY(0);  
	static byte headDerPor[JN]=INIT_ARRAY(0);  
	static byte tailDerPor[JN]=INIT_ARRAY(0); 
	static Int32 DerPort[2][10]={{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0}};	
	
	/* the error @ previous cycle */
	
	_error_position_old[j] = _error_position[j];

	InputError = L_sub(_desired[j], _position[j]);
		
	if (InputError > MAX_16)
		_error_position[j] = MAX_16;
	else
	if (InputError < MIN_16) 
		_error_position[j] = MIN_16;
	else
	{
		_error_position[j] = extract_l(InputError);
	}
			
	/* Proportional */
	ProportionalPortion = ((Int32) _error_position[j]) * ((Int32)_kp[j]);
	
	if (ProportionalPortion>=0)
	{
		ProportionalPortion = ProportionalPortion >> _kr[j]; 
	}
	else
	{
		ProportionalPortion = -(-ProportionalPortion >> _kr[j]);
	}
	
#if VERSION==0x162
	
	/* Derivative */	
	if (j==0)
	{
		DerivativePortion = ((Int32) -(_speed[0]-_speed[1])) * ((Int32) _kd[j]);	
	}
	else 
	{
		DerivativePortion = ((Int32) -(_speed[0]+_speed[1])) * ((Int32) _kd[j]);	
	}

	if (DerivativePortion>=0)
	{
		DerivativePortion = DerivativePortion >> (_kr[j]);//+_jntVel_est_shift[j]); 
	}
	else
	{
		DerivativePortion = -(-DerivativePortion >> (_kr[j]));//+_jntVel_est_shift[j]));
	}

#else

	/* Derivative */	
	DerivativePortion = ((Int32) (_error_position[j]-_error_position_old[j])) * ((Int32) _kd[j]);

	if (DerivativePortion>=0)
	{
		DerivativePortion = DerivativePortion >> _kr[j]; 
	}
	else
	{
		DerivativePortion = -(-DerivativePortion >> _kr[j]);
	}
  
#endif		

#if (CURRENT_BOARD_TYPE  == BOARD_TYPE_BLL)
	//derivative filtering in BLL boards
	//this filter performs a mean on last 10 samples
	DerAccu[j] -= DerPort[j][tailDerPor[j]];
	tailDerPor[j]=headDerPor[j]+1; if(tailDerPor[j]>=10) tailDerPor[j]=0;			
	DerPort[j][headDerPor[j]] = DerivativePortion;
	DerAccu[j] += DerPort[j][headDerPor[j]];
	headDerPor[j]=headDerPor[j]+1; if(headDerPor[j]>=10) headDerPor[j]=0;
	DerivativePortion=DerAccu[j]/10;
#endif

	/* Integral */
	IntegralError =  ( (Int32) _error_position[j]) * ((Int32) _ki[j]);

#if VERSION == 0x0156 || VERSION == 0x0116 || VERSION == 0x0166

	_integral_current[j] = _integral_current[j] + IntegralError;
	IntegralPortion = _integral_current[j];

	_pd_current[j] = L_add(ProportionalPortion, DerivativePortion);
	PIDoutput = L_add(_pd_current[j], IntegralPortion);

#else 

	_integral[j] = _integral[j] + IntegralError;
	IntegralPortion = _integral[j];
	
	if (IntegralPortion>=0)
	{
		IntegralPortion = (IntegralPortion >> _kr[j]); // integral reduction 
	}
	else
	{
		IntegralPortion = -((-IntegralPortion) >> _kr[j]); // integral reduction 
	} 
	
	/* Accumulator saturation */
	if (IntegralPortion >= _integral_limit[j])
	{
		IntegralPortion = _integral_limit[j];
		_integral[j] =  _integral_limit[j];
	}		
	else if (IntegralPortion < - (_integral_limit[j]))
	{
		IntegralPortion = - (_integral_limit[j]);
		_integral[j] = (-_integral_limit[j]);
	}
		
	_pd[j] = L_add(ProportionalPortion, DerivativePortion);
	_pi[j] = IntegralPortion;
	PIDoutput = L_add(_pd[j], IntegralPortion);

#endif
					
	return PIDoutput;
}

/*
 * compute explicit speed PID control (iKart).
 */
Int32 compute_pid_speed(byte j)
{
	Int32 ProportionalPortion, DerivativePortion, IntegralPortion;
	Int32 IntegralError;
	
	Int32 PIDoutput;
	Int32 InputError;
	byte i=0;
	byte k=0;

	static Int32 DerAccu[JN]=INIT_ARRAY(0);  
	static byte headDerPor[JN]=INIT_ARRAY(0);  
	static byte tailDerPor[JN]=INIT_ARRAY(0); 
	static Int32 DerPort[2][10]={{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0}};	
	
	/* the error @ previous cycle */
	
	_error_speed_old[j] = _error_speed[j];

	InputError = L_sub(_set_vel[j], _speed[j]);
		
	if (InputError > MAX_16)
		_error_speed[j] = MAX_16;
	else
	if (InputError < MIN_16) 
		_error_speed[j] = MIN_16;
	else
	{
		_error_speed[j] = extract_l(InputError);
	}
			
	/* Proportional */
	ProportionalPortion = ((Int32) _error_speed[j]) * ((Int32)_kp[j]);
	
	if (ProportionalPortion>=0)
	{
		ProportionalPortion = ProportionalPortion >> (_kr[j]+_jntVel_est_shift[j]); 
	}
	else
	{
		ProportionalPortion = -(-ProportionalPortion >> (_kr[j]+_jntVel_est_shift[j]));
	}
	
	/* Derivative */	
	DerivativePortion = ((Int32) (_error_speed[j]-_error_speed_old[j])) * ((Int32) _kd[j]);

	if (DerivativePortion>=0)
	{
		DerivativePortion = DerivativePortion >> (_kr[j]+_jntVel_est_shift[j]); 
	}
	else
	{
		DerivativePortion = -(-DerivativePortion >> (_kr[j]+_jntVel_est_shift[j]));
	}
  
	//this filter performs a mean on last 10 samples
	DerAccu[j] -= DerPort[j][tailDerPor[j]];
	tailDerPor[j]=headDerPor[j]+1; if(tailDerPor[j]>=10) tailDerPor[j]=0;			
	DerPort[j][headDerPor[j]] = DerivativePortion;
	DerAccu[j] += DerPort[j][headDerPor[j]];
	headDerPor[j]=headDerPor[j]+1; if(headDerPor[j]>=10) headDerPor[j]=0;
	DerivativePortion=DerAccu[j]/10;
		
	/* Integral */
	IntegralError =  ( (Int32) _error_speed[j]) * ((Int32) _ki[j]);

	_integral[j] = _integral[j] + IntegralError;
	IntegralPortion = _integral[j];
	
	if (IntegralPortion>=0)
	{
		IntegralPortion = (IntegralPortion >> (_kr[j]+_jntVel_est_shift[j])); // integral reduction 
	}
	else
	{
		IntegralPortion = -((-IntegralPortion) >> (_kr[j]+_jntVel_est_shift[j])); // integral reduction 
	} 
	
	/* Accumulator saturation */
	if (IntegralPortion >= _integral_limit[j])
	{
		IntegralPortion = _integral_limit[j];
		_integral[j] =  _integral_limit[j];
	}		
	else if (IntegralPortion < - (_integral_limit[j]))
	{
		IntegralPortion = - (_integral_limit[j]);
		_integral[j] = (-_integral_limit[j]);
	}
		
	_pd[j] = L_add(ProportionalPortion, DerivativePortion);
	_pi[j] = IntegralPortion;
	PIDoutput = L_add(_pd[j], IntegralPortion);

					
	return PIDoutput;
}

/*
 * compute PID control (integral implemented).
 */
#if VERSION == 0x0156 || VERSION == 0x0166 || VERSION == 0x0116

Int32 compute_current_pid(byte j)
{
	Int32 ProportionalPortion, DerivativePortion, IntegralPortion;
	Int32 IntegralError;
	Int32 PIDoutput;
	Int32 InputError;
		
	/* the error @ previous cycle */
	_error_current_old[j] = _error_current[j];

	InputError = L_sub(_desired_current[j], get_current(j));
		
	if (InputError > MAX_16)
		_error_current[j] = MAX_16;
	else
	if (InputError < MIN_16) 
		_error_current[j] = MIN_16;
	else
	{
		_error_current[j] = extract_l(InputError);
	}		

	/* Proportional */
	ProportionalPortion = ((Int32) _error_current[j]) * ((Int32)_kp_current[j]);
	ProportionalPortion = ProportionalPortion >> _kr_current[j];
	/* Derivative */	
	DerivativePortion = ((Int32) (_error_current[j]-_error_current_old[j])) * ((Int32) _kd_current[j]);
	DerivativePortion = DerivativePortion >>  _kr_current[j];
	/* Integral */
	IntegralError = ( (Int32) _error_current[j]) * ((Int32) _ki_current[j]);
	IntegralError = IntegralError >> _kr_current[j];
	
	if (IntegralError > MAX_16)
		IntegralError = (Int32) MAX_16;
	if (IntegralError < MIN_16) 
		IntegralError = (Int32) MIN_16;
	
	_integral[j] = L_add(_integral[j], IntegralError);
	IntegralPortion = _integral[j];
		
	_pd[j] = L_add(ProportionalPortion, DerivativePortion);
	_pi[j] = IntegralPortion;
	PIDoutput = L_add(_pd[j], IntegralPortion);
			
	return PIDoutput;
}
#endif

/* 
 * Compute PD for calibrating with the absolute postion sensors
 */
Int32 compute_pid_abs(byte j)
{
	Int32 ProportionalPortion, DerivativePortion;
	Int32 PIDoutput;
	Int16 Kp = 1;
	Int16 Kd = 10;
		
	/* the error @ previous cycle */
	_absolute_error_old[j] = _absolute_error[j];
	/* the errore @ current cycle */
	_absolute_error[j] = _desired_absolute[j] - extract_h(_filt_abs_pos[j]);
	//_absolute_error[j] = 0x5a0 - extract_h(_filt_abs_pos[j]);	

	/* Proportional */
	ProportionalPortion = _absolute_error[j] * Kp;
	/* Derivative */	
	DerivativePortion = (_absolute_error[j]-_absolute_error_old[j]) * Kd;
	
	PIDoutput = (ProportionalPortion + DerivativePortion);
	//AS1_printDWordAsCharsDec (PIDoutput/70);		
	

	return (PIDoutput >> 1);
}

/* 
 * this function filters the current (AD value).
 */
#ifdef SMOOTH_PID_CTRL
 
Int32 compute_filtpid(byte jnt, Int32 PID)
{
	/*
	The filter is the following:
	_filt_current = a_1 * _filt_current_old 
				  + a_2 * (_current_old + _current).
	Parameters a_1 and a_2 are computed on the sample time
	(Ts = 1 ms) and the rising time (ts = 200ms).Specifically
	we have:
	a_1 = (2*tau - Ts) / (2*tau + Ts)
	a_2 = Ts / (2*tau + Ts)
	where tau = ts/2.3. Therefore:
	a_1 = 0.9773
	a_2 = 0.0114
	*/
	float pid;
	static float filt_pid_old[JN] = INIT_ARRAY(0); 
	
	pid = (float) PID;
	filt_pid_old[jnt] = _filt_pid[jnt];
	_filt_pid[jnt] = 0.9773 * filt_pid_old[jnt] + 0.0114 * (_pid_old[jnt] + pid);
	_pid_old[jnt] = pid;
	return (Int32) _filt_pid[jnt];
	//return (Int32) pid;
}

#endif

/*
 * a step in the trajectory generation for velocity control. 
 */
Int32 step_velocity (byte jj)
{
	Int32 u0;
	Int16 dv, da;
	Int16 _tmp_desired_vel;
	Int16 _tmp_diff_vel;
	
	/* dv is a signed 16 bit value, need to be checked for overflow */
	if (_set_vel[jj] < -_max_vel[jj])
		_set_vel[jj] = -_max_vel[jj];
	else
	if (_set_vel[jj] > _max_vel[jj])
		_set_vel[jj] = _max_vel[jj];
	
	dv = _set_vel[jj] - _desired_vel[jj];
	da = _set_acc[jj] * CONTROLLER_PERIOD;
	
	if (__abs(dv) < da)
	{
		_desired_vel[jj] = _set_vel[jj];
	}
	else
	if (dv > 0)
	{
		_desired_vel[jj] += da;
	}
	else
	{
		_desired_vel[jj] -= da;
	}
	
	//since the desired velocity is expressed in
	//[16*encoders_tics/ms] we divide the desired
	//velocity by 16 with a shift of 4 bits
	//(more in general: _desired_vel[jj] which has a default value of 4)
	_tmp_desired_vel = (__abs(_desired_vel[jj]) >> _vel_shift[jj]);
	if (_desired_vel[jj] > 0)
		u0 =   _tmp_desired_vel * CONTROLLER_PERIOD;
	else
		u0 = - _tmp_desired_vel * CONTROLLER_PERIOD;
	
	//the additional 4 bits (which have not been used
	//in computing the desired velocity) are accumulated
	_tmp_diff_vel = __abs(_desired_vel[jj]) - (_tmp_desired_vel << _vel_shift[jj]);
	if (_desired_vel[jj] > 0)
		_accu_desired_vel[jj] = _accu_desired_vel[jj] + _tmp_diff_vel;
	else
		_accu_desired_vel[jj] = _accu_desired_vel[jj] - _tmp_diff_vel;
	
	//if accumulated additional bits overflow (i.e.
	//the fifth...sixteenth bits are different from zero)
	//the overflown part is added to the output
	//finally the accumulator is updated, taking into
	//account that the overflown bit have been considered
	_tmp_desired_vel = (__abs(_accu_desired_vel[jj]) >> _vel_shift[jj]);
	if (_desired_vel[jj] > 0)
	{
		u0 = u0 + _tmp_desired_vel * CONTROLLER_PERIOD;
		_accu_desired_vel[jj] = _accu_desired_vel[jj] - (_tmp_desired_vel<<_vel_shift[jj]);
	}
		
	else
	{
		u0 = u0 - _tmp_desired_vel * CONTROLLER_PERIOD;
		_accu_desired_vel[jj] = _accu_desired_vel[jj] + (_tmp_desired_vel<<_vel_shift[jj]);
	}
	
	return u0;
}


/*
 * helper function to generate desired position.
 */
void compute_desired(byte i)
{		
 	Int32 previous_desired;
	
	if (_control_mode[i] != MODE_IDLE)
	{
		previous_desired = _desired[i];
		
		/* compute trajectory and control mode */
		switch (_control_mode[i])
		{
		case MODE_POSITION:
		case MODE_IMPEDANCE_POS:
		case MODE_CALIB_ABS_AND_INCREMENTAL:
			_desired[i] = step_trajectory (i);
			break;
			
		case MODE_CALIB_ABS_POS_SENS:
		
			_desired_absolute[i] = (Int16) step_trajectory (i);
			
			/* The following lines handle two possible situations:
				(1) the absolute position sensor increseas 
					when the encoder increases
				(2) the absolute position sensor increseas 
					when the encoder increases */
#if (CURRENT_BOARD_TYPE  == BOARD_TYPE_4DC)		
			if (VERSION == 0x0112 && i == 0)
				_desired[i] = _desired[i] - compute_pid_abs (i);
			else
				_desired[i] = _desired[i] + compute_pid_abs (i);
#endif	
			break;
							
							
							
		case MODE_VELOCITY:
		case MODE_IMPEDANCE_VEL:
			_desired[i] += step_trajectory_delta (i);
			_desired[i] += step_velocity (i);
			// checks if the velocity messages streaming
			// has been interrupted (i.e. last message
			// received  more than _vel_timeout ms ago)

			_vel_counter[i]++;
#if (VERSION != 0x0258)		  
		    if(_vel_counter[i] > _vel_timeout[i])
		    {	
		    	//disabling velocity control						
				if (_control_mode[i] == MODE_IMPEDANCE_VEL) _control_mode[i] = MODE_IMPEDANCE_POS;
				else _control_mode[i] = MODE_POSITION;	  	
						  	
				init_trajectory (i, _desired[i], _desired[i], 1);
				#ifdef DEBUG_CAN_MSG
					can_printf("No vel msgs in %d[ms]", _vel_counter[i]);
				#endif	
				
				//resetting the counter
				_vel_counter[i] = 0;	
		    }			  	
#endif
			break;
		}
#if (VERSION != 0x0258)		
		check_desired_within_limits(i, previous_desired);
#endif
	}
}

/***************************************************************** 
 * this function checks if the trajectory is terminated
 * and if trajectory is terminated sets the variable _in_position
 *****************************************************************/
bool check_in_position(byte jnt)
{
	if (_control_mode[jnt] == MODE_POSITION ||
	    _control_mode[jnt] == MODE_VELOCITY ||
	    _control_mode[jnt] == MODE_IMPEDANCE_POS ||
	    _control_mode[jnt] == MODE_IMPEDANCE_VEL)
	{
		//if (__abs(_position[jnt] - _set_point[jnt]) < INPOSITION_THRESHOLD && _ended[jnt])
		if (_ended[jnt])
			return true;
		else
			return false;
	}
	else
		return false;				
}

/***************************************************************** 
//this function reduces the PID values smoothly
//The function divides the Time in steps and in each step it reduces the value of scalefactor and the pid.
/****************************************************************/ 
#ifdef SMOOTH_PID_CTRL
void init_smooth_pid(byte jnt,Int16 finalp,Int16 finald,byte finals, Int16 Time)
{	
	Int16 * ppleft=0;
	Int16 * pdleft=0;
	Int16 * pileft=0;
	byte  * ssleft=0;
	Int16 * ppright=0;
	Int16 * pdright=0;
	Int16 * piright=0;
	byte  * ssright=0;
	Int16 t[JN]={0,0,0,0};
	Int16 j=0;
	Int16 yp;
	Int16 finali=0;
	ip[jnt]=_kp[jnt]<<8;	
	id[jnt]=_kd[jnt]<<8;
	ii[jnt]=_ki[jnt]<<8;
	is[jnt]=_kr[jnt];
	fp[jnt]=finalp<<8;	
	fd[jnt]=finald<<8;
	fi[jnt]=finali<<8;
	fs[jnt]=finals;
	time[jnt]=Time;
	dp[jnt]=-(-fp[jnt]>>fs[jnt]+ip[jnt]>>is[jnt])/(Int32)time[jnt];// %rate di discesa del p
	dd[jnt]=-(-fd[jnt]>>fs[jnt]+id[jnt]>>is[jnt])/(Int32)time[jnt];// %rate di discesa del p
	di[jnt]=-(-fi[jnt]>>fs[jnt]+ii[jnt]>>is[jnt])/(Int32)time[jnt];// %rate di discesa del p
	n[jnt]=(fs[jnt]-is[jnt]); // numero di step
	step_duration[jnt]=(time[jnt]/ (Int32)(n[jnt]+1));//(time[jnt]/(n[jnt]+1));
	 
	for (j=0;j<n[jnt]<<1+2;j++)
	{
		pp[jnt][j]=0;
		pd[jnt][j]=0;
		pi[jnt][j]=0;	
		ss[jnt][j]=0;
		ppleft[j]=0;
		pdleft[j]=0;
		pileft[j]=0;	
		ssleft[j]=0;			
		ppright[j]=0;
		pdright[j]=0;
		piright[j]=0;
		ssright[j]=0;		
	}
	
	pp[jnt][0]=ip[jnt]>>8;
	pd[jnt][0]=id[jnt]>>8;
	pi[jnt][0]=ii[jnt]>>8;
	pp[jnt][n[jnt]<<1+2]=fp[jnt]>>8;
	pd[jnt][n[jnt]<<1+2]=fd[jnt]>>8;
	pi[jnt][n[jnt]<<1+2]=fi[jnt]>>8;
	for (j=0;j<n[jnt];j++)
    {
	    t[j]=step_duration[jnt]*(j+1);
	    ssleft[j]=is[jnt]+(j);
	    ssright[j]=ssleft[j]+1;
	    yp=dp[jnt]*t[j]+ip[jnt]>>is[jnt];
	    ppleft[j]=yp<<ssleft[j];
	    ppright[j]=yp<<ssright[j];
	    pp[jnt][(j*2)+1]=ppleft[j]>>8;
	    pp[jnt][(j*2)+2]=ppright[j]>>8;
	    ss[jnt][(j*2)+1]=ssleft[j];
	    ss[jnt][(j*2)+1]=ssright[j];	
    }	
    smoothing_step[jnt]=n[jnt]+1;
    smoothing[jnt]=true;
}
void smooth_pid(byte jnt)
{
	if (smoothing[jnt]==true && smoothing_step[jnt]<=n[jnt]+1)
	{
		if (_kp[jnt]<=pp[jnt][smoothing_step[jnt]*2+1])
		{
			_kp[jnt]--;
		}
		_kr[jnt]=ss[jnt][smoothing_step[jnt]*2];
			}
	if (smoothing_tip[jnt]<step_duration[jnt])
	{
		smoothing_tip[jnt]++;           	
	} 
	else
	{
		smoothing_tip[jnt]=0;	
		smoothing_step[jnt]++;
	}	
}
#endif

/***************************************************************************/
/**
 * this function turns off pwm of joint <jnt> if <strain_num> watchdog is
 * triggered (returns false). Returns true otherwise (all ok).
 * the force value contained in the <strain_channel> is assigned to strain_val
 ***************************************************************************/
bool read_force_data (byte jnt, byte strain_num, byte strain_chan)
{
	if (_control_mode[jnt] == MODE_TORQUE ||
		_control_mode[jnt] == MODE_IMPEDANCE_POS ||
		_control_mode[jnt] == MODE_IMPEDANCE_VEL )
		{
			if (strain_num==-1)
			{
				_control_mode[jnt] = MODE_IDLE;	
				_pad_enabled[jnt] = false;

				#ifdef DEBUG_CAN_MSG					
					can_printf("WARN:force control not allowed jnt:%d",jnt);
				#endif
								
				PWM_outputPadDisable(jnt);	
				_strain_val[jnt]=0;
				return false;				
			}
			if (_strain_wtd[strain_num]==0)
			{
				_control_mode[jnt] = MODE_IDLE;	
				_pad_enabled[jnt] = false;
					
				#ifdef DEBUG_CAN_MSG
					can_printf("WARN:strain watchdog disabling pwm jnt:%d",jnt);				
				#endif	
				
				PWM_outputPadDisable(jnt);	
				_strain_val[jnt]=0;
				return false;
			}
			else
			{
				_strain_val[jnt]=_strain[strain_num][strain_chan];
				return true;	
			}	
		}
	_strain_val[jnt]=_strain[strain_num][strain_chan];
	return true;		
}

