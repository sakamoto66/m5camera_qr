#pragma once

void qr_recoginze(camera_fb_t *fb, void (*fnc)(int, const struct quirc_code *qcode));
void dump_qrcode_cells(const struct quirc_code *code);
void dump_qrcode_info(const struct quirc_data *data);