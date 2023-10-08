#include "genesis.h"
#include "resources.h"

static void handleInput();
static void joyEvent(u16 joy, u16 changed, u16 state);

static void heightAdjust();
static void drawView();

#define HEIGHT_OFFSET 22
#define HEIGHT_DIV_SHIFT 1
#define HEIGHT_SCALE 100

fix32 px = FIX32(512);
fix32 py = FIX32(512);
u16 phi = 0;
u8 phi_step = 0;
u16 height = HEIGHT_OFFSET;
fix32 horizon = FIX32(50);
static const u16 screen_width = BMP_WIDTH;
static const u16 screen_height = BMP_HEIGHT;
static const fix32 screen_width_inv = FIX32(1.0 / (BMP_WIDTH));

#define Z_INITIAL FIX32(1.0)
#define Z_STEP_INITIAL FIX32(0.5)
#define Z_STEP_INCR FIX32(0.4)
#define Z_MAX FIX32(220.0)
//#define Z_COUNT_MAX 32
const fix32 *z_values = ((const fix32 *) z_values_bin);
const s16 *div_z_values = ((const s16 *) div_z_values_bin);
u8 z_count;

u16 color_lut[512];
void precalc_dither() {

    for(int i = 0; i <256; i++) {
        u8 dcolor = (i >> 4) + ((i & 0xf) << 4);
        u8 color = i;
        color_lut[(i<<1)] = (color<<8)|color;
        color_lut[(i<<1)+1] = (dcolor<<8)|dcolor;
    }
}


int main(bool hard)
{
    precalc_dither();
    char col;


    z_count = sizeof(z_values_bin)/sizeof(fix32);
    heightAdjust();

    JOY_setEventHandler(joyEvent);

    VDP_setScreenWidth256();
    VDP_setHInterrupt(0);
    VDP_setHilightShadow(0);

    // reduce DMA buffer size to avoid running out of memory (we don't need it)
    DMA_setBufferSize(2048);

    // init Bitmap engine (require a ton shit of memory)
    BMP_init(TRUE, BG_A, PAL1, TRUE);

    PAL_setColor(0, bmp_color.palette->data[0]);
    PAL_setPalette(PAL1, bmp_color.palette->data, DMA);

    //XGM_startPlay(bgm);

    /* Do main job here */
    while(1)
    {
        char str[16];

        handleInput();

        // ensure previous flip buffer request has been started
        BMP_waitWhileFlipRequestPending();

        // draw everything
        BMP_showFPS(1);
        BMP_clear();
        drawView();

        // swap buffer
        BMP_flip(1);
    }
}

static void heightAdjust()
{
    s16 idx_x = fix32ToInt(px);
    while(idx_x < 0)
        idx_x += 1024;
    idx_x %= 1024;
    s16 idx_y = fix32ToInt(py);
    while(idx_y < 0)
        idx_y += 1024;
    idx_y %= 1024;
    s16 heightmap = depth_bin[1024*idx_y + idx_x];
    height = heightmap + HEIGHT_OFFSET;
}

static void handleInput()
{
    u16 value;

    // need to call it manually as we don't use SYS_doVBlankProcess() here
    JOY_update();

    value = JOY_readJoypad(JOY_1);

    if (value & BUTTON_A)
    {
    }
    else if (value & BUTTON_B)
    {
    }
    else
    {
        if (value & BUTTON_UP)
        {
            fix32 sinphi = sinFix32(phi);
            fix32 cosphi = cosFix32(phi);
            px += fix32Mul(FIX32(10), cosphi);
            py -= fix32Mul(FIX32(10), sinphi);
            heightAdjust();
        }
        if (value & BUTTON_DOWN)
        {
            fix32 sinphi = sinFix32(phi);
            fix32 cosphi = cosFix32(phi);
            px -= fix32Mul(FIX32(10), cosphi);
            py += fix32Mul(FIX32(10), sinphi);
            heightAdjust();
        }
        if (value & BUTTON_LEFT)
        {
            phi = (phi+16);
            phi &= 1023;
            if(phi_step == 63) { phi_step = 0; } else { phi_step++; }
        }
        if (value & BUTTON_RIGHT)
        {
            phi = (phi+1024-16);
            phi &= 1023;
            if(phi_step == 0) { phi_step = 63; } else { phi_step--; }
        }
    }
}

// 


