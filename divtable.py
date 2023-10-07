#! /usr/bin/env python3

HEIGHT_OFFSET = 22
HEIGHT_DIV_SHIFT = 2
HEIGHT_SCALE = 100.0
HORIZON = 45

Z_INITIAL = 1.0
Z_STEP_INITIAL = 0.5
Z_STEP_INCR = 0.4
Z_MAX = 220.0
Z_COUNT_MAX = 32

z_values = []
div_z_values = []


z = Z_INITIAL
z_step = Z_STEP_INITIAL
for i in range(Z_COUNT_MAX):
    if z >= Z_MAX:
        break
    z_values.append(z)
    for h in range(256 >> (HEIGHT_DIV_SHIFT-1)):
        height_plus_offset_div_z = (-256 + HEIGHT_OFFSET + (h << HEIGHT_DIV_SHIFT)) / z
        div_z_values.append( height_plus_offset_div_z * HEIGHT_SCALE + HORIZON)
    z += z_step
    z_step += Z_STEP_INCR

#print(z_values)
#print(list(enumerate(div_z_values[:len(div_z_values)//len(z_values)])))

z_values_fix32 = [int(x*1024.0) for x in z_values]
div_z_values_int = [int(x) for x in div_z_values]

def int32_clamp(i:int):
    if i < -2147483648:
        return -2147483648
    elif i > 2147483647:
        return 2147483647
    else:
        return i

def int16_clamp(i:int):
    if i < -32768:
        return -32768
    elif i > 32767:
        return 32767
    else:
        return i

with open("res/z_values.bin", "wb") as f_z:
    for z_f32 in z_values_fix32:
        z_f32:int
        f_z.write(int32_clamp(z_f32).to_bytes(4, 'big', signed=True))

with open("res/div_z_values.bin", "wb") as f_div_z:
    for div_z_i16 in div_z_values_int:
        div_z_i16:int
        f_div_z.write(int16_clamp(div_z_i16).to_bytes(2, 'big', signed=True))
