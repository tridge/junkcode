#include "gimbal_attitude.h"
#include <math.h>

/*
  implementation of gimbal attitude estimator from flight controller
  euler angles and magnetic encoders for a 3,1,2 joint angle gimbal

  Andrew Tridgell, October 2023
 */


/*
  attitude representation using a 3x3 rotation matrix. This follows
  the conventions of the ArduPilot AP_Math library
 */
struct RotationMatrix {
    float mat[3][3];
};

/*
  convert an Euler 321 attitude to a rotation matrix form
 */
static void rotmat_from_euler321(struct RotationMatrix *r, const struct Attitude *att)
{
    const float cp = cosf(att->pitch_rad);
    const float sp = sinf(att->pitch_rad);
    const float sr = sinf(att->roll_rad);
    const float cr = cosf(att->roll_rad);
    const float sy = sinf(att->yaw_rad);
    const float cy = cosf(att->yaw_rad);

    r->mat[0][0] = cp * cy;
    r->mat[0][1] = (sr * sp * cy) - (cr * sy);
    r->mat[0][2] = (cr * sp * cy) + (sr * sy);
    r->mat[1][0] = cp * sy;
    r->mat[1][1] = (sr * sp * sy) + (cr * cy);
    r->mat[1][2] = (cr * sp * sy) - (sr * cy);
    r->mat[2][0] = -sp;
    r->mat[2][1] = sr * cp;
    r->mat[2][2] = cr * cp;
}

/*
  convert an Euler 312 attitude from magnetic encoders to a rotation matrix form
 */
static void rotmat_from_euler312(struct RotationMatrix *r, const struct Encoders *enc)
{
    const float c3 = cosf(enc->pitch_rad);
    const float s3 = sinf(enc->pitch_rad);
    const float s2 = sinf(enc->roll_rad);
    const float c2 = cosf(enc->roll_rad);
    const float s1 = sinf(enc->yaw_rad);
    const float c1 = cosf(enc->yaw_rad);

    r->mat[0][0] = c1 * c3 - s1 * s2 * s3;
    r->mat[1][1] = c1 * c2;
    r->mat[2][2] = c3 * c2;
    r->mat[0][1] = -c2*s1;
    r->mat[0][2] = s3*c1 + c3*s2*s1;
    r->mat[1][0] = c3*s1 + s3*s2*c1;
    r->mat[1][2] = s1*s3 - s2*c1*c3;
    r->mat[2][0] = -s3*c2;
    r->mat[2][1] = s2;
}

/*
  convert a rotation matrix to a set of 321 euler angles
 */
static void rotmat_to_euler321(const struct RotationMatrix *m, struct Attitude *e)
{
    if (m->mat[2][0] >= 1.0) {
        e->pitch_rad = M_PI;
    } else if (m->mat[2][0] <= -1.0) {
        e->pitch_rad = -M_PI;
    } else {
        e->pitch_rad = -asinf(m->mat[2][0]);
    }
    e->roll_rad = atan2f(m->mat[2][1], m->mat[2][2]);
    e->yaw_rad  = atan2f(m->mat[1][0], m->mat[0][0]);
}


/*
  multiply rotation matrix m1 by rotation matrix m2 to give a new rotation matrix mout
 */
static void rotmat_multiply(const struct RotationMatrix *m1, const struct RotationMatrix *m2, struct RotationMatrix *mout)
{
    mout->mat[0][0] = m1->mat[0][0] * m2->mat[0][0] + m1->mat[0][1] * m2->mat[1][0] + m1->mat[0][2] * m2->mat[2][0];
    mout->mat[0][1] = m1->mat[0][0] * m2->mat[0][1] + m1->mat[0][1] * m2->mat[1][1] + m1->mat[0][2] * m2->mat[2][1];
    mout->mat[0][2] = m1->mat[0][0] * m2->mat[0][2] + m1->mat[0][1] * m2->mat[1][2] + m1->mat[0][2] * m2->mat[2][2];
    mout->mat[1][0] = m1->mat[1][0] * m2->mat[0][0] + m1->mat[1][1] * m2->mat[1][0] + m1->mat[1][2] * m2->mat[2][0];
    mout->mat[1][1] = m1->mat[1][0] * m2->mat[0][1] + m1->mat[1][1] * m2->mat[1][1] + m1->mat[1][2] * m2->mat[2][1];
    mout->mat[1][2] = m1->mat[1][0] * m2->mat[0][2] + m1->mat[1][1] * m2->mat[1][2] + m1->mat[1][2] * m2->mat[2][2];
    mout->mat[2][0] = m1->mat[2][0] * m2->mat[0][0] + m1->mat[2][1] * m2->mat[1][0] + m1->mat[2][2] * m2->mat[2][0];
    mout->mat[2][1] = m1->mat[2][0] * m2->mat[0][1] + m1->mat[2][1] * m2->mat[1][1] + m1->mat[2][2] * m2->mat[2][1];
    mout->mat[2][2] = m1->mat[2][0] * m2->mat[0][2] + m1->mat[2][1] * m2->mat[1][2] + m1->mat[2][2] * m2->mat[2][2];
}