static void joyEvent(u16 joy, u16 changed, u16 state)
{
    // START button state changed
    if (changed & BUTTON_START)
    {
        // START button pressed ?
        if (state & BUTTON_START)
        {
            // TODO reset view
        }
    }

    if (changed & state & BUTTON_A)
    {

    }
    if (changed & state & BUTTON_B)
    {

    }

    // C button state changed
    if (changed & BUTTON_C)
    {
        // C button pressed ?
        if (state & BUTTON_C)
        {

        }
    }
}


static void drawView()
{

    f32* step_values_f32 = (f32*)step_values;

    // there are 32 steps but also 4 values in each step
    // so we need to skip by 128! 
    // there are 32 z steps for each phi (angle), and each of those have 4 values, so scale by 128 here
    // which is a shift of 7
    f32* phi_step_values = &(step_values_f32[phi_step<<7]); 

    s16 ybuffer[screen_width];
    for(u16 i = 0; i < screen_width; i++)
        ybuffer[i] = screen_height;


    // Draw from front to the back (low z coordinate to high z coordinate)
    u8 zidx = 0;
    while (zidx < z_count)
    {
        const fix32 z = z_values[zidx];

        f32 pleft_x = px;
        f32 pleft_y = py;
        pleft_x += *phi_step_values++;
        pleft_y += *phi_step_values++;
        f32 dx = *phi_step_values++;
        f32 dy = *phi_step_values++;


        s16* div_z_ptr = &div_z_values[(zidx * (512>>HEIGHT_DIV_SHIFT))];

        //Raster line and draw a vertical line for each segment
        for (u16 i = 0; i < screen_width; i+=4)
        {
            s16 idx_x = pleft_x >> FIX32_FRAC_BITS;
            idx_x &= 1023;

            // pleft_y is already scaled by 10 (fix32s have 10 decimal bits)
            // and we need to scale by 1024 to access depth_bin (which is a shift of 10)
            // so just mask off the lower 10 bits here, keeping 10 bits above that
            s32 idx_y_scaled = (pleft_y & (1023<<10));

            s16 height_val = depth_bin[idx_y_scaled+idx_x];


            s16 rel_height = ((s16)height) - HEIGHT_OFFSET - height_val; // ranges from -255 to 255
            //u16 value_idx = ((rel_height + 256) >> HEIGHT_DIV_SHIFT);
            s16 height_on_screen = div_z_ptr[(rel_height + 256) >> HEIGHT_DIV_SHIFT];

            if(height_on_screen < 0) {
                height_on_screen = 0;
            }

            if (height_on_screen < ybuffer[i])
            {
                u8 color = bmp_color.image[(idx_y_scaled>>1) + (idx_x>>1)];
                u16 wcolor = color_lut[(color<<1)];
                u16 wdcolor = color_lut[(color<<1)+1];

                const u16 start_y = height_on_screen;

                u32 dy = ybuffer[i] - start_y;
                u8 * coladdr = BMP_getWritePointer(i, start_y);
                u16* wcol_ptr = coladdr;

                if(dy&1) {
                    *wcol_ptr = wcolor;
                    SWAP_u16(wcolor, wdcolor);
                    wcol_ptr += (BMP_PITCH>>1);
                }
                dy>>=1;


                u32 inc = 256;
                u16 jump_rows = 80-dy;
                u32 bytes_to_skip = (jump_rows * 8);


                // use a fully unrolled jump table here
                // each iteration writes 2 words, which is 8 pixels
                // 28 cycles per 8 pixels

                // the framebuffer is row major, so each next row is 128 bytes past the next
                // which means that if we have to do 80 iterations of this, we have to skip past a signed 32-bit offset
                // which means we cannot just have hard coded offsets here

                // a column-major framebuffer would allow faster column filling (~1.25 cycles per pixel, 10 cycles per 8 pixels!)
                // and also DMA blitting to VRAM
                __asm volatile(
                    "jmp code_table_%=(%%pc, %4.l)\t\n\
                    code_table_%=:\t\n\
                    .rept 80\t\n\
                    move.w %1, (%0)\t\n\
                    move.w %2, 128(%0)\t\n\
                    add.l %3, %0\t\n\
                    .endr\t\n\
                    "
                    : "+&a" (wcol_ptr)
                    : "d" (wcolor), "d" (wdcolor), "d" (inc), "d" (bytes_to_skip)
                );
               
                ybuffer[i] = height_on_screen;
            }
            pleft_x += dx;
            pleft_y += dy;
        }

        zidx ++;
    }
}
