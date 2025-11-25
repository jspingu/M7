#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define POWER_CURVE_SHIFT  0.055
#define             GAMMA  2.4

uint16_t gamma_decode_lut[256];
uint8_t gamma_encode_lut[65536];

void init_gamma_luts(void) {
    double decode_breakpoint = POWER_CURVE_SHIFT / (GAMMA - 1);
    double encode_breakpoint = pow((decode_breakpoint + POWER_CURVE_SHIFT) / (1 + POWER_CURVE_SHIFT), GAMMA);

    for (int i = 0; i < 256; ++i) {
        double normalized = (double)i / 255;
        gamma_decode_lut[i] = (normalized <= decode_breakpoint) ? 
                              (uint16_t) round(normalized * encode_breakpoint / decode_breakpoint * 65535) :
                              (uint16_t) round(pow((normalized + POWER_CURVE_SHIFT) / (1 + POWER_CURVE_SHIFT), GAMMA) * 65535);
    }

    for (int i = 0; i < 65536; ++i) {
        double normalized = (double)i / 65535;
        gamma_encode_lut[i] = (normalized <= encode_breakpoint) ?
                              (uint8_t) round(normalized * decode_breakpoint / encode_breakpoint * 255) :
                              (uint8_t) round((pow(normalized, 1 / GAMMA) * (1 + POWER_CURVE_SHIFT) - POWER_CURVE_SHIFT) * 255);
    }
}

int main(void) {
    init_gamma_luts();

    fprintf(stdout,
        "#include <stdint.h>\n"
        "const uint16_t gamma_decode_lut[256] = { "
    );

    for (int i = 0; i < 256; ++i)
        fprintf(stdout, "%u, ", gamma_decode_lut[i]);

    fprintf(stdout,
        "};\n"
        "const uint8_t gamma_encode_lut[65536] = { "
    );

    for (int i = 0; i < 65536; ++i)
        fprintf(stdout, "%u, ", gamma_encode_lut[i]);

    fprintf(stdout, "};");
    return 0;
}
