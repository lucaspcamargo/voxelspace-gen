import math

z = 1.0
dz = 0.5
ddz = 0.4
z_max = 220.0
z_count_max = 32

def fix32(v):
    return int(v*1024)
    
def int32_clamp(i:int):
    if i < -2147483648:
        return -2147483648
    elif i > 2147483647:
        return 2147483647
    else:
        return i

with open("res/step_values.bin", "wb") as step_table:
    for phi_step in range(64): #64):
        #print("phi step: ", phi_step)
        rad_1024 = (phi_step*16) + 256 + 512 % 1024;
        #print("rads out of 1024", rad_1024)
        
        rads = (rad_1024/1024)*2*math.pi
        #print("rads: ", rads)
        sin_phi = math.sin(rads)
        cos_phi = math.cos(rads)
        
        
        z = 1.0 
        dz = 0.5
        ddz = 0.4
        for i in range(z_count_max):
            if z >= z_max:
                break
        
            sin_phi_z = sin_phi * z
            cos_phi_z = cos_phi * z
            
            pleft_x = (-cos_phi_z) - sin_phi_z
            pleft_y = sin_phi_z - cos_phi_z
            pright_x = cos_phi_z - sin_phi_z
            pright_y = (-sin_phi_z) - cos_phi_z
            dx = (pright_x-pleft_x)/64
            dy = (pright_y-pleft_y)/64
            
            for val in [pleft_x, pleft_y, dx, dy]:
                step_table.write(int32_clamp(fix32(val)).to_bytes(4, 'big', signed=True))

            #print("{}, {}, {}, {},".format(fix32(pleft_x), fix32(pleft_y), fix32(dx), fix32(dy)))   
            z += dz
            dz += ddz
