#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_camera.h"
#include "quirc_internal.h"


static const char *data_type_str(int dt) {
	switch (dt) {
	case QUIRC_DATA_TYPE_NUMERIC:
		return "NUMERIC";
	case QUIRC_DATA_TYPE_ALPHA:
		return "ALPHA";
	case QUIRC_DATA_TYPE_BYTE:
		return "BYTE";
	case QUIRC_DATA_TYPE_KANJI:
		return "KANJI";
	}

	return "unknown";
}

int qr_recoginze(camera_fb_t *fb, void (*fnc)(int, const struct quirc_code *qcode)) {
	int w = fb->width;
	int h = fb->height;

	struct quirc *q;
	q = quirc_new();
	if (!q) {
		printf("can't create quirc object\r\n");
		return -1;
	}

	quirc_begin(q, NULL, NULL);
	q->w = w;
	q->h = h;	
	q->image = fb->buf;
	quirc_end(q);

	int count = quirc_count(q);
	for (int i = 0; i < count; i++) {
		struct quirc_code code;
		quirc_extract(q, i, &code);
		fnc(i, &code);
	}

	q->image = NULL;
	quirc_destroy(q);
	return count;
}

void dump_qrcode_cells(const struct quirc_code *code) {
	int u, v;

	printf("    %d cells, corners:", code->size);
	for (u = 0; u < 4; u++)
		printf(" (%d,%d)", code->corners[u].x, code->corners[u].y);
	printf("\n");

	for (v = 0; v < code->size; v++) {
		printf("\033[0m    ");
		for (u = 0; u < code->size; u++) {
			int p = v * code->size + u;

			if (code->cell_bitmap[p >> 3] & (1 << (p & 7)))
				printf("\033[40m  ");
			else
				printf("\033[47m  ");
		}
		printf("\033[0m\n");
	}
}

void dump_qrcode_info(const struct quirc_data *data) {
	printf("----------------------------------\n");
	printf("    Version: %d\n", data->version);
	printf("    ECC level: %c\n", "MLHQ"[data->ecc_level]);
	printf("    Mask: %d\n", data->mask);
	printf("    Data type: %d (%s)\n", data->data_type, data_type_str(data->data_type));
	printf("    Length: %d\n", data->payload_len);
	printf("    Payload: %s\n", data->payload);

	if (data->eci)
		printf("    ECI: %d\n", data->eci);
}

