/*
  gimbal attitude calculator

  This code implements an attitude calculator for a gimbal that has 3
  axis magnetic encoders using a yaw, roll, pitch (312) joint
  order. It takes an attitude estimate from an ArduPilot flight
  controller and combines it with the magnetic encoders to give an
  overall attitude estimate for the gimbal as euler angles in earth
  frame

  Andrew Tridgell, October 2023
 */

/*
  a representation of the magnetic encoders of the gimbal. These
  encoders are in 312 joint order.

  !!NOTE!!
      A positive yaw angle is clockwise when viewed from above
 */
struct Encoders {
    float roll_rad, pitch_rad, yaw_rad;
};

/*
  an euler 321 attitude
  !!NOTE!!
      A positive yaw angle is clockwise when viewed from above
 */
struct Attitude {
    float roll_rad, pitch_rad, yaw_rad;
};

/*
  calculate earth frame euler attitude for the gimbal based on
  magnetic encoders and an attitude from a flight controller
 */
void calculate_attitude(const struct Encoders *encoders, const struct Attitude *flight_control_attitude, struct Attitude *att_out);
