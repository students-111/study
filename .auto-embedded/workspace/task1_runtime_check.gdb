set pagination off
set confirm off
target extended-remote localhost:3333
monitor reset run
monitor sleep 1000
monitor halt
info registers pc sp xpsr
p g_app_drive_mode_current
p g_dal_gray_sample.sequence
p g_dal_gray_sample.raw_mask
p g_dal_gray_sample.active_count
p g_dal_gray_sample.line_detected
p g_dal_gray_sample.position
p g_dal_key_sample[0].sequence
p g_dal_key_sample[0].pressed
p g_dal_key_sample[0].pressed_edge
p g_dal_mpu6050_sample.sequence
p g_dal_mpu6050_sample.yaw_calibrated
bt
detach
quit
