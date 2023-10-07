#include "genesis.h"
#include "resources.h"

#define guint u16
#define guint8 u8
#include "depth.h"


static void handleInput();
static void joyEvent(u16 joy, u16 changed, u16 state);

static void drawView();

fix32 px = FIX32(512);
fix32 py = FIX32(512);
u16 phi = 0;
fix32 height = FIX32(50);
fix32 horizon = FIX32(40);
static const u16 screen_width = BMP_WIDTH;
static const u16 screen_height = BMP_HEIGHT;
static const fix32 screen_width_inv = FIX32(1.0 / (BMP_WIDTH));


int main(bool hard)
{
    char col;

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

        // can now draw text
        BMP_showFPS(1);

        // display particul number
        // intToStr(numpartic, str, 1);
        // BMP_clearText(1, 3, 4);
        // BMP_drawText(str, 1, 3);

        // display gravity
        // fix16ToStr(gravity, str, 3);
        // BMP_clearText(1, 4, 5);
        // BMP_drawText(str, 1, 4);

        // clear bitmap
        BMP_clear();
        // draw particules
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
    fix32 heightmap = intToFix32(depth.pixel_data[depth.bytes_per_pixel*(depth.width*idx_y + idx_x)]);
    height = heightmap + FIX32(30);
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
    fix32 scale_height = FIX32(100);
    fix32 distance = FIX32(200);

    u16 adjphi = (phi + 256 + 512)%1024;
    fix32 sinphi = sinFix32(adjphi);
    fix32 cosphi = cosFix32(adjphi);

    fix32 ybuffer[screen_width];
    for(u16 i = 0; i < screen_width; i++)
        ybuffer[i] = FIX32(screen_height);


    // Draw from front to the back (low z coordinate to high z coordinate)
    fix32 dz = FIX32(0.5);
    fix32 z = FIX32(1);

    while (z < distance)
    {
        // Find line on map. This calculation corresponds to a field of view of 90°
        fix32 pleft_x = fix32Sub(fix32Neg(fix32Mul(cosphi, z)), fix32Mul(sinphi,z)) + px;
        fix32 pleft_y = fix32Sub( fix32Mul(sinphi, z), fix32Mul(cosphi, z)) + py;
        fix32 pright_x = fix32Sub( fix32Mul(cosphi, z), fix32Mul(sinphi, z)) + px;
        fix32 pright_y = fix32Sub(fix32Neg(fix32Mul(sinphi, z)), fix32Mul(cosphi, z)) + py;

        // segment the line
        fix32 dx = fix32Div(fix32Sub(pright_x, pleft_x)*2, FIX32(screen_width));
        fix32 dy = fix32Div(fix32Sub(pright_y, pleft_y)*2, FIX32(screen_width));

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
            fix32 heightmap = intToFix32(depth.pixel_data[depth.bytes_per_pixel*(depth.width*idx_y + idx_x)]);
            fix32 height_on_screen = fix32Mul( fix32Div( fix32Sub(height, heightmap) , z ), scale_height) + horizon;

            if(height_on_screen < 0)
                height_on_screen = 0;

            if (height_on_screen < ybuffer[i])
            {
                u8 color = bmp_color.image[512 * idx_y + (idx_x/2)];
                u16 start_y = fix32ToInt(height_on_screen);
                u16 end_y = fix32ToInt(ybuffer[i]);
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

        // Go to next line and increase step size when you are far away
        z += dz;
        dz += FIX32(0.75);
    }
}