/*
  calculate earth frame euler attitude for the gimbal based on
  magnetic encoders and an attitude from a flight controller

  equivalent python code using pymavlink rotmat API:
      m1.from_euler(radians(ATT.Roll),radians(ATT.Pitch),radians(ATT.Yaw))
      m1.rotate_312(radians(SIEN.R),radians(SIEN.P),radians(SIEN.Y))
      (r,p,y) = m1.to_euler()
 */
void calculate_attitude(const struct Encoders *encoders, const struct Attitude *flight_control_attitude, struct Attitude *att_out)
{
    struct RotationMatrix m1, m2, m3;
    rotmat_from_euler321(&m1, flight_control_attitude);
    rotmat_from_euler312(&m2, encoders);
    rotmat_multiply(&m1, &m2, &m3);
    rotmat_to_euler321(&m3, att_out);
}

#ifdef MAIN_TEST
/*
  a simple test suite to compare to python implementation
 */
#include <stdio.h>

/*
  maximum angular difference for test suite - NOTE! ignores yaw wrap
 */
static float ang_difference(const struct Attitude *a1, const struct Attitude *a2)
{
#define MAX(a,b) ((a)>(b)?(a):(b))
    float err = fabsf(a1->roll_rad - a2->roll_rad);
    err = MAX(err, fabsf(a1->pitch_rad - a2->pitch_rad));
    err = MAX(err, fabsf(a1->yaw_rad - a2->yaw_rad));
    return err;
}

/*
  convert degrees to radians
 */
static float radians(float deg)
{
    return deg * M_PI / 180.0;
}

/*
  convert radians to degrees
 */
static float degrees(float rad)
{
    return rad * 180.0 / M_PI;
}

/*
  test suite for gimbal attitude
 */
static struct Test {
    float fc_att_deg[3];
    float encoders_deg[3];
    float att_out[3];
} tests[] = {
    // test angles in degrees
    {{ 16, -29, -54 }, { 0,    0,   0 }, {  16,        -29,        -54 } },
    {{ 0, 0, 0 },      { 0,  -29, -54 }, {   0,        -29,        -54 } },
    {{ 0, 0, 73 },     { 0,  -29, -54 }, {   0,        -29,         19 } },
    {{ 0, 0, 0 },      { 16, -29, -54 }, {  18.151808, -27.776832, -62.686943 } },
    {{ 10, 15,  165 }, { 15, 24, -123 }, {  -3.243347,  23.949174,  39.364223 } },
    {{-37, 62, -175 }, { 84, 39,   12 }, { 142.790446,  53.089256, -69.374480 } },
};

int main(void)
{
    for (unsigned i=0; i<sizeof(tests)/sizeof(tests[0]); i++) {
        const struct Test *t = &tests[i];
        const struct Attitude fc_att = { radians(t->fc_att_deg[0]), radians(t->fc_att_deg[1]), radians(t->fc_att_deg[2]) } ;
        const struct Encoders enc = { radians(t->encoders_deg[0]), radians(t->encoders_deg[1]), radians(t->encoders_deg[2]) } ;
        const struct Attitude att_out = { radians(t->att_out[0]), radians(t->att_out[1]), radians(t->att_out[2]) };
        struct Attitude att;
        calculate_attitude(&enc, &fc_att, &att);
        const float err = ang_difference(&att, &att_out);
        if (err > 0.001) {
            printf("Test[%u] error %.3f (%.6f, %.6f, %.6f)\n", i, err, degrees(att.roll_rad), degrees(att.pitch_rad), degrees(att.yaw_rad));
            return 1;
        }
    }
    printf("All OK (C)\n");
    return 0;
}

#endif // MAIN_TEST
