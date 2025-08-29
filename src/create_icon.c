#include <stdio.h>
#include <stdint.h>

// 创建一个简单的16x16 ICO文件
int main() {
    FILE *f = fopen("icon.ico", "wb");
    if (!f) return 1;
    
    // ICO文件头
    uint16_t reserved = 0;
    uint16_t type = 1;      // 1 = ICO
    uint16_t count = 1;     // 1个图像
    
    fwrite(&reserved, 2, 1, f);
    fwrite(&type, 2, 1, f);
    fwrite(&count, 2, 1, f);
    
    // 图像目录条目
    uint8_t width = 16;
    uint8_t height = 16;
    uint8_t colors = 0;     // 0 = 256色以上
    uint8_t reserved2 = 0;
    uint16_t planes = 1;
    uint16_t bpp = 32;      // 32位色彩
    uint32_t size = 40 + 16*16*4 + 16*16/8; // 头部 + 像素数据 + 掩码
    uint32_t offset = 22;   // 从文件开始的偏移
    
    fwrite(&width, 1, 1, f);
    fwrite(&height, 1, 1, f);
    fwrite(&colors, 1, 1, f);
    fwrite(&reserved2, 1, 1, f);
    fwrite(&planes, 2, 1, f);
    fwrite(&bpp, 2, 1, f);
    fwrite(&size, 4, 1, f);
    fwrite(&offset, 4, 1, f);
    
    // BMP头部
    uint32_t header_size = 40;
    uint32_t bmp_width = 16;
    uint32_t bmp_height = 32; // 高度是实际的2倍（包含掩码）
    uint16_t bmp_planes = 1;
    uint16_t bmp_bpp = 32;
    uint32_t compression = 0;
    uint32_t image_size = 0;
    uint32_t x_ppm = 0;
    uint32_t y_ppm = 0;
    uint32_t colors_used = 0;
    uint32_t colors_important = 0;
    
    fwrite(&header_size, 4, 1, f);
    fwrite(&bmp_width, 4, 1, f);
    fwrite(&bmp_height, 4, 1, f);
    fwrite(&bmp_planes, 2, 1, f);
    fwrite(&bmp_bpp, 2, 1, f);
    fwrite(&compression, 4, 1, f);
    fwrite(&image_size, 4, 1, f);
    fwrite(&x_ppm, 4, 1, f);
    fwrite(&y_ppm, 4, 1, f);
    fwrite(&colors_used, 4, 1, f);
    fwrite(&colors_important, 4, 1, f);
    
    // 像素数据 (16x16, 32位BGRA，从下到上)
    for (int y = 15; y >= 0; y--) {
        for (int x = 0; x < 16; x++) {
            uint8_t b, g, r, a;
            
            // 创建一个简单的绿色圆形图标
            int dx = x - 8;
            int dy = y - 8;
            if (dx*dx + dy*dy <= 36) { // 圆形
                b = 34;   // 绿色
                g = 139;
                r = 34;
                a = 255;
            } else {
                b = g = r = a = 0; // 透明
            }
            
            fwrite(&b, 1, 1, f);
            fwrite(&g, 1, 1, f);
            fwrite(&r, 1, 1, f);
            fwrite(&a, 1, 1, f);
        }
    }
    
    // AND掩码 (16x16, 1位，从下到上)
    for (int y = 15; y >= 0; y--) {
        uint8_t mask_byte = 0;
        for (int x = 0; x < 16; x++) {
            int dx = x - 8;
            int dy = y - 8;
            if (dx*dx + dy*dy > 36) {
                mask_byte |= (1 << (7 - (x % 8)));
            }
            if (x % 8 == 7) {
                fwrite(&mask_byte, 1, 1, f);
                mask_byte = 0;
            }
        }
    }
    
    fclose(f);
    printf("Icon created successfully\n");
    return 0;
}
