#include "genesis.h"
#include "resources.h"

static void handleInput();
static void joyEvent(u16 joy, u16 changed, u16 state);

static void heightAdjust();
static void drawView();

#define HEIGHT_OFFSET 22
#define HEIGHT_DIV_SHIFT 2
#define HEIGHT_SCALE 100

fix32 px = FIX32(512);
fix32 py = FIX32(512);
u16 phi = 0;
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

int main(bool hard)
{
    char col;

    // init z div table
    /*  NOW PRECOMPUTED :)
    u8 i;
    fix32 z = Z_INITIAL;
    fix32 z_step = Z_STEP_INITIAL;
    for(i = 0; (i < Z_COUNT_MAX) && (z < Z_MAX); i++)
    {
        z_values[i] = z;
        z += z_step;
        z_step += Z_STEP_INCR;
        for(u16 h = 0; h < (256 >> (HEIGHT_DIV_SHIFT-1)); h++)
        {
            // min height is HEIGHT_OFFSET
            // max height is 255 + HEIGHT_OFFSET
            // min heightmap is 0
            // max heightmap is 255
            // original value to divide by z is height - heightmap
            // so values range from -(255-height_offset) to 255+HEIGHT_OFFSET
            // so from -255 to 255 is the range, if we factor out HEIGHT_OFFSET being added
            // let's use -256 as the minimum for optimization
            div_z_values[ i*Z_COUNT_MAX + h ] = fix32Div(  intToFix32(-256 + HEIGHT_OFFSET + (h << HEIGHT_DIV_SHIFT)), z );
        }
    }
    z_count = i;
    */

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
            phi = (phi+20) % 1024;
        }
        if (value & BUTTON_RIGHT)
        {
            phi = (phi+1024-20) % 1024;
        }
    }
}


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

    u16 adjphi = (phi + 256 + 512)%1024;
    fix32 sinphi = sinFix32(adjphi);
    fix32 cosphi = cosFix32(adjphi);

    s16 ybuffer[screen_width];
    for(u16 i = 0; i < screen_width; i++)
        ybuffer[i] = screen_height;


    // Draw from front to the back (low z coordinate to high z coordinate)
    u8 zidx = 0;
    while (zidx < z_count)
    {
        const fix32 z = z_values[zidx];

        // Find line on map. This calculation corresponds to a field of view of 90°
        fix32 pleft_x = fix32Sub(fix32Neg(fix32Mul(cosphi, z)), fix32Mul(sinphi,z)) + px;
        fix32 pleft_y = fix32Sub( fix32Mul(sinphi, z), fix32Mul(cosphi, z)) + py;
        fix32 pright_x = fix32Sub( fix32Mul(cosphi, z), fix32Mul(sinphi, z)) + px;
        fix32 pright_y = fix32Sub(fix32Neg(fix32Mul(sinphi, z)), fix32Mul(cosphi, z)) + py;

        // segment the line
        //fix32 dx = fix32Div(fix32Sub(pright_x, pleft_x)*2, FIX32(screen_width));
        //fix32 dy = fix32Div(fix32Sub(pright_y, pleft_y)*2, FIX32(screen_width));
        fix32 dx = fix32Sub(pright_x, pleft_x) >> 7;
        fix32 dy = fix32Sub(pright_y, pleft_y) >> 7;

        //Raster line and draw a vertical line for each segment
        for (u16 i = 0; i < screen_width; i+=2)
        {
            s16 idx_x = fix32ToInt(pleft_x);
            while(idx_x < 0)
                idx_x += 1024;
            idx_x %= 1024;
            s16 idx_y = fix32ToInt(pleft_y);
            while(idx_y < 0)
                idx_y += 1024;
            idx_y %= 1024;
            s16 heightmap = depth_bin[1024*idx_y + idx_x];

            // without div table
            // s16 height_on_screen = fix32ToInt( fix32Mul( fix32Div( intToFix32(((s16)height) - heightmap) , z ), FIX32(HEIGHT_SCALE)) + horizon );

            // with div table
            s16 rel_height = ((s16)height) - HEIGHT_OFFSET - heightmap; // ranges from -255 to 255
            u16 value_idx = (zidx * (512>>HEIGHT_DIV_SHIFT)) + ((rel_height + 256) >> HEIGHT_DIV_SHIFT);
            s16 height_on_screen = div_z_values[value_idx];

            if(height_on_screen < 0)
                height_on_screen = 0;

            if (height_on_screen < ybuffer[i])
            {
                u8 color = bmp_color.image[512 * idx_y + (idx_x/2)];
                const u16 start_y = height_on_screen;
                const u16 end_y = ybuffer[i];
                u8 * coladdr = BMP_getWritePointer(i, start_y);
                for(u16 curr_y = start_y; curr_y < end_y; curr_y++)
                {
                    *coladdr = color;
                    color = (color >> 4) + ((color & 0xf) << 4);
                    coladdr += BMP_PITCH;
                }

                ybuffer[i] = height_on_screen;
            }
            pleft_x += dx;
            pleft_y += dy;
        }

        zidx ++;
    }
}
